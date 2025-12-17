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

#include "rcutils/logging_macros.h"

#include "rmw/allocators.h"
#include "rmw/error_handling.h"
#include "rmw/impl/cpp/macros.hpp"
#include "rmw/rmw.h"

#include "rmw_swiftdds_shared_cpp/create_rmw_gid.hpp"
#include "rmw_swiftdds_shared_cpp/custom_participant_info.hpp"
#include "rmw_swiftdds_shared_cpp/custom_service_info.hpp"
#include "rmw_swiftdds_shared_cpp/namespace_prefix.hpp"
#include "rmw_swiftdds_shared_cpp/qos.hpp"
#include "rmw_swiftdds_shared_cpp/rmw_common.hpp"
#include "rmw_swiftdds_shared_cpp/rmw_context_impl.hpp"
#include "rmw_swiftdds_shared_cpp/TypeSupport.hpp"
#include "rmw_swiftdds_shared_cpp/utils.hpp"

#include "swiftdds/dcps/SwiftDdsExport.h"

namespace rmw_swiftdds_shared_cpp
{
rmw_ret_t __rmw_destroy_service(const char *identifier, rmw_node_t *node, rmw_service_t *service)
{
  rmw_ret_t final_ret = RMW_RET_OK;

  auto common_context = static_cast<rmw_dds_common::Context *>(node->context->impl->common);
  auto participant_info =
    static_cast<CustomParticipantInfo *>(node->context->impl->participant_info);
  auto info = static_cast<CustomServiceInfo *>(service->data);

  // Update graph
  greenstone::dds::GUID request_reader_guid;
  request_reader_guid.from_octet16(info->request_reader_->get_instance_handle().value);
  rmw_gid_t request_subscriber_gid =
    rmw_swiftdds_shared_cpp::create_rmw_gid(identifier, request_reader_guid);
  greenstone::dds::GUID response_writer_guid;
  response_writer_guid.from_octet16(info->response_writer_->get_instance_handle().value);
  rmw_gid_t response_publisher_gid =
    rmw_swiftdds_shared_cpp::create_rmw_gid(identifier, response_writer_guid);
  final_ret = common_context->remove_service_graph(
      request_subscriber_gid, response_publisher_gid, node->name, node->namespace_);

  auto show_previous_error = [&final_ret]() {
      if(RMW_RET_OK != final_ret) {
        RMW_SAFE_FWRITE_TO_STDERR(rmw_get_error_string().str);
        RMW_SAFE_FWRITE_TO_STDERR(" during '" RCUTILS_STRINGIFY(__function__) "'\n");
        rmw_reset_error();
      }
    };

  /////
  // Delete DataWriter and DataReader
  {
    std::lock_guard<std::mutex> lck(participant_info->entity_creation_mutex_);

    // Delete DataReader
    greenstone::dds::ReturnCode_t ret =
      participant_info->subscriber_->delete_datareader(info->request_reader_);
    if(ret != greenstone::dds::ReturnCode_t::RETCODE_OK) {
      show_previous_error();
      RMW_SET_ERROR_MSG("Fail in delete datareader");
      final_ret = RMW_RET_ERROR;
      info->request_reader_->set_listener(nullptr, greenstone::dds::EmptyStatusMask);
    }

    // Delete DataReader listener
    if(nullptr != info->listener_) {
      delete info->listener_;
      info->listener_ = nullptr;
    }

    // Delete DataWriter
    ret = participant_info->publisher_->delete_datawriter(info->response_writer_);
    if(ret != greenstone::dds::ReturnCode_t::RETCODE_OK) {
      show_previous_error();
      RMW_SET_ERROR_MSG("Fail in delete datawriter");
      final_ret = RMW_RET_ERROR;
      info->response_writer_->set_listener(nullptr, greenstone::dds::EmptyStatusMask);
    }

    // Delete DataWriter listener
    if(nullptr != info->pub_listener_) {
      delete info->pub_listener_;
      info->pub_listener_ = nullptr;
    }

    // Delete topics and unregister types
    remove_topic_and_type(
        participant_info, nullptr, info->request_topic_, info->request_type_support_.get());
    remove_topic_and_type(
        participant_info, nullptr, info->response_topic_, info->response_type_support_.get());

    // Delete CustomServiceInfo structure
    delete info;
  }

  rmw_free(const_cast<char *>(service->service_name));
  rmw_service_free(service);

  RCUTILS_CAN_RETURN_WITH_ERROR_OF(RMW_RET_ERROR);  // on completion
  return final_ret;
}

rmw_ret_t __rmw_service_response_publisher_get_actual_qos(
  const rmw_service_t *service,
  rmw_qos_profile_t *qos)
{
  auto srv = static_cast<CustomServiceInfo *>(service->data);
  greenstone::dds::DataWriter *swiftdds_rw = srv->response_writer_;
  greenstone::dds::DataWriterQos rw_qos;
  swiftdds_rw->get_qos(rw_qos);
  qos->lifespan = dds_duration_to_rmw(rw_qos.life_span().duration());
  dds_qos_to_rmw_qos(rw_qos, qos);
  return RMW_RET_OK;
}

rmw_ret_t __rmw_service_request_subscription_get_actual_qos(
  const rmw_service_t *service,
  rmw_qos_profile_t *qos)
{
  auto srv = static_cast<CustomServiceInfo *>(service->data);
  greenstone::dds::DataReader *swiftdds_rr = srv->request_reader_;
  greenstone::dds::DataReaderQos rr_qos;
  swiftdds_rr->get_qos(rr_qos);
  dds_qos_to_rmw_qos(rr_qos, qos);
  return RMW_RET_OK;
}

rmw_ret_t __rmw_service_set_on_new_request_callback(
  rmw_service_t *rmw_service,
  rmw_event_callback_t callback,
  const void *user_data)
{
  auto custom_service_info = static_cast<CustomServiceInfo *>(rmw_service->data);
  custom_service_info->listener_->set_on_new_request_callback(user_data, callback);
  return RMW_RET_OK;
}

}  // namespace rmw_swiftdds_shared_cpp
