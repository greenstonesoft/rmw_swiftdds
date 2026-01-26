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
#include "rmw/types.h"

#include "rmw_swiftdds_shared_cpp/custom_client_info.hpp"
#include "rmw_swiftdds_shared_cpp/custom_service_info.hpp"
#include "rmw_swiftdds_shared_cpp/guid_utils.hpp"
#include "rmw_swiftdds_shared_cpp/rmw_common.hpp"
#include "rmw_swiftdds_shared_cpp/TypeSupport.hpp"

#include "tracetools/tracetools.h"

#include "swiftdds/dcps/SwiftDdsExport.h"

namespace rmw_swiftdds_shared_cpp
{
rmw_ret_t __rmw_send_request(
  const char *identifier,
  const rmw_client_t *client,
  const void *ros_request,
  int64_t *sequence_id)
{
  static std::atomic_int64_t all_sequence_id = 1;
  RMW_CHECK_ARGUMENT_FOR_NULL(client, RMW_RET_INVALID_ARGUMENT);
  RMW_CHECK_TYPE_IDENTIFIERS_MATCH(client,
                                   client->implementation_identifier,
                                   identifier,
                                   return RMW_RET_INCORRECT_RMW_IMPLEMENTATION);
  RMW_CHECK_ARGUMENT_FOR_NULL(ros_request, RMW_RET_INVALID_ARGUMENT);
  RMW_CHECK_ARGUMENT_FOR_NULL(sequence_id, RMW_RET_INVALID_ARGUMENT);

  rmw_ret_t returnedValue = RMW_RET_ERROR;
  *sequence_id = all_sequence_id;
  all_sequence_id++;

  auto info = static_cast<CustomClientInfo *>(client->data);
  assert(info);
  dds_request_data_t data;
  memcpy(data.header.writer_guid,
         info->response_reader_->get_instance_handle().value,
         RMW_GID_STORAGE_SIZE);
  data.header.sequence_number = *sequence_id;
  data.data = const_cast<void *>(ros_request);
  if(greenstone::dds::ReturnCode_t::RETCODE_OK ==
    info->request_writer_->write(&data, DDS::HANDLE_NIL))
  {
    returnedValue = RMW_RET_OK;
    // This would ideally go before the write() call, but we can only get the
    // sequence number after
    TRACETOOLS_TRACEPOINT(rmw_send_request,
                          static_cast<const void *>(client),
                          static_cast<const void *>(ros_request),
                          *sequence_id);
  } else {
    RMW_SET_ERROR_MSG("cannot publish data");
  }

  return returnedValue;
}

rmw_ret_t __rmw_take_request(
  const char *identifier,
  const rmw_service_t *service,
  rmw_service_info_t *request_header,
  void *ros_request,
  bool *taken)
{
  RMW_CHECK_ARGUMENT_FOR_NULL(service, RMW_RET_INVALID_ARGUMENT);
  RMW_CHECK_TYPE_IDENTIFIERS_MATCH(service,
                                   service->implementation_identifier,
                                   identifier,
                                   return RMW_RET_INCORRECT_RMW_IMPLEMENTATION);
  RMW_CHECK_ARGUMENT_FOR_NULL(request_header, RMW_RET_INVALID_ARGUMENT);
  RMW_CHECK_ARGUMENT_FOR_NULL(ros_request, RMW_RET_INVALID_ARGUMENT);
  RMW_CHECK_ARGUMENT_FOR_NULL(taken, RMW_RET_INVALID_ARGUMENT);

  *taken = false;

  auto info = static_cast<CustomServiceInfo *>(service->data);
  assert(info);

  dds_request_data_t data;
  data.data = ros_request;
  dds::core::SampleInfo sample_info;

  if(greenstone::dds::ReturnCode_t::RETCODE_OK ==
    info->request_reader_->take_first_sample(&data, sample_info))
  {
    if(sample_info.valid_data) {
      memcpy(request_header->request_id.writer_guid, data.header.writer_guid, RMW_GID_STORAGE_SIZE);
      request_header->request_id.sequence_number = data.header.sequence_number;
      request_header->source_timestamp = sample_info.source_timestamp.to_nanosecond();
      request_header->received_timestamp = sample_info.reception_timestamp.to_nanosecond();
      *taken = true;
    }
  }

  TRACETOOLS_TRACEPOINT(rmw_take_request,
                        static_cast<const void *>(service),
                        static_cast<const void *>(ros_request),
                        request_header->request_id.writer_guid,
                        request_header->request_id.sequence_number,
                        *taken);
  return RMW_RET_OK;
}

}  // namespace rmw_swiftdds_shared_cpp
