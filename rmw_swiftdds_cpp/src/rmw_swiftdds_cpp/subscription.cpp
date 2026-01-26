// Copyright 2019 Open Source Robotics Foundation, Inc.
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

#include <rosidl_dynamic_typesupport/identifier.h>

#include <string>
#include <utility>

#include "rcutils/allocator.h"
#include "rcutils/error_handling.h"
#include "rcutils/macros.h"
#include "rcutils/strdup.h"
#include "rosidl_dynamic_typesupport/dynamic_message_type_support_struct.h"

#include "rmw/allocators.h"
#include "rmw/dynamic_message_type_support.h"
#include "rmw/error_handling.h"
#include "rmw/rmw.h"
#include "rmw/validate_full_topic_name.h"

#include "rcpputils/scope_exit.hpp"

#include "rmw_swiftdds_shared_cpp/custom_participant_info.hpp"
#include "rmw_swiftdds_shared_cpp/custom_subscriber_info.hpp"
#include "rmw_swiftdds_shared_cpp/names.hpp"
#include "rmw_swiftdds_shared_cpp/namespace_prefix.hpp"
#include "rmw_swiftdds_shared_cpp/qos.hpp"
#include "rmw_swiftdds_shared_cpp/rmw_common.hpp"
#include "rmw_swiftdds_shared_cpp/subscription.hpp"
#include "rmw_swiftdds_shared_cpp/utils.hpp"

#include "rmw_swiftdds_cpp/identifier.hpp"
#include "rmw_swiftdds_cpp/subscription.hpp"

#include "tracetools/tracetools.h"

#include "swiftdds/dcps/SwiftDdsExport.h"

using PropertyPolicyHelper = greenstone::dds::PropertyPolicyHelper;

namespace rmw_swiftdds_cpp
{

// Forward decls
// ===================================================================================
rmw_subscription_t * __create_subscription(
  CustomParticipantInfo *participant_info,
  const rosidl_message_type_support_t *type_supports,
  const char *topic_name,
  const rmw_qos_profile_t *qos_policies,
  const rmw_subscription_options_t *subscription_options,
  bool keyed);

// =================================================================================================
rmw_subscription_t * create_subscription(
  CustomParticipantInfo *participant_info,
  const rosidl_message_type_support_t *type_supports,
  const char *topic_name,
  const rmw_qos_profile_t *qos_policies,
  const rmw_subscription_options_t *subscription_options,
  bool keyed)
{
  /////
  // Check input parameters
  RCUTILS_CAN_RETURN_WITH_ERROR_OF(nullptr);

  RMW_CHECK_ARGUMENT_FOR_NULL(participant_info, nullptr);
  RMW_CHECK_ARGUMENT_FOR_NULL(type_supports, nullptr);
  RMW_CHECK_ARGUMENT_FOR_NULL(topic_name, nullptr);
  if(0 == strlen(topic_name)) {
    RMW_SET_ERROR_MSG("create_subscription() called with an empty topic_name argument");
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
      RMW_SET_ERROR_MSG_WITH_FORMAT_STRING(
          "create_subscription() called with invalid topic name: %s", reason);
      return nullptr;
    }
  }
  RMW_CHECK_ARGUMENT_FOR_NULL(subscription_options, nullptr);

  /////
  // Check RMW QoS
  if(!is_valid_qos(*qos_policies)) {
    RMW_SET_ERROR_MSG("create_subscription() called with invalid QoS");
    return nullptr;
  }

  /////
  // Get RMW Type Support

  // In the case it fails to find, rosidl_typesupport emits an error message
  rcutils_reset_error();

  return __create_subscription(
      participant_info, type_supports, topic_name, qos_policies, subscription_options, keyed);
}

// =================================================================================================
// CREATE RUNTIME SUBSCRIPTION
// =================================================================================================

// =================================================================================================
// CREATE SUBSCRIPTION
// =================================================================================================
rmw_subscription_t * __create_subscription(
  CustomParticipantInfo *participant_info,
  const rosidl_message_type_support_t *type_supports,
  const char *topic_name,
  const rmw_qos_profile_t *qos_policies,
  const rmw_subscription_options_t *subscription_options,
  bool keyed)
{
  std::lock_guard<std::mutex> lck(participant_info->entity_creation_mutex_);

  /////
  // Get Participant and Subscriber
  greenstone::dds::DomainParticipant *dds_participant = participant_info->participant_;
  greenstone::dds::Subscriber *subscriber = participant_info->subscriber_;

  /////
  // Create the custom Subscriber struct (info)
  auto info = new(std::nothrow) CustomSubscriberInfo();
  if(!info) {
    RMW_SET_ERROR_MSG("create_subscription() failed to allocate CustomSubscriberInfo");
    return nullptr;
  }

  // Create Topic and Type names
  info->typesupport_identifier_ = type_supports->typesupport_identifier;
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
        participant_info, info->subscription_event_, info->topic_, info->type_support_.get());
        delete info->subscription_event_;
        delete info->data_reader_listener_;
        delete info;
  });

  /////
  // Create Listener
  info->subscription_event_ = new(std::nothrow) RMWSubscriptionEvent(info);
  if(!info->subscription_event_) {
    RMW_SET_ERROR_MSG("create_subscription() could not create subscription event");
    return nullptr;
  }

  info->data_reader_listener_ =
    new(std::nothrow) CustomDataReaderListener(info->subscription_event_);
  if(!info->data_reader_listener_) {
    RMW_SET_ERROR_MSG("create_subscription() could not create subscription data reader "
                      "listener");
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

  info->dds_participant_ = dds_participant;
  info->subscriber_ = subscriber;
  info->topic_name_mangled_ = _create_topic_name(qos_policies, ros_topic_prefix, topic_name);
  info->topic_ = participant_info->find_or_create_topic(info->topic_name_mangled_,
                                                        info->type_support_->get_type_name(),
                                                        info->type_support_->type_identifier(),
                                                        topic_qos,
                                                        info->subscription_event_);
  if(!info->topic_) {
    RMW_SET_ERROR_MSG("create_subscription() failed to create topic");
    return nullptr;
  }

  /////
  // Create DataReader
  greenstone::dds::DataReaderQos reader_qos;
  subscriber->get_default_datareader_qos(reader_qos);

  // Try to load the profile with the topic name
  // It does not need to check the return code, as if the profile does not
  // exist, the QoS is already the default

  if(!get_datareader_qos(
         *qos_policies, *type_supports->get_type_hash_func(type_supports), reader_qos))
  {
    RMW_SET_ERROR_MSG("create_subscription() failed setting data reader QoS");
    return nullptr;
  }

  // Apply resource limits QoS if the type is keyed

  info->datareader_qos_ = reader_qos;

  // create_datareader
  info->data_reader_listener_mask_ = greenstone::dds::StatusKind::SUBSCRIPTION_MATCHED_STATUS;
  if(!rmw_swiftdds_shared_cpp::create_datareader(info->datareader_qos_,
                                                 subscription_options,
                                                 subscriber,
                                                 info->topic_,
                                                 info->data_reader_listener_,
                                                 info->data_reader_listener_mask_,
                                                 &info->data_reader_))
  {
    RMW_SET_ERROR_MSG("create_datareader() could not create data reader");
    return nullptr;
  }

  // Initialize DataReader's StatusCondition to be notified when new data is
  // available
  info->data_reader_->get_statuscondition()->set_enabled_statuses(
      greenstone::dds::StatusKind::DATA_AVAILABLE_STATUS);

  // lambda to delete datareader
  auto cleanup_datareader = rcpputils::make_scope_exit([subscriber, info]() {
        subscriber->delete_datareader(info->data_reader_);
  });

  /////
  // Create RMW GID
  greenstone::dds::GUID dr_guid;
  dr_guid.from_octet16(info->data_reader_->get_instance_handle().value);
  info->subscription_gid_ =
    rmw_swiftdds_shared_cpp::create_rmw_gid(greenstone_swiftdds_identifier, dr_guid);

  /////
  // Allocate subscription
  rmw_subscription_t *rmw_subscription = rmw_subscription_allocate();
  if(!rmw_subscription) {
    RMW_SET_ERROR_MSG("create_subscription() failed to allocate subscription");
    return nullptr;
  }
  auto cleanup_rmw_subscription = rcpputils::make_scope_exit([rmw_subscription]() {
        rmw_free(const_cast<char *>(rmw_subscription->topic_name));
        rmw_subscription_free(rmw_subscription);
  });

  rmw_subscription->can_loan_messages = info->type_support_->is_plain_types();
  rmw_subscription->implementation_identifier = greenstone_swiftdds_identifier;
  rmw_subscription->data = info;

  rmw_subscription->topic_name = rcutils_strdup(topic_name, rcutils_get_default_allocator());
  if(!rmw_subscription->topic_name) {
    RMW_SET_ERROR_MSG("create_subscription() failed to allocate memory for subscription "
                      "topic name");
    return nullptr;
  }
  rmw_subscription->options = *subscription_options;

  cleanup_rmw_subscription.cancel();
  cleanup_datareader.cancel();
  cleanup_info.cancel();

  TRACETOOLS_TRACEPOINT(rmw_subscription_init,
                        static_cast<const void *>(rmw_subscription),
                        info->subscription_gid_.data);
  return rmw_subscription;
}

}  // namespace rmw_swiftdds_cpp
