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

#include <string>

#include "rcutils/error_handling.h"
#include "rcutils/macros.h"

#include "rmw/allocators.h"
#include "rmw/error_handling.h"
#include "rmw/rmw.h"
#include "rmw/validate_full_topic_name.h"

#include "rcpputils/scope_exit.hpp"

#include "rmw_swiftdds_shared_cpp/create_rmw_gid.hpp"
#include "rmw_swiftdds_shared_cpp/custom_participant_info.hpp"
#include "rmw_swiftdds_shared_cpp/custom_publisher_info.hpp"
#include "rmw_swiftdds_shared_cpp/names.hpp"
#include "rmw_swiftdds_shared_cpp/namespace_prefix.hpp"
#include "rmw_swiftdds_shared_cpp/qos.hpp"
#include "rmw_swiftdds_shared_cpp/rmw_common.hpp"
#include "rmw_swiftdds_shared_cpp/utils.hpp"

#include "rmw_swiftdds_cpp/identifier.hpp"
#include "rmw_swiftdds_cpp/publisher.hpp"

#include "tracetools/tracetools.h"

#include "swiftdds/dcps/SwiftDdsExport.h"

extern bool G_IS_DAEMON_NODE;

rmw_publisher_t *
rmw_swiftdds_cpp::create_publisher(
  CustomParticipantInfo *participant_info,
  const rosidl_message_type_support_t *type_supports,
  const char *topic_name,
  const rmw_qos_profile_t *qos_policies,
  const rmw_publisher_options_t *publisher_options)
{
  /////
  // Check input parameters
  RCUTILS_CAN_RETURN_WITH_ERROR_OF(nullptr);

  RMW_CHECK_ARGUMENT_FOR_NULL(participant_info, nullptr);
  RMW_CHECK_ARGUMENT_FOR_NULL(type_supports, nullptr);
  RMW_CHECK_ARGUMENT_FOR_NULL(topic_name, nullptr);
  if(0 == strlen(topic_name)) {
    RMW_SET_ERROR_MSG("create_publisher() called with an empty topic_name argument");
    return nullptr;
  }
  RMW_CHECK_ARGUMENT_FOR_NULL(qos_policies, nullptr);
  if(!qos_policies->avoid_ros_namespace_conventions) {
    int validation_result = RMW_TOPIC_VALID;
    rmw_ret_t ret = rmw_validate_full_topic_name(topic_name, &validation_result, nullptr);
    if(RMW_RET_OK != ret) {
      return nullptr;
    }
    if(RMW_TOPIC_VALID != validation_result) {
      const char *reason = rmw_full_topic_name_validation_result_string(validation_result);
      RMW_SET_ERROR_MSG_WITH_FORMAT_STRING("create_publisher() called with invalid topic name: %s",
                                           reason);
      return nullptr;
    }
  }
  RMW_CHECK_ARGUMENT_FOR_NULL(publisher_options, nullptr);

  if(RMW_UNIQUE_NETWORK_FLOW_ENDPOINTS_STRICTLY_REQUIRED ==
    publisher_options->require_unique_network_flow_endpoints)
  {
    RMW_SET_ERROR_MSG("Unique network flow endpoints not supported on publishers");
    return nullptr;
  }

  /////
  // Check RMW QoS
  if(!is_valid_qos(*qos_policies)) {
    RMW_SET_ERROR_MSG("create_publisher() called with invalid QoS");
    return nullptr;
  }

  std::lock_guard<std::mutex> lck(participant_info->entity_creation_mutex_);

  /////
  // Get Participant and Publisher
  greenstone::dds::DomainParticipant *dds_participant = participant_info->participant_;
  greenstone::dds::Publisher *publisher = participant_info->publisher_;

  /////
  // Create the custom Publisher struct (info)
  auto info = new(std::nothrow) CustomPublisherInfo();
  if(!info) {
    RMW_SET_ERROR_MSG("create_publisher() failed to allocate CustomPublisherInfo");
    return nullptr;
  }

  // Create Topic and Type names
  auto type_support = rmw_swiftdds_shared_cpp::make_message_value_type(type_supports);
  auto type_name = type_support->get_type_name();
  std::string const type_map_key {type_name + ":" + type_support->type_identifier()};
  if(participant_info->participant_->is_type_registered(type_name,
    type_support->type_identifier()))
  {
    info->type_support_ = participant_info->type_name_to_type_[type_map_key];
  } else {
    info->type_support_ = type_support;
    info->type_support_->register_type(
        participant_info->participant_,
        const_cast<std::string &>(info->type_support_->get_type_name()));
    participant_info->type_name_to_type_[type_map_key] = info->type_support_;
  }

  auto cleanup_info = rcpputils::make_scope_exit([info, participant_info]() {
        rmw_swiftdds_shared_cpp::remove_topic_and_type(
        participant_info, info->publisher_event_, info->topic_, info->type_support_.get());
        delete info->data_writer_listener_;
        delete info->publisher_event_;
        delete info;
  });

  info->typesupport_identifier_ = type_supports->typesupport_identifier;

  /////
  // Create Listener
  info->publisher_event_ = new(std::nothrow) RMWPublisherEvent(info);
  if(!info->publisher_event_) {
    RMW_SET_ERROR_MSG("create_publisher() could not create publisher event");
    return nullptr;
  }

  info->data_writer_listener_ = new(std::nothrow) CustomDataWriterListener(info->publisher_event_);
  if(!info->data_writer_listener_) {
    RMW_SET_ERROR_MSG("create_publisher() could not create publisher data writer listener");
    return nullptr;
  }

  /////
  // Create and register Topic
  greenstone::dds::TopicQos topic_qos;
  dds_participant->get_default_topic_qos(topic_qos);
  if(!get_topic_qos(*qos_policies, topic_qos)) {
    // get_topic_qos already set the error
    return nullptr;
  }

  info->topic_ = participant_info->find_or_create_topic(
      _create_topic_name(qos_policies, ros_topic_prefix, topic_name),
      info->type_support_->get_type_name(),
      info->type_support_->type_identifier(),
      topic_qos,
      info->publisher_event_);
  if(!info->topic_) {
    RMW_SET_ERROR_MSG("create_publisher() failed to create topic");
    return nullptr;
  }

  /////
  // Create DataWriter
  greenstone::dds::DataWriterQos writer_qos;
  publisher->get_default_datawriter_qos(writer_qos);

  // Get QoS from RMW
  if(!get_datawriter_qos(
         *qos_policies, *type_supports->get_type_hash_func(type_supports), writer_qos))
  {
    RMW_SET_ERROR_MSG("create_publisher() failed setting data writer QoS");
    return nullptr;
  }

  if(G_IS_DAEMON_NODE) {
    auto attr = writer_qos.attributes();
    attr.enable_zero_copy(false);
    writer_qos.attributes(attr);
  }

  // Creates DataWriter with a mask enabling publication_matched calls for the
  // listener
  info->data_writer_listener_mask_ = greenstone::dds::StatusKind::PUBLICATION_MATCHED_STATUS;
  info->data_writer_ =
    publisher->create_datawriter(info->topic_,
                                   writer_qos,
                                   info->data_writer_listener_,
                                   info->data_writer_listener_mask_);

  if(!info->data_writer_) {
    RMW_SET_ERROR_MSG("create_publisher() could not create data writer");
    return nullptr;
  }

  // Set the StatusCondition to none to prevent triggering via WaitSets
  info->data_writer_->get_statuscondition()->set_enabled_statuses(greenstone::dds::EmptyStatusMask);

  // lambda to delete datawriter
  auto cleanup_datawriter = rcpputils::make_scope_exit([publisher, info]() {
        publisher->delete_datawriter(info->data_writer_);
  });

  /////
  // Create RMW GID
  greenstone::dds::GUID dw_guid;
  dw_guid.from_octet16(info->data_writer_->get_instance_handle().value);
  info->publisher_gid =
    rmw_swiftdds_shared_cpp::create_rmw_gid(greenstone_swiftdds_identifier, dw_guid);

  /////
  // Allocate publisher
  rmw_publisher_t *rmw_publisher = rmw_publisher_allocate();
  if(!rmw_publisher) {
    RMW_SET_ERROR_MSG("create_publisher() failed to allocate rmw_publisher");
    return nullptr;
  }
  auto cleanup_rmw_publisher = rcpputils::make_scope_exit([rmw_publisher]() {
        rmw_free(const_cast<char *>(rmw_publisher->topic_name));
        rmw_publisher_free(rmw_publisher);
  });

  // The type support in the RMW implementation is always XCDR1.
  rmw_publisher->can_loan_messages = info->type_support_->is_plain_types();
  rmw_publisher->implementation_identifier = greenstone_swiftdds_identifier;
  rmw_publisher->data = info;

  rmw_publisher->topic_name = static_cast<char *>(rmw_allocate(strlen(topic_name) + 1));
  if(!rmw_publisher->topic_name) {
    RMW_SET_ERROR_MSG("create_publisher() failed to allocate memory for rmw_publisher topic "
                      "name");
    return nullptr;
  }
  memcpy(const_cast<char *>(rmw_publisher->topic_name), topic_name, strlen(topic_name) + 1);

  rmw_publisher->options = *publisher_options;

  cleanup_rmw_publisher.cancel();
  cleanup_datawriter.cancel();
  cleanup_info.cancel();

  TRACETOOLS_TRACEPOINT(
      rmw_publisher_init, static_cast<const void *>(rmw_publisher), info->publisher_gid.data);
  return rmw_publisher;
}
