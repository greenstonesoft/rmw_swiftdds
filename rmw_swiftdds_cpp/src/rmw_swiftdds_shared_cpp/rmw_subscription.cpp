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

#include <utility>
#include <string>

#include "rmw/allocators.h"
#include "rmw/error_handling.h"
#include "rmw/rmw.h"

#include "rcpputils/scope_exit.hpp"

#include "rmw_swiftdds_shared_cpp/custom_participant_info.hpp"
#include "rmw_swiftdds_shared_cpp/custom_subscriber_info.hpp"
#include "rmw_swiftdds_shared_cpp/namespace_prefix.hpp"
#include "rmw_swiftdds_shared_cpp/qos.hpp"
#include "rmw_swiftdds_shared_cpp/rmw_common.hpp"
#include "rmw_swiftdds_shared_cpp/rmw_context_impl.hpp"
#include "rmw_swiftdds_shared_cpp/subscription.hpp"
#include "rmw_swiftdds_shared_cpp/utils.hpp"
#include "rmw_swiftdds_shared_cpp/TypeSupport.hpp"

#include "swiftdds/dcps/SwiftDdsExport.h"

namespace rmw_swiftdds_shared_cpp
{
rmw_ret_t __rmw_destroy_subscription(
  const char *identifier,
  const rmw_node_t *node,
  rmw_subscription_t *subscription,
  bool reset_cft)
{
  assert(node->implementation_identifier == identifier);
  assert(subscription->implementation_identifier == identifier);

  rmw_ret_t ret = RMW_RET_OK;
  rmw_error_state_t error_state;
  rmw_error_string_t error_string;
  auto common_context = static_cast<rmw_dds_common::Context *>(node->context->impl->common);
  auto info = static_cast<const CustomSubscriberInfo *>(subscription->data);
  // Update graph
  ret = common_context->remove_subscriber_graph(
      info->subscription_gid_, node->name, node->namespace_);
  if(RMW_RET_OK != ret) {
    error_state = *rmw_get_error_state();
    error_string = rmw_get_error_string();
    rmw_reset_error();
  }

  auto participant_info =
    static_cast<CustomParticipantInfo *>(node->context->impl->participant_info);
  rmw_ret_t local_ret = destroy_subscription(identifier, participant_info, subscription, reset_cft);
  if(RMW_RET_OK != local_ret) {
    if(RMW_RET_OK != ret) {
      RMW_SAFE_FWRITE_TO_STDERR(error_string.str);
      RMW_SAFE_FWRITE_TO_STDERR(" during '" RCUTILS_STRINGIFY(__function__) "'\n");
    }
    ret = local_ret;
  } else if(RMW_RET_OK != ret) {
    rmw_set_error_state(error_state.message, error_state.file, error_state.line_number);
  }
  return ret;
}

rmw_ret_t __rmw_subscription_count_matched_publishers(
  const rmw_subscription_t *subscription,
  size_t *publisher_count)
{
  auto info = static_cast<CustomSubscriberInfo *>(subscription->data);

  *publisher_count = info->subscription_event_->publisher_count();

  return RMW_RET_OK;
}

rmw_ret_t __rmw_subscription_get_actual_qos(
  const rmw_subscription_t *subscription,
  rmw_qos_profile_t *qos)
{
  auto info = static_cast<CustomSubscriberInfo *>(subscription->data);
  greenstone::dds::DataReader *swiftdds_dr = info->data_reader_;
  greenstone::dds::DataReaderQos dds_qos;
  swiftdds_dr->get_qos(dds_qos);

  dds_qos_to_rmw_qos(dds_qos, qos);

  return RMW_RET_OK;
}

rmw_ret_t
__rmw_subscription_set_content_filter(
  rmw_subscription_t * subscription,
  const rmw_subscription_content_filter_options_t * options
)
{
  return RMW_RET_UNSUPPORTED;
}

rmw_ret_t
__rmw_subscription_get_content_filter(
  const rmw_subscription_t * subscription,
  rcutils_allocator_t * allocator,
  rmw_subscription_content_filter_options_t * options
)
{
  return RMW_RET_UNSUPPORTED;
}

rmw_ret_t __rmw_subscription_set_on_new_message_callback(
  rmw_subscription_t *rmw_subscription,
  rmw_event_callback_t callback,
  const void *user_data)
{
  auto custom_subscriber_info = static_cast<CustomSubscriberInfo *>(rmw_subscription->data);
  custom_subscriber_info->subscription_event_->set_on_new_message_callback(user_data, callback);
  return RMW_RET_OK;
}

}  // namespace rmw_swiftdds_shared_cpp
