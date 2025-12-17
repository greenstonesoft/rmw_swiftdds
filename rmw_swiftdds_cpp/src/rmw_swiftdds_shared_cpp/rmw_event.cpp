// Copyright 2020 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#include <unordered_set>

#include "rmw/impl/cpp/macros.hpp"

#include "event_helpers.hpp"
#include "rmw_swiftdds_shared_cpp/custom_event_info.hpp"
#include "rmw_swiftdds_shared_cpp/rmw_common.hpp"
#include "types/event_types.hpp"
#include "swiftdds/dcps/SwiftDdsExport.h"

static const std::unordered_set<rmw_event_type_t> g_rmw_event_type_set{
  RMW_EVENT_LIVELINESS_CHANGED,
  RMW_EVENT_REQUESTED_DEADLINE_MISSED,
  RMW_EVENT_LIVELINESS_LOST,
  RMW_EVENT_OFFERED_DEADLINE_MISSED,
  RMW_EVENT_MESSAGE_LOST,
  RMW_EVENT_OFFERED_QOS_INCOMPATIBLE,
  RMW_EVENT_REQUESTED_QOS_INCOMPATIBLE,
  RMW_EVENT_SUBSCRIPTION_INCOMPATIBLE_TYPE,
  RMW_EVENT_PUBLISHER_INCOMPATIBLE_TYPE,
  RMW_EVENT_SUBSCRIPTION_MATCHED,
  RMW_EVENT_PUBLICATION_MATCHED
};

namespace rmw_swiftdds_shared_cpp
{
namespace internal
{

DDS::StatusMask rmw_event_to_dds_statusmask(const rmw_event_type_t event_type)
{
  DDS::StatusMask ret_statusmask{DDS::EmptyStatusMask};
  switch(event_type) {
    case RMW_EVENT_LIVELINESS_CHANGED:
      ret_statusmask = DDS::StatusKind::LIVELINESS_CHANGED_STATUS;
      break;
    case RMW_EVENT_REQUESTED_DEADLINE_MISSED:
      ret_statusmask = DDS::StatusKind::REQUESTED_DEADLINE_MISSED_STATUS;
      break;
    case RMW_EVENT_LIVELINESS_LOST:
      ret_statusmask = DDS::StatusKind::LIVELINESS_LOST_STATUS;
      break;
    case RMW_EVENT_OFFERED_DEADLINE_MISSED:
      ret_statusmask = DDS::StatusKind::OFFERED_DEADLINE_MISSED_STATUS;
      break;
    case RMW_EVENT_MESSAGE_LOST:
      ret_statusmask = DDS::StatusKind::SAMPLE_LOST_STATUS;
      break;
    case RMW_EVENT_OFFERED_QOS_INCOMPATIBLE:
      ret_statusmask = DDS::StatusKind::OFFERED_INCOMPATIBLE_QOS_STATUS;
      break;
    case RMW_EVENT_REQUESTED_QOS_INCOMPATIBLE:
      ret_statusmask = DDS::StatusKind::REQUESTED_INCOMPATIBLE_QOS_STATUS;
      break;
    case RMW_EVENT_SUBSCRIPTION_INCOMPATIBLE_TYPE:
      ret_statusmask = DDS::StatusKind::INCONSISTENT_TOPIC_STATUS;
      break;
    case RMW_EVENT_PUBLISHER_INCOMPATIBLE_TYPE:
      ret_statusmask = DDS::StatusKind::INCONSISTENT_TOPIC_STATUS;
      break;
    case RMW_EVENT_SUBSCRIPTION_MATCHED:
      ret_statusmask = DDS::StatusKind::SUBSCRIPTION_MATCHED_STATUS;
      break;
    case RMW_EVENT_PUBLICATION_MATCHED:
      ret_statusmask = DDS::StatusKind::PUBLICATION_MATCHED_STATUS;
      break;
    default:
      break;
  }

  return ret_statusmask;
}

bool is_event_supported(rmw_event_type_t event_type)
{
  return g_rmw_event_type_set.count(event_type) == 1;
}

rmw_qos_policy_kind_t dds_qos_policy_to_rmw_qos_policy(DDS::QosPolicyId_t policy_id)
{
  switch(policy_id) {
    case dds::qos::DURABILITY_QOS_POLICY_ID:
      return RMW_QOS_POLICY_DURABILITY;
    case dds::qos::DEADLINE_QOS_POLICY_ID:
      return RMW_QOS_POLICY_DEADLINE;
    case dds::qos::LIVELINESS_QOS_POLICY_ID:
      return RMW_QOS_POLICY_LIVELINESS;
    case dds::qos::RELIABILITY_QOS_POLICY_ID:
      return RMW_QOS_POLICY_RELIABILITY;
    case dds::qos::HISTORY_QOS_POLICY_ID:
      return RMW_QOS_POLICY_HISTORY;
    case dds::qos::LIFESPAN_QOS_POLICY_ID:
      return RMW_QOS_POLICY_LIFESPAN;
    default:
      return RMW_QOS_POLICY_INVALID;
  }
}

}  // namespace internal

rmw_ret_t __rmw_init_event(
  const char *identifier,
  rmw_event_t *rmw_event,
  const char *topic_endpoint_impl_identifier,
  void *data,
  rmw_event_type_t event_type)
{
  RMW_CHECK_ARGUMENT_FOR_NULL(identifier, RMW_RET_INVALID_ARGUMENT);
  RMW_CHECK_ARGUMENT_FOR_NULL(rmw_event, RMW_RET_INVALID_ARGUMENT);
  RMW_CHECK_ARGUMENT_FOR_NULL(topic_endpoint_impl_identifier, RMW_RET_INVALID_ARGUMENT);
  RMW_CHECK_ARGUMENT_FOR_NULL(data, RMW_RET_INVALID_ARGUMENT);
  RMW_CHECK_TYPE_IDENTIFIERS_MATCH(topic endpoint,
                                   topic_endpoint_impl_identifier,
                                   identifier,
                                   return RMW_RET_INCORRECT_RMW_IMPLEMENTATION);
  if(!internal::is_event_supported(event_type)) {
    RMW_SET_ERROR_MSG_WITH_FORMAT_STRING("provided event_type is not supported by %s", identifier);
    return RMW_RET_UNSUPPORTED;
  }

  rmw_event->implementation_identifier = topic_endpoint_impl_identifier;
  rmw_event->data = data;
  rmw_event->event_type = event_type;
  CustomEventInfo *event = static_cast<CustomEventInfo *>(rmw_event->data);
  DDS::StatusMask status_mask =
    event->get_listener()->get_statuscondition()->get_enabled_statuses();
  status_mask |= rmw_swiftdds_shared_cpp::internal::rmw_event_to_dds_statusmask(event_type);
  event->get_listener()->get_statuscondition()->set_enabled_statuses(status_mask);

  return RMW_RET_OK;
}

rmw_ret_t __rmw_event_set_callback(
  rmw_event_t *rmw_event,
  rmw_event_callback_t callback,
  const void *user_data)
{
  auto custom_event_info = static_cast<CustomEventInfo *>(rmw_event->data);
  custom_event_info->get_listener()->set_on_new_event_callback(
      rmw_event->event_type, user_data, callback);
  return RMW_RET_OK;
}

bool __rmw_event_type_is_supported(rmw_event_type_t rmw_event_type)
{
  return rmw_swiftdds_shared_cpp::internal::is_event_supported(rmw_event_type);
}

}  // namespace rmw_swiftdds_shared_cpp
