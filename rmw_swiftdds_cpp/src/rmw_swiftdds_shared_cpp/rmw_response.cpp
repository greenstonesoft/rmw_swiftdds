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

#include <cassert>

#include "rmw/error_handling.h"
#include "rmw/rmw.h"
#include "rmw/impl/cpp/macros.hpp"

#include "rmw_swiftdds_shared_cpp/custom_client_info.hpp"
#include "rmw_swiftdds_shared_cpp/custom_service_info.hpp"
#include "rmw_swiftdds_shared_cpp/guid_utils.hpp"
#include "rmw_swiftdds_shared_cpp/rmw_common.hpp"
#include "rmw_swiftdds_shared_cpp/TypeSupport.hpp"

#include "tracetools/tracetools.h"

#include "swiftdds/dcps/SwiftDdsExport.h"

namespace rmw_swiftdds_shared_cpp
{
rmw_ret_t __rmw_take_response(
  const char *identifier,
  const rmw_client_t *client,
  rmw_service_info_t *request_header,
  void *ros_response,
  bool *taken)
{
  RMW_CHECK_ARGUMENT_FOR_NULL(client, RMW_RET_INVALID_ARGUMENT);
  RMW_CHECK_TYPE_IDENTIFIERS_MATCH(client,
                                   client->implementation_identifier,
                                   identifier,
                                   return RMW_RET_INCORRECT_RMW_IMPLEMENTATION);
  RMW_CHECK_ARGUMENT_FOR_NULL(request_header, RMW_RET_INVALID_ARGUMENT);
  RMW_CHECK_ARGUMENT_FOR_NULL(ros_response, RMW_RET_INVALID_ARGUMENT);
  RMW_CHECK_ARGUMENT_FOR_NULL(taken, RMW_RET_INVALID_ARGUMENT);

  *taken = false;

  auto info = static_cast<CustomClientInfo *>(client->data);
  assert(info);

  dds_request_data_t data;
  data.data = ros_response;
  dds::core::SampleInfo sample_info;

  if(greenstone::dds::ReturnCode_t::RETCODE_OK ==
    info->response_reader_->take_first_sample(&data, sample_info))
  {
    if(sample_info.valid_data) {
      request_header->source_timestamp = sample_info.source_timestamp.to_nanosecond();
      request_header->received_timestamp = sample_info.reception_timestamp.to_nanosecond();
      request_header->request_id.sequence_number = data.header.sequence_number;
      memcpy(request_header->request_id.writer_guid, data.header.writer_guid, RMW_GID_STORAGE_SIZE);
      *taken = true;
    }
  }

  TRACETOOLS_TRACEPOINT(rmw_take_response,
                        static_cast<const void *>(client),
                        static_cast<const void *>(ros_response),
                        request_header->request_id.sequence_number,
                        request_header->source_timestamp,
                        *taken);
  return RMW_RET_OK;
}

rmw_ret_t __rmw_send_response(
  const char *identifier,
  const rmw_service_t *service,
  rmw_request_id_t *request_header,
  void *ros_response)
{
  RMW_CHECK_ARGUMENT_FOR_NULL(service, RMW_RET_INVALID_ARGUMENT);
  RMW_CHECK_TYPE_IDENTIFIERS_MATCH(service,
                                   service->implementation_identifier,
                                   identifier,
                                   return RMW_RET_INCORRECT_RMW_IMPLEMENTATION);
  RMW_CHECK_ARGUMENT_FOR_NULL(request_header, RMW_RET_INVALID_ARGUMENT);
  RMW_CHECK_ARGUMENT_FOR_NULL(ros_response, RMW_RET_INVALID_ARGUMENT);

  rmw_ret_t returnedValue = RMW_RET_ERROR;

  auto info = static_cast<CustomServiceInfo *>(service->data);
  assert(info);

  dds_request_data_t data;
  memcpy(data.header.writer_guid, request_header->writer_guid, RMW_GID_STORAGE_SIZE);
  data.header.sequence_number = request_header->sequence_number;
  data.data = ros_response;
  greenstone::dds::Time_t timestamp;
  info->response_writer_->get_publisher()->get_participant()->get_current_time(timestamp);
  TRACETOOLS_TRACEPOINT(rmw_send_response,
                        static_cast<const void *>(service),
                        static_cast<const void *>(ros_response),
                        request_header->writer_guid,
                        request_header->sequence_number,
                        timestamp.to_nanosecond());
  if(greenstone::dds::ReturnCode_t::RETCODE_OK ==
    info->response_writer_->write_w_timestamp(&data, DDS::HANDLE_NIL, timestamp))
  {
    returnedValue = RMW_RET_OK;
  } else {
    RMW_SET_ERROR_MSG("cannot publish data");
  }

  return returnedValue;
}

}  // namespace rmw_swiftdds_shared_cpp
