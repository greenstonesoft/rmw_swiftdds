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

#ifndef RMW_SWIFTDDS_SHARED_CPP__TYPESUPPORT_HPP_
#define RMW_SWIFTDDS_SHARED_CPP__TYPESUPPORT_HPP_

#include <cassert>
#include <functional>
#include <memory>
#include <regex>
#include <string>
#include <utility>
#include <vector>
#include <iterator>

#include "rosidl_runtime_c/string_functions.h"
#include "rosidl_runtime_c/u16string_functions.h"
#include "rosidl_runtime_c/primitives_sequence_functions.h"
#include "rosidl_typesupport_introspection_c/identifier.h"
#include "rosidl_typesupport_introspection_c/message_introspection.h"
#include "rosidl_typesupport_introspection_c/service_introspection.h"
#include "rosidl_typesupport_introspection_cpp/field_types.hpp"
#include "rosidl_typesupport_introspection_cpp/identifier.hpp"
#include "rosidl_typesupport_introspection_cpp/message_introspection.hpp"
#include "rosidl_typesupport_introspection_cpp/service_introspection.hpp"
#include "rmw/types.h"

#include "rmw_swiftdds_shared_cpp/visibility_control.h"
#include "swiftdds/dcps/SwiftDdsExport.h"
#include "swiftdds/rtps/CdrSize.h"

namespace rmw_swiftdds_shared_cpp
{

enum class TypeGenerator
{
  ROSIDL_C,
  ROSIDL_Cpp,
};

template<TypeGenerator>
struct TypeGeneratorInfo;

template<>
struct TypeGeneratorInfo<TypeGenerator::ROSIDL_C>
{
  static constexpr auto enum_value = TypeGenerator::ROSIDL_C;
  static const auto & get_identifier()
  {
    return rosidl_typesupport_introspection_c__identifier;
  }
  using MetaMessage = rosidl_typesupport_introspection_c__MessageMembers;
  using MetaMember = rosidl_typesupport_introspection_c__MessageMember;
  using MetaService = rosidl_typesupport_introspection_c__ServiceMembers;
};

template<>
struct TypeGeneratorInfo<TypeGenerator::ROSIDL_Cpp>
{
  static constexpr auto enum_value = TypeGenerator::ROSIDL_Cpp;
  static const auto & get_identifier()
  {
    return rosidl_typesupport_introspection_cpp::typesupport_identifier;
  }
  using MetaMessage = rosidl_typesupport_introspection_cpp::MessageMembers;
  using MetaMember = rosidl_typesupport_introspection_cpp::MessageMember;
  using MetaService = rosidl_typesupport_introspection_cpp::ServiceMembers;
};

template<TypeGenerator g> using MetaMessage = typename TypeGeneratorInfo<g>::MetaMessage;
template<TypeGenerator g> using MetaMember = typename TypeGeneratorInfo<g>::MetaMember;
template<TypeGenerator g> using MetaService = typename TypeGeneratorInfo<g>::MetaService;

namespace tsi_enum = rosidl_typesupport_introspection_cpp;

// these are shared between c and cpp
enum class ROSIDL_TypeKind : uint8_t
{
  FLOAT = tsi_enum::ROS_TYPE_FLOAT,
  DOUBLE = tsi_enum::ROS_TYPE_DOUBLE,
  CHAR = tsi_enum::ROS_TYPE_CHAR,
  WCHAR = tsi_enum::ROS_TYPE_WCHAR,
  BOOLEAN = tsi_enum::ROS_TYPE_BOOLEAN,
  OCTET = tsi_enum::ROS_TYPE_OCTET,
  UINT8 = tsi_enum::ROS_TYPE_UINT8,
  INT8 = tsi_enum::ROS_TYPE_INT8,
  UINT16 = tsi_enum::ROS_TYPE_UINT16,
  INT16 = tsi_enum::ROS_TYPE_INT16,
  UINT32 = tsi_enum::ROS_TYPE_UINT32,
  INT32 = tsi_enum::ROS_TYPE_INT32,
  UINT64 = tsi_enum::ROS_TYPE_UINT64,
  INT64 = tsi_enum::ROS_TYPE_INT64,
  STRING = tsi_enum::ROS_TYPE_STRING,
  WSTRING = tsi_enum::ROS_TYPE_WSTRING,

  MESSAGE = tsi_enum::ROS_TYPE_MESSAGE,
};

typedef struct DdsRequestData
{
  rmw_request_id_t header;
  void *data;
} dds_request_data_t;

struct MemberBase
{
  std::string name;
  size_t member_offset{0U};
  bool is_key{false};
  bool is_plain_types{true};

  virtual ~MemberBase() = default;

  virtual DdsCdr & serialize(DdsCdr & cdr, void *data) = 0;

  virtual DdsCdr & deserialize(DdsCdr & cdr, void *data) = 0;

  virtual uint32_t max_align_size(uint32_t const _cur_al, void *data) = 0;
};

template<ROSIDL_TypeKind _kind,
  TypeGenerator g = TypeGenerator::ROSIDL_C,
  bool is_arr = false,
  bool fix_size = false>
struct TypeMap;

template<TypeGenerator g>
struct TypeMap<ROSIDL_TypeKind::FLOAT, g>
{
  using type = float;
};

template<TypeGenerator g>
struct TypeMap<ROSIDL_TypeKind::DOUBLE, g>
{
  using type = double;
};

template<TypeGenerator g>
struct TypeMap<ROSIDL_TypeKind::CHAR, g>
{
  using type = char;
};

template<TypeGenerator g>
struct TypeMap<ROSIDL_TypeKind::WCHAR, g>
{
  using type = uint16_t;
};

template<TypeGenerator g>
struct TypeMap<ROSIDL_TypeKind::BOOLEAN, g>
{
  using type = bool;
};

template<TypeGenerator g>
struct TypeMap<ROSIDL_TypeKind::OCTET, g>
{
  using type = uint8_t;
};

template<TypeGenerator g>
struct TypeMap<ROSIDL_TypeKind::UINT8, g>
{
  using type = uint8_t;
};

template<TypeGenerator g>
struct TypeMap<ROSIDL_TypeKind::INT8, g>
{
  using type = char;
};

template<TypeGenerator g>
struct TypeMap<ROSIDL_TypeKind::UINT16, g>
{
  using type = uint16_t;
};

template<TypeGenerator g>
struct TypeMap<ROSIDL_TypeKind::INT16, g>
{
  using type = int16_t;
};

template<TypeGenerator g>
struct TypeMap<ROSIDL_TypeKind::UINT32, g>
{
  using type = uint32_t;
};

template<TypeGenerator g>
struct TypeMap<ROSIDL_TypeKind::INT32, g>
{
  using type = int32_t;
};

template<TypeGenerator g>
struct TypeMap<ROSIDL_TypeKind::UINT64, g>
{
  using type = uint64_t;
};

template<TypeGenerator g>
struct TypeMap<ROSIDL_TypeKind::INT64, g>
{
  using type = int64_t;
};

template<>
struct TypeMap<ROSIDL_TypeKind::STRING, TypeGenerator::ROSIDL_Cpp>
{
  using type = std::string;
};

template<>
struct TypeMap<ROSIDL_TypeKind::WSTRING, TypeGenerator::ROSIDL_Cpp>
{
  using type = std::u16string;
};

class StructValueType
{
public:
  using InstanceHandle_t = gstone::rtps::InstanceHandle_t;

  DdsCdr & serialize(DdsCdr & cdr, void *data);

  DdsCdr & deserialize(DdsCdr & cdr, void *data);

  bool get_key(void *data, InstanceHandle_t *ihandle) noexcept;

  uint32_t max_align_size(uint32_t const _cur_al, void *data);

  inline bool is_with_key() noexcept
  {
    return m_is_with_key;
  }

  inline bool is_plain_types() noexcept
  {
    return m_is_plain_types;
  }

  inline void add_member(std::unique_ptr<MemberBase> & mem_ptr) noexcept
  {
    if(mem_ptr->is_key) {
      m_is_with_key = true;
    }
    if(mem_ptr->is_plain_types == false) {
      m_is_plain_types = false;
    }
    m_members.push_back(std::move(mem_ptr));
  }

private:
  std::vector<std::unique_ptr<MemberBase>> m_members{};
  bool m_is_with_key{false};
  bool m_is_plain_types{true};
};

class ROSIDLC_StringValueType
{
public:
  static constexpr bool IS_VARIABLE_TYPES{true};
  using type = rosidl_runtime_c__String;

  ROSIDLC_StringValueType() = default;

  DdsCdr & serialize(DdsCdr & cdr, void *data)
  {
    type *meta_data = reinterpret_cast<type *>(data);
    std::string tmp;
    if(meta_data->data != nullptr) {
      tmp = std::string{meta_data->data, meta_data->size};
    }
    cdr.serialize(tmp);

    return cdr;
  }

  DdsCdr & deserialize(DdsCdr & cdr, void *data)
  {
    std::string tmp;
    cdr.deserialize(tmp);
    type *meta_data = reinterpret_cast<type *>(data);
    if(meta_data->data != nullptr) {
      rosidl_runtime_c__String__init(meta_data);
    }
    rosidl_runtime_c__String__assign(meta_data, tmp.c_str());

    return cdr;
  }

  uint32_t max_align_size(uint32_t const _cur_al, void *data)
  {
    type *meta_data = reinterpret_cast<type *>(data);
    uint32_t cur{static_cast<uint32_t>(gstone::rtps::CdrUtil::alignment_bytes(_cur_al, 4U))};
    cur += static_cast<uint32_t>(meta_data->size + 1U);
    return cur;
  }
};

class ROSIDLC_WStringValueType
{
public:
  static constexpr bool IS_VARIABLE_TYPES{true};
  using type = rosidl_runtime_c__U16String;

  ROSIDLC_WStringValueType() = default;

  DdsCdr & serialize(DdsCdr & cdr, void *data)
  {
    type *meta_data = reinterpret_cast<type *>(data);
    std::u16string tmp;
    if(meta_data->data != nullptr) {
      tmp = std::u16string{reinterpret_cast<char16_t *>(meta_data->data), meta_data->size};
    }
    cdr.serialize(tmp);

    return cdr;
  }

  DdsCdr & deserialize(DdsCdr & cdr, void *data)
  {
    std::u16string tmp;
    cdr.deserialize(tmp);
    type *meta_data = reinterpret_cast<type *>(data);
    if(meta_data->data != nullptr) {
      rosidl_runtime_c__U16String__init(meta_data);
    }
    rosidl_runtime_c__U16String__assign(meta_data, reinterpret_cast<uint16_t const *>(tmp.c_str()));

    return cdr;
  }

  uint32_t max_align_size(uint32_t const _cur_al, void *data)
  {
    type *meta_data = reinterpret_cast<type *>(data);
    uint32_t cur{static_cast<uint32_t>(gstone::rtps::CdrUtil::alignment_bytes(_cur_al, 4U))};
    cur += static_cast<uint32_t>(meta_data->size * 2U + 2U);
    return cur;
  }
};

template<TypeGenerator g>
struct TypeMap<ROSIDL_TypeKind::MESSAGE, g>
{
  using type = StructValueType;
};

template<typename _type, typename = void>
class ArrayValueType
{
public:
  static constexpr bool IS_VARIABLE_TYPES{false};
  static constexpr bool MEM_TYPE_IS_CLASS{false};
  explicit ArrayValueType(size_t const _size)
  : m_size{_size}
  {
  }

  DdsCdr & serialize(DdsCdr & cdr, void *data)
  {
    _type *meta_data = reinterpret_cast<_type *>(data);
    if(meta_data != nullptr) {
      if constexpr(std::is_class<_type>::value) {
        for(size_t i{0U}; i < m_size; ++i) {
          cdr.serialize(meta_data[i]);
        }
      } else {
        cdr.serialize(const_cast<_type const *const>(meta_data), m_size);
      }
    }

    return cdr;
  }

  DdsCdr & deserialize(DdsCdr & cdr, void *data)
  {
    _type *meta_data = reinterpret_cast<_type *>(data);
    if(meta_data != nullptr) {
      if constexpr(std::is_class<_type>::value) {
        for(size_t i{0U}; i < m_size; ++i) {
          cdr.deserialize(meta_data[i]);
        }
      } else {
        cdr.deserialize(meta_data, m_size);
      }
    }

    return cdr;
  }

  uint32_t max_align_size(uint32_t const _cur_al, void *data)
  {
    _type *meta_data = reinterpret_cast<_type *>(data);
    uint32_t max_size{_cur_al};
    uint32_t const obj_size{static_cast<uint32_t>(m_size)};
    if constexpr(std::is_same<_type, std::string>::value ||
      std::is_same<_type, std::u16string>::value)
    {
      for(uint32_t i{0U}; i < obj_size; ++i) {
        max_size = static_cast<uint32_t>(gstone::rtps::CdrUtil::alignment(max_size, meta_data[i]));
      }
    } else {
      uint32_t const one_ele{static_cast<uint32_t>(gstone::rtps::CdrUtil::alignment(1U,
            meta_data[0U]))};
      max_size += one_ele * obj_size;
    }

    return max_size;
  }

private:
  size_t m_size{0U};
};

template<typename _type>
class ArrayValueType<
  _type,
  typename std::enable_if_t<std::is_class_v<_type>&& !std::is_same_v<_type, std::string>&&
  !std::is_same_v<_type, std::u16string>>>
{
public:
  static constexpr bool IS_VARIABLE_TYPES{false};
  static constexpr bool MEM_TYPE_IS_CLASS{true};
  ArrayValueType(
    size_t const _size,
    std::function<void *(void *, size_t)> const & _func,
    void *type_ptr)
  : m_size{_size},
    get_function{_func},
    m_type{std::unique_ptr<_type>(reinterpret_cast<_type *>(type_ptr))}
  {
  }

  DdsCdr & serialize(DdsCdr & cdr, void *data)
  {
    if(data != nullptr) {
      auto const obj_size{m_size};
      for(auto i{0U}; i < obj_size; ++i) {
        m_type->serialize(cdr, get_function(data, i));
      }
    }

    return cdr;
  }

  DdsCdr & deserialize(DdsCdr & cdr, void *data)
  {
    if(data != nullptr) {
      auto const obj_size{m_size};
      for(auto i{0U}; i < obj_size; ++i) {
        m_type->deserialize(cdr, get_function(data, i));
      }
    }

    return cdr;
  }

  uint32_t max_align_size(uint32_t const _cur_al, void *data)
  {
    auto const obj_size{m_size};
    uint32_t max_size{_cur_al};
    for(auto i{0U}; i < obj_size; ++i) {
      max_size = m_type->max_align_size(max_size, get_function(data, i));
    }
    return max_size;
  }

private:
  size_t m_size{0U};
  std::function<void *(void *, size_t)> get_function{nullptr};
  std::unique_ptr<_type> m_type{nullptr};
};

template<typename _type, typename = void>
class ROSIDLC_SpanSequenceValueType
{
public:
  static constexpr bool MEM_TYPE_IS_CLASS{false};
  ROSIDLC_SpanSequenceValueType() = default;
  explicit ROSIDLC_SpanSequenceValueType(
    std::function<size_t(void const *)> const & _size_func,
    std::function<void *(void *, size_t)> const & _get_func,
    std::function<bool(void *, size_t)> const & _resize_func,
    bool const is_upper_bound,
    size_t const bound_size)
  : size_function{_size_func},
    get_function{_get_func},
    resize_function{_resize_func},
    m_is_upper_bound{is_upper_bound},
    m_bound_size{bound_size}
  {
  }

  DdsCdr & serialize(DdsCdr & cdr, void *data)
  {
    if(data != nullptr) {
      uint32_t _size{static_cast<uint32_t>(size_function(data))};
      if(m_is_upper_bound && (_size > m_bound_size)) {
        _size = m_bound_size;
      }
      cdr.serialize(_size);
      void *ser_data = get_function(data, 0);
      if constexpr(std::is_same<_type, char>::value) {
        cdr.serialize(const_cast<char const *const>(reinterpret_cast<char *>(ser_data)), _size);
      } else {
        cdr.serialize(const_cast<_type const *const>(reinterpret_cast<_type *>(ser_data)), _size);
      }
    }

    return cdr;
  }

  DdsCdr & deserialize(DdsCdr & cdr, void *data)
  {
    uint32_t obj_size{0U};
    cdr.deserialize(obj_size);
    if(resize_function != nullptr) {
      resize_function(data, obj_size);
    }
    void *deser_data = get_function(data, 0);
    if constexpr(std::is_same<_type, char>::value) {
      cdr.deserialize(reinterpret_cast<char *>(deser_data), obj_size);
    } else {
      cdr.deserialize(reinterpret_cast<_type *>(deser_data), obj_size);
    }

    return cdr;
  }

  uint32_t max_align_size(uint32_t const _cur_al, void *data)
  {
    uint32_t cur{static_cast<uint32_t>(gstone::rtps::CdrUtil::alignment_bytes(_cur_al, 4U))};
    uint32_t const one_ele{static_cast<uint32_t>(gstone::rtps::CdrUtil::alignment(
        1U, *reinterpret_cast<_type *>(get_function(data, 0))))};
    uint32_t _size{static_cast<uint32_t>(size_function(data))};
    if(m_is_upper_bound && (_size > m_bound_size)) {
      _size = m_bound_size;
    }
    if((_size == 0U) && (sizeof(_type) == 8U)) {
      cur = static_cast<uint32_t>(gstone::rtps::CdrUtil::alignment_bytes(_cur_al, 8U));
    } else {
      cur = cur + one_ele * _size;
    }
    return cur;
  }

private:
  std::function<size_t(void const *)> size_function{nullptr};
  std::function<void *(void *, size_t)> get_function{nullptr};
  std::function<bool(void *, size_t)> resize_function{nullptr};
  bool m_is_upper_bound{false};
  size_t m_bound_size{0U};
};

template<typename _type>
class ROSIDLC_SpanSequenceValueType<_type, typename std::enable_if_t<std::is_class_v<_type>>>
{
public:
  static constexpr bool MEM_TYPE_IS_CLASS{true};
  ROSIDLC_SpanSequenceValueType() = default;
  ROSIDLC_SpanSequenceValueType(
    std::function<size_t(void const *)> const & _size_func,
    std::function<void *(void *, size_t)> const & _get_func,
    std::function<bool(void *, size_t)> const & _resize_func,
    bool const is_upper_bound,
    size_t const bound_size,
    void *type_ptr)
  : size_function{_size_func},
    get_function{_get_func},
    resize_function{_resize_func},
    m_is_upper_bound{is_upper_bound},
    m_bound_size{bound_size},
    m_type{std::unique_ptr<_type>{reinterpret_cast<_type *>(type_ptr)}}
  {
  }

  DdsCdr & serialize(DdsCdr & cdr, void *data)
  {
    if(data != nullptr) {
      uint32_t obj_size{static_cast<uint32_t>(size_function(data))};
      if(m_is_upper_bound && (obj_size > m_bound_size)) {
        obj_size = m_bound_size;
      }
      cdr.serialize(obj_size);
      for(uint32_t i{0U}; i < obj_size; ++i) {
        m_type->serialize(cdr, get_function(data, i));
      }
    }

    return cdr;
  }

  DdsCdr & deserialize(DdsCdr & cdr, void *data)
  {
    if(data != nullptr) {
      uint32_t obj_size{0U};
      cdr.deserialize(obj_size);
      if(resize_function != nullptr) {
        resize_function(data, obj_size);
      }
      for(uint32_t i{0U}; i < obj_size; ++i) {
        m_type->deserialize(cdr, get_function(data, i));
      }
    }

    return cdr;
  }

  uint32_t max_align_size(uint32_t const _cur_al, void *data)
  {
    auto obj_size{static_cast<uint32_t>(size_function(data))};
    if(m_is_upper_bound && (obj_size > m_bound_size)) {
      obj_size = m_bound_size;
    }
    uint32_t max_size{static_cast<uint32_t>(gstone::rtps::CdrUtil::alignment_bytes(_cur_al, 4U))};
    for(auto i{0U}; i < obj_size; ++i) {
      max_size = m_type->max_align_size(max_size, get_function(data, i));
    }
    return max_size;
  }

private:
  std::function<size_t(void const *)> size_function{nullptr};
  std::function<void *(void *, size_t)> get_function{nullptr};
  std::function<bool(void *, size_t)> resize_function{nullptr};
  bool m_is_upper_bound{false};
  size_t m_bound_size{0U};
  std::unique_ptr<_type> m_type{nullptr};
};

template<typename _type, typename = void>
class CallbackSpanSequenceValueType
{
public:
  static constexpr bool MEM_TYPE_IS_CLASS{false};
  CallbackSpanSequenceValueType() = default;
  CallbackSpanSequenceValueType(bool const is_upper_bound, size_t const bound_size)
  : m_is_upper_bound{is_upper_bound},
    m_bound_size{bound_size}
  {
  }

  DdsCdr & serialize(DdsCdr & cdr, void *data)
  {
    if(data != nullptr) {
      auto & obj_vec = *reinterpret_cast<std::vector<_type> *>(data);
      if(m_is_upper_bound && (obj_vec.size() > m_bound_size)) {
        obj_vec.resize(m_bound_size);
      }
      cdr.serialize(obj_vec);
    }

    return cdr;
  }

  DdsCdr & deserialize(DdsCdr & cdr, void *data)
  {
    auto & obj_vec = *reinterpret_cast<std::vector<_type> *>(data);
    cdr.deserialize(obj_vec);
    return cdr;
  }

  uint32_t max_align_size(uint32_t const _cur_al, void *data)
  {
    auto & obj_vec = *reinterpret_cast<std::vector<_type> *>(data);
    if(m_is_upper_bound && (obj_vec.size() > m_bound_size)) {
      obj_vec.resize(m_bound_size);
    }
    return static_cast<uint32_t>(gstone::rtps::CdrUtil::alignment(_cur_al, obj_vec));
  }

private:
  bool m_is_upper_bound{false};
  size_t m_bound_size{0U};
};

template<typename _type>
class CallbackSpanSequenceValueType<
  _type,
  typename std::enable_if_t<std::is_class_v<_type>&& !std::is_same_v<_type, std::string>&&
  !std::is_same_v<_type, std::u16string>>>
{
public:
  static constexpr bool MEM_TYPE_IS_CLASS{true};
  CallbackSpanSequenceValueType(
    std::function<size_t(void const *)> const & _size_func,
    std::function<void *(void *, size_t)> const & _get_func,
    std::function<void(void *, size_t)> const & _resize_func,
    bool const is_upper_bound,
    size_t const bound_size,
    void *type_ptr)
  : size_function{_size_func},
    get_function{_get_func},
    resize_function{_resize_func},
    m_is_upper_bound{is_upper_bound},
    m_bound_size{bound_size},
    m_type{std::unique_ptr<_type>{reinterpret_cast<_type *>(type_ptr)}}
  {
  }

  DdsCdr & serialize(DdsCdr & cdr, void *data)
  {
    if(data != nullptr) {
      uint32_t obj_size{static_cast<uint32_t>(size_function(data))};
      if(m_is_upper_bound && (obj_size > m_bound_size)) {
        obj_size = m_bound_size;
      }
      cdr.serialize(obj_size);
      for(uint32_t i{0U}; i < obj_size; ++i) {
        m_type->serialize(cdr, get_function(data, i));
      }
    }

    return cdr;
  }

  DdsCdr & deserialize(DdsCdr & cdr, void *data)
  {
    if(data != nullptr) {
      uint32_t obj_size{0U};
      cdr.deserialize(obj_size);
      if(resize_function != nullptr) {
        resize_function(data, obj_size);
      }
      for(uint32_t i{0U}; i < obj_size; ++i) {
        m_type->deserialize(cdr, get_function(data, i));
      }
    }

    return cdr;
  }

  uint32_t max_align_size(uint32_t const _cur_al, void *data)
  {
    uint32_t obj_size{static_cast<uint32_t>(size_function(data))};
    if(m_is_upper_bound && (obj_size > m_bound_size)) {
      obj_size = m_bound_size;
    }
    uint32_t max_size{static_cast<uint32_t>(gstone::rtps::CdrUtil::alignment_bytes(_cur_al, 4U))};
    for(uint32_t i{0U}; i < obj_size; ++i) {
      max_size = m_type->max_align_size(max_size, get_function(data, i));
    }
    return max_size;
  }

private:
  std::function<size_t(void const *)> size_function{nullptr};
  std::function<void *(void *, size_t)> get_function{nullptr};
  std::function<void(void *, size_t)> resize_function{nullptr};
  bool m_is_upper_bound{false};
  size_t m_bound_size{0U};
  std::unique_ptr<_type> m_type{nullptr};
};

template<>
struct TypeMap<ROSIDL_TypeKind::STRING, TypeGenerator::ROSIDL_C>
{
  using type = ROSIDLC_StringValueType;
};

template<>
struct TypeMap<ROSIDL_TypeKind::WSTRING, TypeGenerator::ROSIDL_C>
{
  using type = ROSIDLC_WStringValueType;
};

template<ROSIDL_TypeKind _kind, TypeGenerator g>
struct TypeMap<_kind, g, true, true>
{
  using type = ArrayValueType<typename TypeMap<_kind, g>::type>;
};

template<ROSIDL_TypeKind _kind>
struct TypeMap<_kind, TypeGenerator::ROSIDL_C, true, false>
{
  using type =
    ROSIDLC_SpanSequenceValueType<typename TypeMap<_kind, TypeGenerator::ROSIDL_C>::type>;
};

template<ROSIDL_TypeKind _kind>
struct TypeMap<_kind, TypeGenerator::ROSIDL_Cpp, true, false>
{
  using type = CallbackSpanSequenceValueType<typename TypeMap<_kind,
      TypeGenerator::ROSIDL_Cpp>::type>;
};

template<ROSIDL_TypeKind _kind, TypeGenerator g, bool is_arr, bool fix_size>
using TypeMap_t = typename TypeMap<_kind, g, is_arr, fix_size>::type;

template<ROSIDL_TypeKind _kind, TypeGenerator g, bool is_arr, bool fix_size>
struct Member : public MemberBase
{
  template<typename ... Args> void init_member_args(Args... args)
  {
    mem_type = std::make_unique<TypeMap_t<_kind, g, is_arr, fix_size>>(
        std::forward<Args>(args)...);
  }

  DdsCdr & serialize(DdsCdr & cdr, void *data) final
  {
    char *res{static_cast<char *>(data)};
    if(res == nullptr) {
      return cdr;
    }
    std::advance(res, member_offset);
    if constexpr(std::is_class<TypeMap_t<_kind, g, is_arr, fix_size>>::value &&
      !std::is_same<TypeMap_t<_kind, g, is_arr, fix_size>,
      std::string>::value &&
      !std::is_same<TypeMap_t<_kind, g, is_arr, fix_size>,
      std::u16string>::value)
    {
      mem_type->serialize(cdr, res);
    } else {
      auto & obj = *reinterpret_cast<TypeMap_t<_kind, g, is_arr, fix_size> *>(res);
      cdr.serialize(obj);
    }

    return cdr;
  }

  DdsCdr & deserialize(DdsCdr & cdr, void *data) final
  {
    char *res{static_cast<char *>(data)};
    if(res == nullptr) {
      return cdr;
    }
    std::advance(res, member_offset);
    if constexpr(std::is_class<TypeMap_t<_kind, g, is_arr, fix_size>>::value &&
      !std::is_same<TypeMap_t<_kind, g, is_arr, fix_size>,
      std::string>::value &&
      !std::is_same<TypeMap_t<_kind, g, is_arr, fix_size>,
      std::u16string>::value)
    {
      mem_type->deserialize(cdr, res);
    } else {
      auto & obj = *reinterpret_cast<TypeMap_t<_kind, g, is_arr, fix_size> *>(res);
      cdr.deserialize(obj);
    }
    return cdr;
  }

  uint32_t max_align_size(uint32_t const _cur_al, void *data) final
  {
    char *res{static_cast<char *>(data)};
    if(res == nullptr) {
      return 0U;
    }
    std::advance(res, member_offset);
    uint32_t maxSize{_cur_al};
    if constexpr(std::is_class<TypeMap_t<_kind, g, is_arr, fix_size>>::value &&
      !std::is_same<TypeMap_t<_kind, g, is_arr, fix_size>,
      std::string>::value &&
      !std::is_same<TypeMap_t<_kind, g, is_arr, fix_size>,
      std::u16string>::value)
    {
      maxSize = mem_type->max_align_size(maxSize, res);
    } else {
      auto & obj = *reinterpret_cast<TypeMap_t<_kind, g, is_arr, fix_size> *>(res);
      maxSize = static_cast<uint32_t>(gstone::rtps::CdrUtil::alignment(maxSize, obj));
    }

    return maxSize;
  }

  std::unique_ptr<TypeMap_t<_kind, g, is_arr, fix_size>> mem_type{nullptr};
};

std::shared_ptr<dds::topic::TopicDataType>
make_message_value_type(rosidl_message_type_support_t const *mts);

std::string get_message_value_type_name(rosidl_message_type_support_t const *mts);

uint32_t get_message_value_type_size(rosidl_message_type_support_t const *mts);

// pair .first is request, .second is respond
std::pair<std::shared_ptr<dds::topic::TopicDataType>, std::shared_ptr<dds::topic::TopicDataType>>
make_request_response_value_types(rosidl_service_type_support_t const *svc_ts);

// pair .first is request type name, .second is respond type name
std::pair<std::string, std::string>
get_request_response_value_type_name(rosidl_service_type_support_t const *svc_ts);

}  // namespace rmw_swiftdds_shared_cpp

#endif  // RMW_SWIFTDDS_SHARED_CPP__TYPESUPPORT_HPP_
