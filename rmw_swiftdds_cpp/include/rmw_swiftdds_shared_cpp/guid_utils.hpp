// Copyright 2020 Open Source Robotics Foundation, Inc.
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

#ifndef RMW_SWIFTDDS_SHARED_CPP__GUID_UTILS_HPP_
#define RMW_SWIFTDDS_SHARED_CPP__GUID_UTILS_HPP_

#include <cassert>
#include <cstddef>
#include <cstring>
#include <type_traits>
#include "swiftdds/dcps/SwiftDdsExport.h"

namespace rmw_swiftdds_shared_cpp
{

template<typename ByteT>
void copy_from_byte_array_to_dds_guid(const ByteT *guid_byte_array, greenstone::dds::GUID *guid)
{
  static_assert(std::is_same<uint8_t, ByteT>::value || std::is_same<int8_t, ByteT>::value,
                "ByteT should be either int8_t or uint8_t");
  assert(guid_byte_array);
  assert(guid);
  guid->from_octet16(guid_byte_array);
}

template<typename ByteT>
void copy_from_dds_guid_to_byte_array(const greenstone::dds::GUID & guid, ByteT *guid_byte_array)
{
  static_assert(std::is_same<uint8_t, ByteT>::value || std::is_same<int8_t, ByteT>::value,
                "ByteT should be either int8_t or uint8_t");
  assert(guid_byte_array);
  guid.to_octet16(guid_byte_array);
}

struct hash_swiftdds_guid
{
  std::size_t operator()(const greenstone::dds::GUID & guid) const
  {
    union u_convert{
      uint8_t plain_value[sizeof(guid)];
      uint32_t plain_ints[sizeof(guid) / sizeof(uint32_t)];
    } u{};

    static_assert(sizeof(guid) == 16 && sizeof(u.plain_value) == sizeof(u.plain_ints) &&
                      offsetof(u_convert, plain_value) == offsetof(u_convert, plain_ints),
                  "Plain guid should be easily convertible to uint32_t[4]");

    copy_from_dds_guid_to_byte_array(guid, u.plain_value);

    constexpr std::size_t prime_1 = 7;
    constexpr std::size_t prime_2 = 31;
    constexpr std::size_t prime_3 = 59;

    size_t ret_val = prime_1 * u.plain_ints[0];
    ret_val = prime_2 * (u.plain_ints[1] + ret_val);
    ret_val = prime_3 * (u.plain_ints[2] + ret_val);
    ret_val = u.plain_ints[3] + ret_val;

    return ret_val;
  }
};

}  // namespace rmw_swiftdds_shared_cpp

#endif  // RMW_SWIFTDDS_SHARED_CPP__GUID_UTILS_HPP_
