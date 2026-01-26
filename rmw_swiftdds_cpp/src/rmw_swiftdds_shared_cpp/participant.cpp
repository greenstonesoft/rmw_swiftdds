// Copyright 2019 Open Source Robotics Foundation, Inc.
// Copyright 2016-2018 Proyectos y Sistemas de Mantenimiento SL (eProsima).
// Copyright 2025 GreenStoneSoft, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include <fstream>

#include <nlohmann/json.hpp>

#include "rcpputils/scope_exit.hpp"
#include "rcutils/env.h"
#include "rcutils/filesystem.h"

#include "rmw/allocators.h"

#include "rmw_swiftdds_shared_cpp/custom_participant_info.hpp"
#include "rmw_swiftdds_shared_cpp/participant.hpp"
#include "rmw_swiftdds_shared_cpp/rmw_common.hpp"
#include "rmw_swiftdds_shared_cpp/rmw_security_logging.hpp"
#include "rmw_swiftdds_shared_cpp/utils.hpp"
#include "rmw_swiftdds_shared_cpp/qos_config.hpp"

#include "rmw_security_common/security.hpp"

#include "swiftdds/dcps/SwiftDdsExport.h"

extern bool G_IS_DAEMON_NODE;

// Private function to create Participant with QoS
static CustomParticipantInfo *
__create_participant(
  const char *identifier,
  const greenstone::dds::DomainParticipantQos & domainParticipantQos,
  bool leave_middleware_default_qos,
  publishing_mode_t publishing_mode,
  rmw_dds_common::Context *common_context,
  size_t domain_id)
{
  CustomParticipantInfo *participant_info = nullptr;

  /////
  // Create Custom Participant
  try {
    participant_info = new CustomParticipantInfo();
  } catch(std::bad_alloc &) {
    RMW_SET_ERROR_MSG("__create_participant failed to allocate CustomParticipantInfo struct");
    return nullptr;
  }

  // lambda to delete participant info
  auto cleanup_participant_info = rcpputils::make_scope_exit([participant_info]() {
        if(nullptr != participant_info->participant_) {
          participant_info->participant_->delete_publisher(participant_info->publisher_);
          participant_info->factory_->delete_participant(participant_info->participant_);
        }
        delete participant_info->listener_;
        delete participant_info;
  });

  /////
  // Create Participant listener
  try {
    participant_info->listener_ = new ParticipantListener(identifier, common_context);
  } catch(std::bad_alloc &) {
    RMW_SET_ERROR_MSG("__create_participant failed to allocate participant listener");
    return nullptr;
  }

  /////
  // Create Participant

  // As the participant listener is only used for discovery related callbacks,
  // which are DDS extensions to the DDS standard DomainParticipantListener
  // interface, an empty mask should be used to let child entities handle
  // standard DDS events.
  greenstone::dds::StatusMask participant_mask =
    greenstone::dds::StatusKind::READER_DISCOVERY_STATUS |
    greenstone::dds::StatusKind::WRITER_DISCOVERY_STATUS |
    greenstone::dds::StatusKind::PARTICIPANT_MATCHED_STATUS;

  participant_info->participant_ =
    participant_info->factory_->create_participant(static_cast<uint32_t>(domain_id),
                                                     domainParticipantQos,
                                                     participant_info->listener_,
                                                     participant_mask);

  if(!participant_info->participant_) {
    RMW_SET_ERROR_MSG("__create_participant failed to create participant");
    return nullptr;
  }

  /////
  // Set participant info parameters
  participant_info->leave_middleware_default_qos = leave_middleware_default_qos;
  participant_info->publishing_mode = publishing_mode;

  /////
  // Create Publisher
  greenstone::dds::PublisherQos publisherQos;
  participant_info->participant_->get_default_publisher_qos(publisherQos);
  publisherQos.entity_factory(domainParticipantQos.entity_factory());

  participant_info->publisher_ = participant_info->participant_->create_publisher(
      publisherQos, nullptr, greenstone::dds::EmptyStatusMask);
  if(!participant_info->publisher_) {
    RMW_SET_ERROR_MSG("__create_participant could not create publisher");
    return nullptr;
  }

  /////
  // Create Subscriber
  greenstone::dds::SubscriberQos subscriberQos;
  participant_info->participant_->get_default_subscriber_qos(subscriberQos);
  subscriberQos.entity_factory(domainParticipantQos.entity_factory());

  participant_info->subscriber_ = participant_info->participant_->create_subscriber(
      subscriberQos, nullptr, greenstone::dds::EmptyStatusMask);
  if(!participant_info->subscriber_) {
    RMW_SET_ERROR_MSG("__create_participant could not create subscriber");
    return nullptr;
  }

  cleanup_participant_info.cancel();

  return participant_info;
}

CustomParticipantInfo *
rmw_swiftdds_shared_cpp::create_participant(
  const char *identifier,
  size_t domain_id,
  const rmw_security_options_t *security_options,
  const rmw_discovery_options_t *discovery_options,
  const char *enclave,
  rmw_dds_common::Context *common_context)
{
  RCUTILS_CAN_RETURN_WITH_ERROR_OF(nullptr);

  if(!security_options) {
    RMW_SET_ERROR_MSG("security_options is null");
    return nullptr;
  }

  const char *env_value;
  const char *error_str;
  auto factory = greenstone::dds::DomainParticipantFactory::get_instance();
  greenstone::dds::DomainParticipantQos domainParticipantQos;
  factory->get_default_participant_qos(domainParticipantQos);

  error_str = rcutils_get_env("RMW_SWIFTDDS_CONFIG", &env_value);
  if(error_str != NULL) {
    RCUTILS_LOG_DEBUG_NAMED(
        "rmw_swiftdds_shared_cpp", "Error getting env var[RMW_SWIFTDDS_CONFIG]: %s\n", error_str);
    return nullptr;
  }

  if(env_value != nullptr) {
    if(strcmp(env_value, "") != 0) {
      RCUTILS_LOG_DEBUG_NAMED(
          "rmw_swiftdds_shared_cpp", "RMW_SWIFTDDS_CONFIG is set [%s]", env_value);
      std::string env_value_str = std::string(env_value);
      std::ifstream config_file(env_value_str);
      if(!config_file.is_open()) {
        RCUTILS_LOG_ERROR_NAMED("rmw_swiftdds_shared_cpp", "RMW_SWIFTDDS_CONFIG can't read!");
        return nullptr;
      }
      nlohmann::json config_json;
      try {
        config_file >> config_json;
      } catch(const nlohmann::json::parse_error & e) {
        RCUTILS_LOG_ERROR_NAMED("rmw_swiftdds_shared_cpp", "RMW_SWIFTDDS_CONFIG can't parse!");
        return nullptr;
      }
      greenstone::dds::ParticipantAttributes participant_attr =
        domainParticipantQos.rtps_participant_attributes();
      // IP
      if(config_json.contains("ip_address") && !G_IS_DAEMON_NODE) {
        auto ip_array = config_json["ip_address"];
        gstone::rtps::SPDPAttributes tempSpdp = participant_attr.spdp_attributes();

        for(const auto & ip : ip_array) {
          tempSpdp.add_meta_unicast_locator(gstone::rtps::Locator_t(
              gstone::rtps::LOCATOR_KIND_UDPv4, 0, ip.get<std::string>().c_str()));
        }
        participant_attr.spdp_attributes(tempSpdp);
      }
      // SHM
      if(config_json.contains("shared_memory") && !G_IS_DAEMON_NODE) {
        auto shm_enabled = config_json["shared_memory"];
        if(shm_enabled) {
          participant_attr.add_unicast_transport(gstone::rtps::LOCATOR_KIND_SHM, 0, "127.0.0.1");
        }
      }
      // SHM BUFFER SIZE
      if(config_json.contains("shared_memory_buffer_size")) {
        auto shm_buffer_size = config_json["shared_memory_buffer_size"];
        participant_attr.shared_memory_size(shm_buffer_size.get<uint32_t>());
      }
      // WLP
      if(config_json.contains("WLP")) {
        auto WLP_enabled = config_json["WLP"];
        if(WLP_enabled) {
          participant_attr.used_wlp(true);
        }
      }
      {
        std::lock_guard<std::mutex> lock{QosConfig::getInstance().qosConfig_mutex};
        // SEND MODE
        if (config_json.contains("send_mode")) {
          auto send_mode = config_json["send_mode"];
          if (send_mode == "sync") {
            QosConfig::getInstance().send_sync = true;
          } else if (send_mode == "async") {
            QosConfig::getInstance().send_sync = false;
          }
        }
        // enable_zero_copy
        if(config_json.contains("enable_zero_copy")) {
          auto enable_zero_copy = config_json["enable_zero_copy"];
          if(enable_zero_copy) {
            QosConfig::getInstance().enable_zero_copy = true;

            // zero copy shm size
            if(config_json.contains("zero_copy_shm_size")) {
              auto zero_copy_shm_size = config_json["zero_copy_shm_size"];
              QosConfig::getInstance().enable_zero_copy = zero_copy_shm_size.get<uint32_t>();
            }
          }
        }
      }
      domainParticipantQos.rtps_participant_attributes(participant_attr);
    } else {
      RCUTILS_LOG_DEBUG_NAMED("rmw_swiftdds_shared_cpp",
                             "RMW_SWIFTDDS_CONFIG is set [default config]");
    }
  }
  // local mode
  domainParticipantQos.rtps_participant_attributes().local_mode(
      greenstone::dds::LocalMode::TURN_ON_SYNC);
  // opt qos
  domainParticipantQos.rtps_participant_attributes().send_optional_qos(true);
  domainParticipantQos.rtps_participant_attributes().data_use_shared_memory(!G_IS_DAEMON_NODE);

  std::string user_data = std::string("enclave=") + std::string(enclave) + std::string(";");
  std::vector<octet> ud{};
  ud.assign(user_data.begin(), user_data.end());
  ud.push_back('\0');
  greenstone::dds::UserDataQosPolicy dp_Data;
  dp_Data.value(ud);
  domainParticipantQos.user_data(dp_Data);

  bool leave_middleware_default_qos = false;
  publishing_mode_t publishing_mode = publishing_mode_t::SYNCHRONOUS;

  if(security_options->security_root_path) {
    // if security_root_path provided, try to find the key and certificate files
#if HAVE_SECURITY
    rcutils_allocator_t allocator = rcutils_get_default_allocator();
    rcutils_string_map_t security_files_paths = rcutils_get_zero_initialized_string_map();
    rcutils_ret_t ret = rcutils_string_map_init(&security_files_paths, 0, allocator);

    if(ret != RMW_RET_OK) {
      RMW_SET_ERROR_MSG("Failed to initialize string map for security");
      return nullptr;
    }

    auto scope_exit_ws = rcpputils::make_scope_exit([&security_files_paths]() {
          rcutils_ret_t ret = rcutils_string_map_fini(&security_files_paths);
          if(ret != RMW_RET_OK) {
            RMW_SET_ERROR_MSG("Failed to fini string map for security");
          }
    });

    if(get_security_files_support_pkcs(
           true, "file://", security_options->security_root_path, &security_files_paths) ==
      RMW_RET_OK)
    {
      greenstone::dds::PropertyPolicy property_policy;
      property_policy.properties().emplace_back("dds.sec.auth.plugin", "builtin.PKI-DH");
      property_policy.properties().emplace_back(
          "dds.sec.auth.builtin.PKI-DH.identity_ca",
          std::string(rcutils_string_map_get(&security_files_paths, "IDENTITY_CA")));
      property_policy.properties().emplace_back(
          "dds.sec.auth.builtin.PKI-DH.identity_certificate",
          std::string(rcutils_string_map_get(&security_files_paths, "CERTIFICATE")));
      property_policy.properties().emplace_back(
          "dds.sec.auth.builtin.PKI-DH.private_key",
          std::string(rcutils_string_map_get(&security_files_paths, "PRIVATE_KEY")));
      property_policy.properties().emplace_back("dds.sec.crypto.plugin", "builtin.AES-GCM-GMAC");

      property_policy.properties().emplace_back("dds.sec.access.plugin",
                                                "builtin.Access-Permissions");
      property_policy.properties().emplace_back(
          "dds.sec.access.builtin.Access-Permissions.permissions_ca",
          std::string(rcutils_string_map_get(&security_files_paths, "PERMISSIONS_CA")));
      property_policy.properties().emplace_back(
          "dds.sec.access.builtin.Access-Permissions.governance",
          std::string(rcutils_string_map_get(&security_files_paths, "GOVERNANCE")));
      property_policy.properties().emplace_back(
          "dds.sec.access.builtin.Access-Permissions.permissions",
          std::string(rcutils_string_map_get(&security_files_paths, "PERMISSIONS")));

      if(rcutils_string_map_key_exists(&security_files_paths, "CRL")) {
        property_policy.properties().emplace_back(
            "dds.sec.auth.builtin.PKI-DH.identity_crl",
            std::string(rcutils_string_map_get(&security_files_paths, "CRL")));
      }

      // Configure security logging
      if(!apply_security_logging_configuration(property_policy)) {
        return nullptr;
      }

      domainParticipantQos.properties(property_policy);
    } else if(security_options->enforce_security) {
      RMW_SET_ERROR_MSG("couldn't find all security files!");
      return nullptr;
    }
#else
    RMW_SET_ERROR_MSG("This DDS version doesn't have the security libraries\n"
                      "Please compile DDS using the -DSECURITY=ON CMake option");
    return nullptr;
#endif
  }
  return __create_participant(identifier,
                              domainParticipantQos,
                              leave_middleware_default_qos,
                              publishing_mode,
                              common_context,
                              domain_id);
}

rmw_ret_t rmw_swiftdds_shared_cpp::destroy_participant(CustomParticipantInfo *participant_info)
{
  if(!participant_info) {
    RMW_SET_ERROR_MSG("participant_info is null on destroy_participant");
    return RMW_RET_ERROR;
  }

  // Make the participant stop listening to discovery
  participant_info->participant_->set_listener(nullptr, greenstone::dds::EmptyStatusMask);

  // Delete Domain Participant
  participant_info->participant_->delete_contained_entities();
  auto ret = participant_info->factory_->delete_participant(participant_info->participant_);

  if(greenstone::dds::ReturnCode_t::RETCODE_OK != ret) {
    RCUTILS_SAFE_FWRITE_TO_STDERR("Failed to delete participant");
  }

  // Delete Listener
  delete participant_info->listener_;

  // Delete Custom Participant
  delete participant_info;

  RCUTILS_CAN_RETURN_WITH_ERROR_OF(RMW_RET_ERROR);  // on completion

  return RMW_RET_OK;
}
