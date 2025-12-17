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

#include "rmw/error_handling.h"
#include "rmw/serialized_message.h"
#include "rmw/rmw.h"

#include "rmw_swiftdds_shared_cpp/TypeSupport.hpp"

extern "C" {
rmw_ret_t rmw_serialize(
  const void *ros_message,
  const rosidl_message_type_support_t *type_support,
  rmw_serialized_message_t *serialized_message)
{
  if((ros_message == nullptr) || (type_support == nullptr) || (serialized_message == nullptr)) {
    return RMW_RET_ERROR;
  }
  auto data_type = rmw_swiftdds_shared_cpp::make_message_value_type(type_support);
  uint32_t const payload_size{data_type->get_cdr_serialized_size(
      const_cast<void *>(ros_message))};

  if(serialized_message->buffer_capacity < payload_size) {
    if(rmw_serialized_message_resize(serialized_message, payload_size) != RMW_RET_OK) {
      rmw_reset_error();
      RMW_SET_ERROR_MSG("unable to dynamically resize serialized message");
      return RMW_RET_ERROR;
    }
  }
  greenstone::dds::DdsCdr cdr{};
  cdr.init(data_type->get_serialized_payload_header(), serialized_message->buffer, payload_size);
  auto data_value{std::make_shared<greenstone::dds::SerializedPayload_t>(false)};
  if(!(data_type->serialize(cdr, const_cast<void *>(ros_message), data_value))) {
    rmw_reset_error();
    RMW_SET_ERROR_MSG("data type serialize error");
    return RMW_RET_ERROR;
  }
  serialized_message->buffer_length = data_value->length();

  return RMW_RET_OK;
}

rmw_ret_t rmw_deserialize(
  const rmw_serialized_message_t *serialized_message,
  const rosidl_message_type_support_t *type_support,
  void *ros_message)
{
  if((serialized_message == nullptr) || (type_support == nullptr) || (ros_message == nullptr)) {
    return RMW_RET_ERROR;
  }
  auto payload{std::make_shared<greenstone::dds::SerializedPayload_t>(false)};
  payload->value(reinterpret_cast<octet *>(serialized_message->buffer));
  payload->length(serialized_message->buffer_length);
  auto data_type = rmw_swiftdds_shared_cpp::make_message_value_type(type_support);
  greenstone::dds::DdsCdr cdr{};
  if(!(data_type->deserialize(cdr, payload, ros_message))) {
    rmw_reset_error();
    RMW_SET_ERROR_MSG("data type deserialize error");
    return RMW_RET_ERROR;
  }

  return RMW_RET_OK;
}

rmw_ret_t
rmw_get_serialized_message_size(
  const rosidl_message_type_support_t * /*type_support*/,
  const rosidl_runtime_c__Sequence__bound * /*message_bounds*/,
  size_t * /*size*/)
{
  RMW_SET_ERROR_MSG("unimplemented");
  return RMW_RET_UNSUPPORTED;
}
}  // extern "C"
