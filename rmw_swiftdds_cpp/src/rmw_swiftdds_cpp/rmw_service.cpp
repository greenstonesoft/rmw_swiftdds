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
#include <algorithm>
#include <array>
#include <cassert>
#include <condition_variable>
#include <limits>
#include <list>
#include <map>
#include <mutex>
#include <utility>
#include <set>
#include <string>
#include <vector>

#include "rcpputils/scope_exit.hpp"
#include "rcutils/error_handling.h"
#include "rcutils/logging_macros.h"

#include "rmw/allocators.h"
#include "rmw/error_handling.h"
#include "rmw/impl/cpp/macros.hpp"
#include "rmw/rmw.h"
#include "rmw/validate_full_topic_name.h"

#include "rmw_dds_common/qos.hpp"

#include "rmw_swiftdds_shared_cpp/custom_participant_info.hpp"
#include "rmw_swiftdds_shared_cpp/custom_service_info.hpp"
#include "rmw_swiftdds_shared_cpp/names.hpp"
#include "rmw_swiftdds_shared_cpp/namespace_prefix.hpp"
#include "rmw_swiftdds_shared_cpp/qos.hpp"
#include "rmw_swiftdds_shared_cpp/rmw_common.hpp"
#include "rmw_swiftdds_shared_cpp/rmw_context_impl.hpp"
#include "rmw_swiftdds_shared_cpp/utils.hpp"

#include "rmw_swiftdds_cpp/identifier.hpp"

#include "swiftdds/dcps/SwiftDdsExport.h"

extern "C" {
rmw_service_t * rmw_create_service(
  const rmw_node_t *node,
  const rosidl_service_type_support_t *type_supports,
  const char *service_name,
  const rmw_qos_profile_t *qos_policies)
{
  /////
  // Check input parameters
  RMW_CHECK_ARGUMENT_FOR_NULL(node, nullptr);
  RMW_CHECK_TYPE_IDENTIFIERS_MATCH(
      node, node->implementation_identifier, greenstone_swiftdds_identifier, return nullptr);
  RMW_CHECK_ARGUMENT_FOR_NULL(type_supports, nullptr);
  RMW_CHECK_ARGUMENT_FOR_NULL(service_name, nullptr);
  if(0 == strlen(service_name)) {
    RMW_SET_ERROR_MSG("service_name argument is an empty string");
    return nullptr;
  }
  RMW_CHECK_ARGUMENT_FOR_NULL(qos_policies, nullptr);
  if(!qos_policies->avoid_ros_namespace_conventions) {
    int validation_result = RMW_TOPIC_VALID;
    rmw_ret_t ret = rmw_validate_full_topic_name(service_name, &validation_result, nullptr);
    if(RMW_RET_OK != ret) {
      return nullptr;
    }
    if(RMW_TOPIC_VALID != validation_result) {
      const char *reason = rmw_full_topic_name_validation_result_string(validation_result);
      RMW_SET_ERROR_MSG_WITH_FORMAT_STRING("service_name argument is invalid: %s", reason);
      return nullptr;
    }
  }

  rmw_qos_profile_t adapted_qos_policies =
    rmw_dds_common::qos_profile_update_best_available_for_services(*qos_policies);

  /////
  // Check RMW QoS
  if(!is_valid_qos(adapted_qos_policies)) {
    RMW_SET_ERROR_MSG("create_service() called with invalid QoS");
    return nullptr;
  }

  /////
  // Get Participant and SubEntities
  auto common_context = static_cast<rmw_dds_common::Context *>(node->context->impl->common);
  auto participant_info =
    static_cast<CustomParticipantInfo *>(node->context->impl->participant_info);

  greenstone::dds::DomainParticipant *dds_participant = participant_info->participant_;
  greenstone::dds::Publisher *publisher = participant_info->publisher_;
  greenstone::dds::Subscriber *subscriber = participant_info->subscriber_;

  /////

  std::lock_guard<std::mutex> lck(participant_info->entity_creation_mutex_);

  /////
  // Create the custom Service struct (info)
  CustomServiceInfo *info = new(std::nothrow) CustomServiceInfo();
  if(!info) {
    RMW_SET_ERROR_MSG("create_service() failed to allocate custom info");
    return nullptr;
  }

  // Create Topic and Type names
  std::string request_topic_name = _create_topic_name(
      &adapted_qos_policies, ros_service_requester_prefix, service_name, "Request");
  std::string response_topic_name =
    _create_topic_name(&adapted_qos_policies, ros_service_response_prefix, service_name, "Reply");

  // Create the Type Support structs
  auto all_types{rmw_swiftdds_shared_cpp::make_request_response_value_types(type_supports)};
  std::string const req_type_map_key {all_types.first->get_type_name() + ":" +
    all_types.first->type_identifier()};
  std::string const res_type_map_key {all_types.second->get_type_name() + ":" +
    all_types.second->type_identifier()};
  if(participant_info->participant_->is_type_registered(all_types.first->get_type_name(),
      all_types.first->type_identifier()))
  {
    info->request_type_support_ =
      participant_info->type_name_to_type_[req_type_map_key];
    info->response_type_support_ =
      participant_info->type_name_to_type_[res_type_map_key];
  } else {
    info->request_type_support_ = all_types.first;
    info->response_type_support_ = all_types.second;
    info->request_type_support_->register_type(
        participant_info->participant_,
        const_cast<std::string &>(info->request_type_support_->get_type_name()));
    info->response_type_support_->register_type(
        participant_info->participant_,
        const_cast<std::string &>(info->response_type_support_->get_type_name()));
    participant_info->type_name_to_type_[req_type_map_key] =
      info->request_type_support_;
    participant_info->type_name_to_type_[res_type_map_key] =
      info->response_type_support_;
  }

  auto cleanup_info = rcpputils::make_scope_exit([info, participant_info]() {
        rmw_swiftdds_shared_cpp::remove_topic_and_type(
        participant_info, nullptr, info->response_topic_, info->response_type_support_.get());
        rmw_swiftdds_shared_cpp::remove_topic_and_type(
        participant_info, nullptr, info->request_topic_, info->request_type_support_.get());
        delete info->pub_listener_;
        delete info->listener_;
        delete info;
  });

  info->typesupport_identifier_ = type_supports->typesupport_identifier;

  /////
  // Create Listeners
  info->listener_ = new(std::nothrow) ServiceListener(info);
  if(!info->listener_) {
    RMW_SET_ERROR_MSG("create_service() failed to create request subscriber listener");
    return nullptr;
  }

  info->pub_listener_ = new(std::nothrow) ServicePubListener(info);
  if(!info->pub_listener_) {
    RMW_SET_ERROR_MSG("create_service() failed to create response publisher listener");
    return nullptr;
  }

  /////
  // Create and register Topics
  // Same default topic QoS for both topics
  greenstone::dds::TopicQos topic_qos;
  dds_participant->get_default_topic_qos(topic_qos);
  if(!get_topic_qos(adapted_qos_policies, topic_qos)) {
    // get_topic_qos already set the error
    return nullptr;
  }

  // Create request topic
  info->request_topic_ = participant_info->find_or_create_topic(
      request_topic_name, info->request_type_support_->get_type_name(),
      info->request_type_support_->type_identifier(), topic_qos, nullptr);
  if(!info->request_topic_) {
    RMW_SET_ERROR_MSG("create_service() failed to create request topic");
    return nullptr;
  }

  // Create response topic
  info->response_topic_ = participant_info->find_or_create_topic(
      response_topic_name, info->response_type_support_->get_type_name(),
      info->response_type_support_->type_identifier(), topic_qos, nullptr);
  if(!info->response_topic_) {
    RMW_SET_ERROR_MSG("create_service() failed to create response topic");
    return nullptr;
  }

  /////
  // Create request DataReader
  greenstone::dds::DataReaderQos reader_qos;
  subscriber->get_default_datareader_qos(reader_qos);

  // Try to load the profile named "service",
  // if it does not exist it tries with the request topic name
  // It does not need to check the return code, as if any of the profile does
  // not exist, the QoS is already set correctly: If none exist is default, if
  // only one exists is the one chosen, if both exist topic name is chosen

  const rosidl_type_hash_t * ser_type_hash = type_supports->get_type_hash_func(type_supports);
  if(!get_datareader_qos(adapted_qos_policies,
                         *type_supports->request_typesupport->get_type_hash_func(
                             type_supports->request_typesupport),
                         reader_qos, ser_type_hash))
  {
    RMW_SET_ERROR_MSG("create_service() failed setting request DataReader QoS");
    return nullptr;
  }

  // Creates DataReader
  info->listener_mask_ = greenstone::dds::StatusKind::SUBSCRIPTION_MATCHED_STATUS;
  info->request_reader_ =
    subscriber->create_datareader(info->request_topic_,
                                    reader_qos,
                                    info->listener_,
                                    info->listener_mask_);

  if(!info->request_reader_) {
    RMW_SET_ERROR_MSG("create_service() failed to create request DataReader");
    return nullptr;
  }

  info->request_reader_->get_statuscondition()->set_enabled_statuses(
      greenstone::dds::StatusKind::DATA_AVAILABLE_STATUS);

  // lambda to delete datareader
  auto cleanup_datareader = rcpputils::make_scope_exit([subscriber, info]() {
        subscriber->delete_datareader(info->request_reader_);
  });

  /////
  // Create response DataWriter
  greenstone::dds::DataWriterQos writer_qos;
  publisher->get_default_datawriter_qos(writer_qos);

  // Try to load the profile named "service",
  // if it does not exist it tries with the request topic name
  // It does not need to check the return code, as if any of the profile does
  // not exist, the QoS is already set correctly: If none exist is default, if
  // only one exists is the one chosen, if both exist topic name is chosen

  if(!get_datawriter_qos(adapted_qos_policies,
                         *type_supports->response_typesupport->get_type_hash_func(
                             type_supports->response_typesupport),
                         writer_qos, ser_type_hash))
  {
    RMW_SET_ERROR_MSG("create_service() failed setting response DataWriter QoS");
    return nullptr;
  }

  // Creates DataWriter with a mask enabling publication_matched calls for the
  // listener
  info->pub_listener_mask_ = greenstone::dds::StatusKind::PUBLICATION_MATCHED_STATUS;
  info->response_writer_ =
    publisher->create_datawriter(info->response_topic_,
                                   writer_qos,
                                   info->pub_listener_,
                                   info->pub_listener_mask_);

  if(!info->response_writer_) {
    RMW_SET_ERROR_MSG("create_service() failed to create response DataWriter");
    return nullptr;
  }

  // Set the StatusCondition to none to prevent triggering via WaitSets
  info->response_writer_->get_statuscondition()->set_enabled_statuses(
      greenstone::dds::EmptyStatusMask);

  // lambda to delete datawriter
  auto cleanup_datawriter = rcpputils::make_scope_exit([publisher, info]() {
        publisher->delete_datawriter(info->response_writer_);
  });

  /////
  // Create Service

  // Debug info
  RCUTILS_LOG_DEBUG_NAMED("rmw_swiftdds_cpp", "************ Service Details *********");
  RCUTILS_LOG_DEBUG_NAMED("rmw_swiftdds_cpp", "Sub Topic %s", request_topic_name.c_str());
  RCUTILS_LOG_DEBUG_NAMED("rmw_swiftdds_cpp", "Pub Topic %s", response_topic_name.c_str());
  RCUTILS_LOG_DEBUG_NAMED("rmw_swiftdds_cpp", "***********");

  rmw_service_t *rmw_service = rmw_service_allocate();
  if(!rmw_service) {
    RMW_SET_ERROR_MSG("create_service() failed to allocate memory for rmw_service");
    return nullptr;
  }
  auto cleanup_rmw_service = rcpputils::make_scope_exit([rmw_service]() {
        rmw_free(const_cast<char *>(rmw_service->service_name));
        rmw_free(rmw_service);
  });

  rmw_service->implementation_identifier = greenstone_swiftdds_identifier;
  rmw_service->data = info;
  rmw_service->service_name =
    reinterpret_cast<const char *>(rmw_allocate(strlen(service_name) + 1));
  if(!rmw_service->service_name) {
    RMW_SET_ERROR_MSG("create_service() failed to allocate memory for service name");
    return nullptr;
  }
  memcpy(const_cast<char *>(rmw_service->service_name), service_name, strlen(service_name) + 1);

  // Update graph
  greenstone::dds::GUID request_reader_guid;
  request_reader_guid.from_octet16(info->request_reader_->get_instance_handle().value);
  rmw_gid_t request_subscriber_gid =
    rmw_swiftdds_shared_cpp::create_rmw_gid(greenstone_swiftdds_identifier, request_reader_guid);
  greenstone::dds::GUID response_writer_guid;
  response_writer_guid.from_octet16(info->response_writer_->get_instance_handle().value);
  rmw_gid_t response_publisher_gid =
    rmw_swiftdds_shared_cpp::create_rmw_gid(greenstone_swiftdds_identifier, response_writer_guid);
  if(RMW_RET_OK !=
    common_context->add_service_graph(
         request_subscriber_gid, response_publisher_gid, node->name, node->namespace_))
  {
    return nullptr;
  }

  cleanup_rmw_service.cancel();
  cleanup_datawriter.cancel();
  cleanup_datareader.cancel();
  cleanup_info.cancel();
  return rmw_service;
}

rmw_ret_t rmw_destroy_service(rmw_node_t *node, rmw_service_t *service)
{
  RMW_CHECK_ARGUMENT_FOR_NULL(node, RMW_RET_INVALID_ARGUMENT);
  RMW_CHECK_TYPE_IDENTIFIERS_MATCH(node,
                                   node->implementation_identifier,
                                   greenstone_swiftdds_identifier,
                                   return RMW_RET_INCORRECT_RMW_IMPLEMENTATION);
  RMW_CHECK_ARGUMENT_FOR_NULL(service, RMW_RET_INVALID_ARGUMENT);
  RMW_CHECK_TYPE_IDENTIFIERS_MATCH(service,
                                   service->implementation_identifier,
                                   greenstone_swiftdds_identifier,
                                   return RMW_RET_INCORRECT_RMW_IMPLEMENTATION);

  return rmw_swiftdds_shared_cpp::__rmw_destroy_service(
      greenstone_swiftdds_identifier, node, service);
}

rmw_ret_t rmw_service_response_publisher_get_actual_qos(
  const rmw_service_t *service,
  rmw_qos_profile_t *qos)
{
  RMW_CHECK_ARGUMENT_FOR_NULL(service, RMW_RET_INVALID_ARGUMENT);
  RMW_CHECK_TYPE_IDENTIFIERS_MATCH(service,
                                   service->implementation_identifier,
                                   greenstone_swiftdds_identifier,
                                   return RMW_RET_INCORRECT_RMW_IMPLEMENTATION);
  RMW_CHECK_ARGUMENT_FOR_NULL(qos, RMW_RET_INVALID_ARGUMENT);

  return rmw_swiftdds_shared_cpp::__rmw_service_response_publisher_get_actual_qos(service, qos);
}

rmw_ret_t rmw_service_request_subscription_get_actual_qos(
  const rmw_service_t *service,
  rmw_qos_profile_t *qos)
{
  RMW_CHECK_ARGUMENT_FOR_NULL(service, RMW_RET_INVALID_ARGUMENT);
  RMW_CHECK_TYPE_IDENTIFIERS_MATCH(service,
                                   service->implementation_identifier,
                                   greenstone_swiftdds_identifier,
                                   return RMW_RET_INCORRECT_RMW_IMPLEMENTATION);
  RMW_CHECK_ARGUMENT_FOR_NULL(qos, RMW_RET_INVALID_ARGUMENT);

  return rmw_swiftdds_shared_cpp::__rmw_service_request_subscription_get_actual_qos(service, qos);
}

rmw_ret_t rmw_service_set_on_new_request_callback(
  rmw_service_t *rmw_service,
  rmw_event_callback_t callback,
  const void *user_data)
{
  RMW_CHECK_ARGUMENT_FOR_NULL(rmw_service, RMW_RET_INVALID_ARGUMENT);

  return rmw_swiftdds_shared_cpp::__rmw_service_set_on_new_request_callback(
      rmw_service, callback, user_data);
}
}  // extern "C"
