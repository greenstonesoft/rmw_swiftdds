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

#ifndef RMW_SWIFTDDS_SHARED_CPP__CUSTOM_PARTICIPANT_INFO_HPP_
#define RMW_SWIFTDDS_SHARED_CPP__CUSTOM_PARTICIPANT_INFO_HPP_

#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "rcpputils/thread_safety_annotations.hpp"
#include "rcutils/logging_macros.h"

#include "rmw/impl/cpp/key_value.hpp"
#include "rmw/qos_profiles.h"
#include "rmw/rmw.h"

#include "rmw_dds_common/context.hpp"
#include "rmw_dds_common/qos.hpp"

#include "rmw_swiftdds_shared_cpp/create_rmw_gid.hpp"
#include "rmw_swiftdds_shared_cpp/custom_event_info.hpp"
#include "rmw_swiftdds_shared_cpp/qos.hpp"
#include "rmw_swiftdds_shared_cpp/rmw_common.hpp"
#include "swiftdds/dcps/SwiftDdsExport.h"

using rmw_dds_common::operator<<;

class ParticipantListener;

enum class publishing_mode_t
{
  ASYNCHRONOUS,  // Asynchronous publishing mode
  SYNCHRONOUS,   // Synchronous publishing mode
  AUTO           // Use publishing mode set in XML file or DDS default
};

class CustomTopicListener final : public greenstone::dds::TopicListener
{
public:
  RMW_SWIFTDDS_SHARED_CPP_PUBLIC
  explicit CustomTopicListener(EventListenerInterface *event_listener);

  RMW_SWIFTDDS_SHARED_CPP_PUBLIC
  void
  on_inconsistent_topic(
    greenstone::dds::Topic *topic,
    greenstone::dds::InconsistentTopicStatus const & status) noexcept override;

  RMW_SWIFTDDS_SHARED_CPP_PUBLIC
  void add_event_listener(EventListenerInterface *event_listener);

  RMW_SWIFTDDS_SHARED_CPP_PUBLIC
  void remove_event_listener(EventListenerInterface *event_listener);

private:
  std::mutex event_listeners_mutex_;
  std::set<EventListenerInterface *>
  event_listeners_ RCPPUTILS_TSA_GUARDED_BY(event_listeners_mutex_);
};

typedef struct UseCountTopic
{
  greenstone::dds::Topic *topic{nullptr};
  CustomTopicListener *topic_listener{nullptr};
  size_t use_count{0};
} UseCountTopic;

typedef struct CustomParticipantInfo
{
  greenstone::dds::DomainParticipantFactory *factory_{
    greenstone::dds::DomainParticipantFactory::get_instance()
  };

  greenstone::dds::DomainParticipant *participant_{nullptr};
  ParticipantListener *listener_{nullptr};

  std::mutex topic_name_to_topic_mutex_;
  std::map<std::string, std::unique_ptr<UseCountTopic>> topic_name_to_topic_;
  std::map<std::string, std::shared_ptr<greenstone::dds::TopicDataType>> type_name_to_type_;

  greenstone::dds::Publisher *publisher_{nullptr};
  greenstone::dds::Subscriber *subscriber_{nullptr};

  // Protects creation and destruction of topics, readers and writers
  mutable std::mutex entity_creation_mutex_;

  // Flag to establish if the QoS of the DomainParticipant,
  // its DataWriters, and its DataReaders are going
  // to be configured only from an XML file or if
  // their settings are going to be overwritten by code
  // with the default configuration.
  bool leave_middleware_default_qos;
  publishing_mode_t publishing_mode;

  RMW_SWIFTDDS_SHARED_CPP_PUBLIC
  greenstone::dds::Topic * find_or_create_topic(
    const std::string & topic_name,
    const std::string & type_name,
    const std::string & type_identifier,
    const greenstone::dds::TopicQos & topic_qos,
    EventListenerInterface *event_listener);

  RMW_SWIFTDDS_SHARED_CPP_PUBLIC
  void delete_topic(
    const greenstone::dds::Topic *topic, const std::string & type_identifier,
    EventListenerInterface *event_listener);
} CustomParticipantInfo;

class ParticipantListener : public greenstone::dds::DomainParticipantListener
{
public:
  explicit ParticipantListener(const char *identifier, rmw_dds_common::Context *context)
  : context(context),
    identifier_(identifier)
  {
  }

  void
  on_participant_matched(
    greenstone::dds::DomainParticipant *participant,
    const greenstone::dds::ParticipantBuiltinTopicData & info,
    greenstone::dds::RemoteDiscoveryStatus const & reason) noexcept override
  {
    static_cast<void>(participant);
    greenstone::dds::GUID participant_guid;
    participant_guid.from_octet16(info.key().value());
    switch(reason) {
      case greenstone::dds::RemoteDiscoveryStatus::ONLINE: {
          auto map = rmw::impl::cpp::parse_key_value(info.user_data().value());
          auto name_found = map.find("enclave");

          if(name_found == map.end()) {
            return;
          }
          auto enclave = std::string(name_found->second.begin(), name_found->second.end());

          context->graph_cache.add_participant(
          rmw_swiftdds_shared_cpp::create_rmw_gid(identifier_, participant_guid), enclave);
          break;
        }
      case greenstone::dds::RemoteDiscoveryStatus::OFFLINE:
        context->graph_cache.remove_participant(
          rmw_swiftdds_shared_cpp::create_rmw_gid(identifier_, participant_guid));
        break;
      default:
        return;
    }
  }

  void on_reader_discovery(
    greenstone::dds::DomainParticipant *,
    const greenstone::dds::SubscriptionBuiltinTopicData & info,
    greenstone::dds::RemoteDiscoveryStatus const & reason) noexcept override
  {
    bool is_alive = greenstone::dds::RemoteDiscoveryStatus::ONLINE == reason;
    process_discovery_info(info, is_alive, true);
  }

  void on_writer_discovery(
    greenstone::dds::DomainParticipant *,
    const greenstone::dds::PublicationBuiltinTopicData & info,
    greenstone::dds::RemoteDiscoveryStatus const & reason) noexcept override
  {
    bool is_alive = greenstone::dds::RemoteDiscoveryStatus::ONLINE == reason;
    process_discovery_info(info, is_alive, false);
  }

private:
  template<class T> void process_discovery_info(T & proxyData, bool is_alive, bool is_reader)
  {
    {
      greenstone::dds::GUID pubsub_guid;
      pubsub_guid.from_octet16(proxyData.key().value());
      if(is_alive) {
        rmw_qos_profile_t qos_profile = rmw_qos_profile_unknown;
        rtps_qos_to_rmw_qos(proxyData, &qos_profile);

        const auto & userDataValue = proxyData.user_data().value();
        rosidl_type_hash_t type_hash;
        if(RMW_RET_OK != rmw_dds_common::parse_type_hash_from_user_data(
                             userDataValue.data(), userDataValue.size(), type_hash))
        {
          // Avoid deadlock trying to acquire rclcpp's global logging mutex
          // by using eProsima's logging mechanism.
          // TODO(sloretz) revisit when this is fixed:
          // https://github.com/ros2/rclcpp/issues/2147 EPROSIMA_LOG_WARNING(
          //   "rmw_swiftdds_shared_cpp", "Failed to parse a type hash for a
          //   topic");
          /*
          RCUTILS_LOG_WARN_NAMED(
            "rmw_swiftdds_shared_cpp",
            "Failed to parse type hash for topic '%s' with type '%s' from
          USER_DATA '%*s'.", proxyData.topicName().c_str(),
            proxyData.typeName().c_str(),
            static_cast<int>(userDataValue.size()),
            reinterpret_cast<const char *>(userDataValue.data()));
          */
          type_hash = rosidl_get_zero_initialized_type_hash();
          // We've handled the error, so clear it out.
          rmw_reset_error();
        }
        greenstone::dds::GUID participant_guid;
        participant_guid.from_octet16(proxyData.participant_key().value());
        rosidl_type_hash_t ser_type_hash;
        rosidl_type_hash_t * ser_type_hash_ptr = nullptr;
        if (RMW_RET_OK == rmw_dds_common::parse_sertype_hash_from_user_data(
            userDataValue.data(), userDataValue.size(), ser_type_hash))
        {
          ser_type_hash_ptr = &ser_type_hash;
        }
        context->graph_cache.add_entity(
            rmw_swiftdds_shared_cpp::create_rmw_gid(identifier_, pubsub_guid),
            proxyData.topic_name(),
            proxyData.type_name(),
            type_hash,
            rmw_swiftdds_shared_cpp::create_rmw_gid(identifier_, participant_guid),
            qos_profile,
            is_reader,
            ser_type_hash_ptr);
      } else {
        context->graph_cache.remove_entity(
            rmw_swiftdds_shared_cpp::create_rmw_gid(identifier_, pubsub_guid), is_reader);
      }
    }
  }

  rmw_dds_common::Context *context;
  const char *const identifier_;
};

#endif  // RMW_SWIFTDDS_SHARED_CPP__CUSTOM_PARTICIPANT_INFO_HPP_
