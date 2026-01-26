// Copyright GREENSTONE TECHNOLOGY CO.,LTD. 2025-2030
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

#include "rmw_swiftdds_shared_cpp/TypeSupport.hpp"

#include "rcutils/error_handling.h"

namespace rmw_swiftdds_shared_cpp
{

namespace
{

class MessageTopicDataType : public dds::topic::TopicDataType
{
public:
  using InstanceHandle_t = gstone::rtps::InstanceHandle_t;

  MessageTopicDataType() = default;
  ~MessageTopicDataType() = default;

  bool serialize(
    DdsCdr & cdr,
    void *data,
    std::shared_ptr<greenstone::dds::SerializedPayload_t> data_value) final
  {
    if((data == nullptr) || (data_value == nullptr)) {
      return false;
    }
    m_members->serialize(cdr, data);
    void *addr{nullptr};
    data_value->length(cdr.get_buf(&addr));
    data_value->value(static_cast<octet *>(addr));

    return true;
  }

  bool deserialize(
    DdsCdr & cdr,
    std::shared_ptr<greenstone::dds::SerializedPayload_t> data_value,
    void *data) final
  {
    if((data_value == nullptr) || (data == nullptr)) {
      return false;
    }
    cdr.set_buf(reinterpret_cast<void *>(data_value->value()), data_value->length());
    m_members->deserialize(cdr, data);

    return true;
  }

  // The func of getKey is non-thread-safe
  bool get_key(void *data, InstanceHandle_t *ihandle) noexcept final
  {
    if((data == nullptr) || (ihandle == nullptr)) {
      return false;
    }
    if(!!m_members->is_with_key()) {
      return false;
    }
    m_members->get_key(data, ihandle);

    return true;
  }

  bool get_key(
    std::shared_ptr<greenstone::dds::SerializedPayload_t> data_value,
    InstanceHandle_t *ihandle) noexcept final
  {
    if((data_value == nullptr) || (ihandle == nullptr)) {
      return false;
    }
    if(!m_members->is_with_key()) {
      return false;
    }
    void *data = malloc(m_struct_size);
    if(m_init_func != nullptr) {
      m_init_func(data);
    }
    DdsCdr cdr;
    cdr.set_buf(reinterpret_cast<void *>(data_value->value()), data_value->length());
    m_members->deserialize(cdr, data);
    m_members->get_key(reinterpret_cast<void *>(data), ihandle);

    if(m_fini_func != nullptr) {
      m_fini_func(data);
    }
    free(data);

    return true;
  }

  bool init_data_ptr(void *data) noexcept
  {
    if(data == nullptr) {
      return false;
    }
    if(m_init_func != nullptr) {
      m_init_func(data);
    }
    return true;
  }

  uint32_t get_cdr_serialized_size(void *data) noexcept final
  {
    if(data == nullptr) {
      return 0U;
    }
    uint32_t max_size{m_members->max_align_size(4U, data)};

    return static_cast<uint32_t>(gstone::rtps::CdrUtil::alignment_bytes(max_size, 4U));
  }

  bool is_with_key() noexcept final
  {
    return m_members->is_with_key();
  }

  bool is_plain_types() noexcept final
  {
    return m_members->is_plain_types();
  }

  void * create_data_resource() noexcept final
  {
    void *pdata = malloc(m_struct_size);
    if(m_init_func != nullptr) {
      m_init_func(pdata);
    }
    return pdata;
  }

  void release_data_resource(void *data) noexcept final
  {
    if(m_fini_func != nullptr) {
      m_fini_func(data);
    }
    free(data);
  }

  greenstone::dds::SerializedPayloadHeader const get_serialized_payload_header() noexcept final
  {
    static greenstone::dds::SerializedPayloadHeader const header{
      {0x00, 0x01},
      {0x00, 0x00}
    };  // PLAIN_CDR, LITTLE_ENDIAN
    return header;
  }

  uint32_t data_size_of() noexcept final
  {
    return m_struct_size;
  }

  inline void set_members(std::unique_ptr<StructValueType> & mems) noexcept
  {
    m_members = std::move(mems);
  }

  inline void set_struct_size(uint32_t const _size) noexcept
  {
    m_struct_size = _size;
  }

  inline void set_proc_func(
    std::function<void(void *)> const & init_func,
    std::function<void(void *)> const & fini_func) noexcept
  {
    m_init_func = init_func;
    m_fini_func = fini_func;
  }

private:
  std::unique_ptr<StructValueType> m_members{nullptr};
  uint32_t m_struct_size{0U};
  std::function<void(void *)> m_init_func{nullptr};
  std::function<void(void *)> m_fini_func{nullptr};
};

class ServiceTopicDataType : public dds::topic::TopicDataType
{
public:
  using InstanceHandle_t = gstone::rtps::InstanceHandle_t;

  ServiceTopicDataType() = default;
  ~ServiceTopicDataType() = default;

  bool serialize(
    DdsCdr & cdr,
    void *data,
    std::shared_ptr<greenstone::dds::SerializedPayload_t> data_value) final
  {
    if((data == nullptr) || (data_value == nullptr)) {
      return false;
    }
    auto srv_data = reinterpret_cast<dds_request_data_t *>(data);
    std::array<uint8_t, RMW_GID_STORAGE_SIZE> std_arr;
    std::copy(std::begin(srv_data->header.writer_guid),
              std::end(srv_data->header.writer_guid),
              std_arr.begin());
    cdr.serialize(std_arr);
    cdr.serialize(srv_data->header.sequence_number);

    m_members->serialize(cdr, srv_data->data);

    void *addr{nullptr};
    data_value->length(cdr.get_buf(&addr));
    data_value->value(static_cast<octet *>(addr));

    return true;
  }

  bool deserialize(
    DdsCdr & cdr,
    std::shared_ptr<greenstone::dds::SerializedPayload_t> data_value,
    void *data) final
  {
    if((data_value == nullptr) || (data == nullptr)) {
      return false;
    }
    cdr.set_buf(reinterpret_cast<void *>(data_value->value()), data_value->length());
    auto srv_data = reinterpret_cast<dds_request_data_t *>(data);
    cdr.deserialize(srv_data->header.writer_guid, RMW_GID_STORAGE_SIZE);
    cdr.deserialize(srv_data->header.sequence_number);

    m_members->deserialize(cdr, srv_data->data);

    return true;
  }

  // The func of getKey is non-thread-safe
  bool get_key(void *data, InstanceHandle_t *ihandle) noexcept final
  {
    if((data == nullptr) || (ihandle == nullptr)) {
      return false;
    }
    if(!!m_members->is_with_key()) {
      return false;
    }
    auto srv_data = reinterpret_cast<dds_request_data_t *>(data);
    m_members->get_key(srv_data->data, ihandle);

    return true;
  }

  bool get_key(
    std::shared_ptr<greenstone::dds::SerializedPayload_t> data_value,
    InstanceHandle_t *ihandle) noexcept final
  {
    if((data_value == nullptr) || (ihandle == nullptr)) {
      return false;
    }
    if(!m_members->is_with_key()) {
      return false;
    }
    dds_request_data_t srv_data{};
    srv_data.data = malloc(m_struct_size);
    if(m_init_func != nullptr) {
      m_init_func(srv_data.data);
    }
    DdsCdr cdr;
    cdr.set_buf(reinterpret_cast<void *>(data_value->value()), data_value->length());

    m_members->deserialize(cdr, srv_data.data);
    m_members->get_key(srv_data.data, ihandle);

    if(m_fini_func != nullptr) {
      m_fini_func(srv_data.data);
    }
    free(srv_data.data);

    return true;
  }

  bool init_data_ptr(void *data) noexcept
  {
    if(data == nullptr) {
      return false;
    }
    if(m_init_func != nullptr) {
      auto srv_data = reinterpret_cast<dds_request_data_t *>(data);
      m_init_func(srv_data->data);
    }
    return true;
  }

  uint32_t get_cdr_serialized_size(void *data) noexcept final
  {
    if(data == nullptr) {
      return 0U;
    }
    auto srv_data = reinterpret_cast<dds_request_data_t *>(data);
    std::array<uint8_t, RMW_GID_STORAGE_SIZE> std_arr;
    std::copy(std::begin(srv_data->header.writer_guid),
              std::end(srv_data->header.writer_guid),
              std_arr.begin());
    uint32_t max_size{static_cast<uint32_t>(gstone::rtps::CdrUtil::alignment(4U, std_arr))};
    max_size = gstone::rtps::CdrUtil::alignment(max_size, srv_data->header.sequence_number);

    max_size = m_members->max_align_size(max_size, srv_data->data);

    return static_cast<uint32_t>(gstone::rtps::CdrUtil::alignment_bytes(max_size, 4U));
  }

  bool is_with_key() noexcept final
  {
    return m_members->is_with_key();
  }

  bool is_plain_types() noexcept final
  {
    return false;
  }

  void * create_data_resource() noexcept final
  {
    void *pdata = malloc(m_struct_size);
    if(m_init_func != nullptr) {
      m_init_func(pdata);
    }
    return pdata;
  }

  void release_data_resource(void *data) noexcept final
  {
    if(m_fini_func != nullptr) {
      m_fini_func(data);
    }
    free(data);
  }

  greenstone::dds::SerializedPayloadHeader const get_serialized_payload_header() noexcept final
  {
    static greenstone::dds::SerializedPayloadHeader const header{
      {0x00, 0x01},
      {0x00, 0x00}
    };  // PLAIN_CDR, LITTLE_ENDIAN
    return header;
  }

  uint32_t data_size_of() noexcept final
  {
    return 0U;
  }

  inline void set_members(std::unique_ptr<StructValueType> & mems) noexcept
  {
    m_members = std::move(mems);
  }

  inline void set_struct_size(uint32_t const _size) noexcept
  {
    m_struct_size = _size;
  }

  inline void set_proc_func(
    std::function<void(void *)> const & init_func,
    std::function<void(void *)> const & fini_func) noexcept
  {
    m_init_func = init_func;
    m_fini_func = fini_func;
  }

private:
  std::unique_ptr<StructValueType> m_members{nullptr};
  uint32_t m_struct_size{0U};
  std::function<void(void *)> m_init_func{nullptr};
  std::function<void(void *)> m_fini_func{nullptr};
};

template<ROSIDL_TypeKind _kind, TypeGenerator g>
inline std::unique_ptr<MemberBase> factory_create_type_member(
  bool const is_arr,
  bool const fix_size,
  bool const is_upper_bound,
  MetaMember<g> *mem_info,
  void *mem_type_ptr = nullptr)
{
  std::unique_ptr<MemberBase> mem_ptr_base{nullptr};
  if(is_arr) {
    if(fix_size && !is_upper_bound) {
      auto mem_ptr = std::make_unique<Member<_kind, g, true, true>>();
      if constexpr(TypeMap_t<_kind, g, true, true>::MEM_TYPE_IS_CLASS) {
        mem_ptr->init_member_args(mem_info->array_size_, mem_info->get_function, mem_type_ptr);
      } else {
        mem_ptr->init_member_args(mem_info->array_size_);
      }
      if constexpr(TypeMap_t<_kind, g, true, true>::IS_VARIABLE_TYPES) {
        mem_ptr->is_plain_types = false;
      }
      mem_ptr_base = std::move(mem_ptr);
    } else {
      if constexpr(g == TypeGenerator::ROSIDL_Cpp) {
        auto mem_ptr = std::make_unique<Member<_kind, g, true, false>>();
        if constexpr(TypeMap_t<_kind, g, true, false>::MEM_TYPE_IS_CLASS) {
          mem_ptr->init_member_args(mem_info->size_function,
                                    mem_info->get_function,
                                    mem_info->resize_function,
                                    is_upper_bound,
                                    mem_info->array_size_,
                                    mem_type_ptr);
        } else {
          mem_ptr->init_member_args(is_upper_bound, mem_info->array_size_);
        }
        mem_ptr_base = std::move(mem_ptr);
      }

      if constexpr(g == TypeGenerator::ROSIDL_C) {
        auto mem_ptr = std::make_unique<Member<_kind, g, true, false>>();
        if constexpr(TypeMap_t<_kind, g, true, false>::MEM_TYPE_IS_CLASS) {
          mem_ptr->init_member_args(mem_info->size_function,
                                    mem_info->get_function,
                                    mem_info->resize_function,
                                    is_upper_bound,
                                    mem_info->array_size_,
                                    mem_type_ptr);
        } else {
          mem_ptr->init_member_args(mem_info->size_function,
                                    mem_info->get_function,
                                    mem_info->resize_function,
                                    is_upper_bound,
                                    mem_info->array_size_);
        }
        mem_ptr_base = std::move(mem_ptr);
      }

      mem_ptr_base->is_plain_types = false;
    }
  } else {
    auto mem_ptr = std::make_unique<Member<_kind, g, false, false>>();
    mem_ptr->mem_type = std::unique_ptr<TypeMap_t<_kind, g, false, false>>(
        reinterpret_cast<TypeMap_t<_kind, g, false, false> *>(mem_type_ptr));
    mem_ptr_base = std::move(mem_ptr);
  }

  return mem_ptr_base;
}

template<>
inline std::unique_ptr<MemberBase>
factory_create_type_member<ROSIDL_TypeKind::STRING, TypeGenerator::ROSIDL_C>(
  bool const is_arr,
  bool const fix_size,
  bool const is_upper_bound,
  MetaMember<TypeGenerator::ROSIDL_C> *mem_info,
  void *mem_type_ptr)
{
  std::unique_ptr<MemberBase> mem_ptr_base{nullptr};
  if(is_arr) {
    if(fix_size && !is_upper_bound) {
      auto mem_ptr = std::make_unique<
        Member<ROSIDL_TypeKind::STRING, TypeGenerator::ROSIDL_C, true, true>>();
      mem_ptr->init_member_args(
          mem_info->array_size_, mem_info->get_function, new ROSIDLC_StringValueType());
      mem_ptr_base = std::move(mem_ptr);
    } else {
      {
        auto mem_ptr = std::make_unique<
          Member<ROSIDL_TypeKind::STRING, TypeGenerator::ROSIDL_C, true, false>>();
        mem_ptr->init_member_args(mem_info->size_function,
                                  mem_info->get_function,
                                  mem_info->resize_function,
                                  is_upper_bound,
                                  mem_info->array_size_,
                                  new ROSIDLC_StringValueType());
        mem_ptr_base = std::move(mem_ptr);
      }
    }
  } else {
    auto mem_ptr = std::make_unique<
      Member<ROSIDL_TypeKind::STRING, TypeGenerator::ROSIDL_C, false, false>>();
    mem_ptr->mem_type = std::make_unique<ROSIDLC_StringValueType>();
    mem_ptr_base = std::move(mem_ptr);
  }
  return mem_ptr_base;
}

template<>
inline std::unique_ptr<MemberBase>
factory_create_type_member<ROSIDL_TypeKind::WSTRING, TypeGenerator::ROSIDL_C>(
  bool const is_arr,
  bool const fix_size,
  bool const is_upper_bound,
  MetaMember<TypeGenerator::ROSIDL_C> *mem_info,
  void *mem_type_ptr)
{
  std::unique_ptr<MemberBase> mem_ptr_base{nullptr};
  if(is_arr) {
    if(fix_size && !is_upper_bound) {
      auto mem_ptr = std::make_unique<
        Member<ROSIDL_TypeKind::WSTRING, TypeGenerator::ROSIDL_C, true, true>>();
      mem_ptr->init_member_args(
          mem_info->array_size_, mem_info->get_function, new ROSIDLC_WStringValueType());
      mem_ptr_base = std::move(mem_ptr);
    } else {
      {
        auto mem_ptr = std::make_unique<
          Member<ROSIDL_TypeKind::WSTRING, TypeGenerator::ROSIDL_C, true, false>>();
        mem_ptr->init_member_args(mem_info->size_function,
                                  mem_info->get_function,
                                  mem_info->resize_function,
                                  is_upper_bound,
                                  mem_info->array_size_,
                                  new ROSIDLC_WStringValueType());
        mem_ptr_base = std::move(mem_ptr);
      }
    }
  } else {
    auto mem_ptr = std::make_unique<
      Member<ROSIDL_TypeKind::WSTRING, TypeGenerator::ROSIDL_C, false, false>>();
    mem_ptr->mem_type = std::make_unique<ROSIDLC_WStringValueType>();
    mem_ptr_base = std::move(mem_ptr);
  }

  return mem_ptr_base;
}

template<TypeGenerator g>
void ROSIDL_InitMessageDataTypeMembers(
  MetaMessage<g> const *impl,
  std::unique_ptr<StructValueType> & _type)
{
  if((impl != nullptr) && (_type != nullptr)) {
    for(uint32_t idx{0U}; idx < impl->member_count_; ++idx) {
      auto member_impl = impl->members_[idx];
      bool const is_arr{member_impl.is_array_};
      bool const fix_size{is_arr && (member_impl.array_size_ > 0U)};
      bool is_upper_bound{member_impl.is_upper_bound_};

      ROSIDL_TypeKind const _kind{member_impl.type_id_};
      std::unique_ptr<MemberBase> mem_ptr{nullptr};
      switch(_kind) {
        case ROSIDL_TypeKind::FLOAT:
          mem_ptr = factory_create_type_member<ROSIDL_TypeKind::FLOAT, g>(
            is_arr, fix_size, is_upper_bound, &member_impl);
          break;
        case ROSIDL_TypeKind::DOUBLE:
          mem_ptr = factory_create_type_member<ROSIDL_TypeKind::DOUBLE, g>(
            is_arr, fix_size, is_upper_bound, &member_impl);
          break;
        case ROSIDL_TypeKind::CHAR:
          mem_ptr = factory_create_type_member<ROSIDL_TypeKind::CHAR, g>(
            is_arr, fix_size, is_upper_bound, &member_impl);
          break;
        case ROSIDL_TypeKind::WCHAR:
          mem_ptr = factory_create_type_member<ROSIDL_TypeKind::WCHAR, g>(
            is_arr, fix_size, is_upper_bound, &member_impl);
          break;
        case ROSIDL_TypeKind::BOOLEAN:
          mem_ptr = factory_create_type_member<ROSIDL_TypeKind::BOOLEAN, g>(
            is_arr, fix_size, is_upper_bound, &member_impl);
          break;
        case ROSIDL_TypeKind::OCTET:
          mem_ptr = factory_create_type_member<ROSIDL_TypeKind::OCTET, g>(
            is_arr, fix_size, is_upper_bound, &member_impl);
          break;
        case ROSIDL_TypeKind::UINT8:
          mem_ptr = factory_create_type_member<ROSIDL_TypeKind::UINT8, g>(
            is_arr, fix_size, is_upper_bound, &member_impl);
          break;
        case ROSIDL_TypeKind::INT8:
          mem_ptr = factory_create_type_member<ROSIDL_TypeKind::INT8, g>(
            is_arr, fix_size, is_upper_bound, &member_impl);
          break;
        case ROSIDL_TypeKind::UINT16:
          mem_ptr = factory_create_type_member<ROSIDL_TypeKind::UINT16, g>(
            is_arr, fix_size, is_upper_bound, &member_impl);
          break;
        case ROSIDL_TypeKind::INT16:
          mem_ptr = factory_create_type_member<ROSIDL_TypeKind::INT16, g>(
            is_arr, fix_size, is_upper_bound, &member_impl);
          break;
        case ROSIDL_TypeKind::UINT32:
          mem_ptr = factory_create_type_member<ROSIDL_TypeKind::UINT32, g>(
            is_arr, fix_size, is_upper_bound, &member_impl);
          break;
        case ROSIDL_TypeKind::INT32:
          mem_ptr = factory_create_type_member<ROSIDL_TypeKind::INT32, g>(
            is_arr, fix_size, is_upper_bound, &member_impl);
          break;
        case ROSIDL_TypeKind::UINT64:
          mem_ptr = factory_create_type_member<ROSIDL_TypeKind::UINT64, g>(
            is_arr, fix_size, is_upper_bound, &member_impl);
          break;
        case ROSIDL_TypeKind::INT64:
          mem_ptr = factory_create_type_member<ROSIDL_TypeKind::INT64, g>(
            is_arr, fix_size, is_upper_bound, &member_impl);
          break;
        case ROSIDL_TypeKind::STRING:
          mem_ptr = factory_create_type_member<ROSIDL_TypeKind::STRING, g>(
            is_arr, fix_size, is_upper_bound, &member_impl);
          if(mem_ptr != nullptr) {
            mem_ptr->is_plain_types = false;
          }
          break;
        case ROSIDL_TypeKind::WSTRING:
          mem_ptr = factory_create_type_member<ROSIDL_TypeKind::WSTRING, g>(
            is_arr, fix_size, is_upper_bound, &member_impl);
          if(mem_ptr != nullptr) {
            mem_ptr->is_plain_types = false;
          }
          break;
        case ROSIDL_TypeKind::MESSAGE: {
            auto struct_type{std::make_unique<StructValueType>()};
            if(auto ts = get_message_typesupport_handle(member_impl.members_,
                                                    TypeGeneratorInfo<g>::get_identifier()))
            {
              auto members{static_cast<const MetaMessage<g> *>(ts->data)};
              ROSIDL_InitMessageDataTypeMembers<g>(members, struct_type);
              mem_ptr = factory_create_type_member<ROSIDL_TypeKind::MESSAGE, g>(
              is_arr, fix_size, is_upper_bound, &member_impl, struct_type.get());
              mem_ptr->is_plain_types &= struct_type->is_plain_types();
              struct_type.release();
            } else {
              rcutils_error_string_t error_string = rcutils_get_error_string();
              rcutils_reset_error();

              throw std::runtime_error(
              std::string("Type support not from this implementation.  Got:\n") + "    " +
              error_string.str + "\n" + "while fetching it");
            }
          } break;
        default:
          break;
      }

      if(mem_ptr != nullptr) {
        mem_ptr->is_key = member_impl.is_key_;
        mem_ptr->member_offset = member_impl.offset_;
        mem_ptr->name = std::string(member_impl.name_);
        _type->add_member(mem_ptr);
      }
    }
  }
}

template<TypeGenerator g>
inline std::string generate_topic_data_type_name(MetaMessage<g> const *msg)
{
  std::string _name{};
  if(msg != nullptr) {
    std::ostringstream ss;
    std::string message_namespace(msg->message_namespace_);
    std::string message_name(msg->message_name_);
    if(!message_namespace.empty()) {
      message_namespace = std::regex_replace(message_namespace, std::regex("__"), "::");
      ss << message_namespace << "::";
    }
    ss << "dds_::" << message_name << "_";
    _name = ss.str();
  }

  return _name;
}

template<TypeGenerator g>
inline std::string generate_service_topic_data_type_name(
  MetaService<g> const *msg,
  bool is_request)
{
  std::string _name{};
  if(msg != nullptr) {
    std::ostringstream ss;
    std::string service_namespace(msg->service_namespace_);
    std::string service_name(msg->service_name_);
    std::string name_suffix{"_Request_"};
    if(!service_namespace.empty()) {
      if(!is_request) {
        name_suffix = "_Response_";
      }
      service_namespace = std::regex_replace(service_namespace, std::regex("__"), "::");
      ss << service_namespace << "::";
    }
    ss << "dds_::" << service_name << name_suffix;
    _name = ss.str();
  }

  return _name;
}

}  // namespace

DdsCdr & StructValueType::serialize(DdsCdr & cdr, void *data)
{
  for(auto const & mem : m_members) {
    mem->serialize(cdr, data);
  }

  return cdr;
}

DdsCdr & StructValueType::deserialize(DdsCdr & cdr, void *data)
{
  for(auto & mem : m_members) {
    mem->deserialize(cdr, data);
  }

  return cdr;
}

bool StructValueType::get_key(void *data, InstanceHandle_t *ihandle) noexcept
{
  if(!m_is_with_key) {
    return false;
  }
  static gstone::rtps::SerializedPayloadHeader payloadHeader{
    {0x00, 0x01},
    {0x00, 0x00}
  };
  DdsCdr cdr;
  cdr.init(payloadHeader);
  for(auto const & mem : m_members) {
    if(mem->is_key) {
      mem->serialize(cdr, data);
    }
  }
  void *addr{nullptr};
  uint32_t const length{cdr.get_buf(&addr)};
  if(length > 16) {
    gstone::rtps::UtilHelper::generate_digest(
        static_cast<char *>(addr), length, reinterpret_cast<char *>(ihandle->value));
  } else {
    memcpy(ihandle->value, addr, length);
  }
  if(addr) {
    delete addr;
    addr = nullptr;
  }
  return true;
}

uint32_t StructValueType::max_align_size(uint32_t const _cur_al, void *data)
{
  uint32_t max_size{_cur_al};

  for(auto const & mem : m_members) {
    max_size = mem->max_align_size(max_size, data);
  }

  return max_size;
}

std::shared_ptr<dds::topic::TopicDataType>
make_message_value_type(rosidl_message_type_support_t const *mts)
{
  std::shared_ptr<dds::topic::TopicDataType> res_type{nullptr};
  if(auto ts_c = get_message_typesupport_handle(
         mts, TypeGeneratorInfo<TypeGenerator::ROSIDL_C>::get_identifier()))
  {
    auto members{static_cast<const MetaMessage<TypeGenerator::ROSIDL_C> *>(ts_c->data)};
    auto topic_data_type{std::make_shared<MessageTopicDataType>()};
    topic_data_type->set_struct_size(members->size_of_);
    auto func{members->init_function};
    std::function<void(void *)> init_func{[func](void *data) -> void {
        func(data, ROSIDL_RUNTIME_C_MSG_INIT_ALL);
      }};
    topic_data_type->set_proc_func(init_func, members->fini_function);
    auto struct_type{std::make_unique<StructValueType>()};
    ROSIDL_InitMessageDataTypeMembers<TypeGenerator::ROSIDL_C>(members, struct_type);
    topic_data_type->set_members(struct_type);
    topic_data_type->set_name(
        generate_topic_data_type_name<TypeGenerator::ROSIDL_C>(members).c_str());
    topic_data_type->type_identifier(
        std::string{TypeGeneratorInfo<TypeGenerator::ROSIDL_C>::get_identifier()});
    res_type = std::move(topic_data_type);
    return res_type;
  } else {
    rcutils_error_string_t prev_error_string = rcutils_get_error_string();
    rcutils_reset_error();

    if(auto ts_cpp = get_message_typesupport_handle(
           mts, TypeGeneratorInfo<TypeGenerator::ROSIDL_Cpp>::get_identifier()))
    {
      auto members{static_cast<const MetaMessage<TypeGenerator::ROSIDL_Cpp> *>(ts_cpp->data)};
      auto topic_data_type{std::make_shared<MessageTopicDataType>()};
      topic_data_type->set_struct_size(members->size_of_);
      auto func{members->init_function};
      std::function<void(void *)> init_func{[func](void *data) -> void {
          func(data, rosidl_runtime_cpp::MessageInitialization::ALL);
        }};
      topic_data_type->set_proc_func(init_func, members->fini_function);
      auto struct_type{std::make_unique<StructValueType>()};
      ROSIDL_InitMessageDataTypeMembers<TypeGenerator::ROSIDL_Cpp>(members, struct_type);
      topic_data_type->set_members(struct_type);
      topic_data_type->set_name(
          generate_topic_data_type_name<TypeGenerator::ROSIDL_Cpp>(members).c_str());
      topic_data_type->type_identifier(
          std::string{TypeGeneratorInfo<TypeGenerator::ROSIDL_Cpp>::get_identifier()});
      res_type = std::move(topic_data_type);
      return res_type;
    } else {
      rcutils_error_string_t error_string = rcutils_get_error_string();
      rcutils_reset_error();

      throw std::runtime_error(std::string("Type support not from this implementation.  Got:\n") +
                               "    " + prev_error_string.str + "\n" + "    " + error_string.str +
                               "\n" + "while fetching it");
    }
  }
}

std::string get_message_value_type_name(rosidl_message_type_support_t const *mts)
{
  if(auto ts_c = get_message_typesupport_handle(
         mts, TypeGeneratorInfo<TypeGenerator::ROSIDL_C>::get_identifier()))
  {
    auto members{static_cast<const MetaMessage<TypeGenerator::ROSIDL_C> *>(ts_c->data)};
    return generate_topic_data_type_name<TypeGenerator::ROSIDL_C>(members);
  } else {
    rcutils_error_string_t prev_error_string = rcutils_get_error_string();
    rcutils_reset_error();

    if(auto ts_cpp = get_message_typesupport_handle(
           mts, TypeGeneratorInfo<TypeGenerator::ROSIDL_Cpp>::get_identifier()))
    {
      auto members{static_cast<const MetaMessage<TypeGenerator::ROSIDL_Cpp> *>(ts_cpp->data)};
      return generate_topic_data_type_name<TypeGenerator::ROSIDL_Cpp>(members);
    } else {
      rcutils_error_string_t error_string = rcutils_get_error_string();
      rcutils_reset_error();

      throw std::runtime_error(std::string("Type support not from this implementation.  Got:\n") +
                               "    " + prev_error_string.str + "\n" + "    " + error_string.str +
                               "\n" + "while fetching it");
    }
  }
}

uint32_t get_message_value_type_size(rosidl_message_type_support_t const *mts)
{
  if(auto ts_c = get_message_typesupport_handle(
         mts, TypeGeneratorInfo<TypeGenerator::ROSIDL_C>::get_identifier()))
  {
    auto members{static_cast<const MetaMessage<TypeGenerator::ROSIDL_C> *>(ts_c->data)};
    return static_cast<uint32_t>(members->size_of_);
  } else {
    rcutils_error_string_t prev_error_string = rcutils_get_error_string();
    rcutils_reset_error();

    if(auto ts_cpp = get_message_typesupport_handle(
           mts, TypeGeneratorInfo<TypeGenerator::ROSIDL_Cpp>::get_identifier()))
    {
      auto members{static_cast<const MetaMessage<TypeGenerator::ROSIDL_Cpp> *>(ts_cpp->data)};
      return static_cast<uint32_t>(members->size_of_);
    } else {
      rcutils_error_string_t error_string = rcutils_get_error_string();
      rcutils_reset_error();

      throw std::runtime_error(std::string("Type support not from this implementation.  Got:\n") +
                               "    " + prev_error_string.str + "\n" + "    " + error_string.str +
                               "\n" + "while fetching it");
    }
  }
}

std::pair<std::shared_ptr<dds::topic::TopicDataType>, std::shared_ptr<dds::topic::TopicDataType>>
make_request_response_value_types(rosidl_service_type_support_t const *svc_ts)
{
  if(auto tsc = get_service_typesupport_handle(
         svc_ts, TypeGeneratorInfo<TypeGenerator::ROSIDL_C>::get_identifier()))
  {
    auto typed =
      static_cast<const TypeGeneratorInfo<TypeGenerator::ROSIDL_C>::MetaService *>(tsc->data);

    auto req_members{typed->request_members_};
    auto req_topic_data_type{std::make_shared<ServiceTopicDataType>()};
    req_topic_data_type->set_struct_size(req_members->size_of_);
    auto req_func{req_members->init_function};
    std::function<void(void *)> req_init_func{[req_func](void *data) -> void {
        req_func(data, ROSIDL_RUNTIME_C_MSG_INIT_ALL);
      }};
    req_topic_data_type->set_proc_func(req_init_func, req_members->fini_function);
    auto req_struct_type{std::make_unique<StructValueType>()};
    ROSIDL_InitMessageDataTypeMembers<TypeGenerator::ROSIDL_C>(req_members, req_struct_type);
    req_topic_data_type->set_members(req_struct_type);
    req_topic_data_type->set_name(
        generate_service_topic_data_type_name<TypeGenerator::ROSIDL_C>(typed, true).c_str());
    req_topic_data_type->type_identifier(
        std::string{TypeGeneratorInfo<TypeGenerator::ROSIDL_C>::get_identifier()});
    auto res_members{typed->response_members_};
    auto res_topic_data_type{std::make_shared<ServiceTopicDataType>()};
    res_topic_data_type->set_struct_size(res_members->size_of_);
    auto res_func{res_members->init_function};
    std::function<void(void *)> res_init_func{[res_func](void *data) -> void {
        res_func(data, ROSIDL_RUNTIME_C_MSG_INIT_ALL);
      }};
    res_topic_data_type->set_proc_func(res_init_func, res_members->fini_function);
    auto res_struct_type{std::make_unique<StructValueType>()};
    ROSIDL_InitMessageDataTypeMembers<TypeGenerator::ROSIDL_C>(res_members, res_struct_type);
    res_topic_data_type->set_members(res_struct_type);
    res_topic_data_type->set_name(
        generate_service_topic_data_type_name<TypeGenerator::ROSIDL_C>(typed, false).c_str());
    res_topic_data_type->type_identifier(
        std::string{TypeGeneratorInfo<TypeGenerator::ROSIDL_C>::get_identifier()});

    return std::make_pair(std::move(req_topic_data_type), std::move(res_topic_data_type));
  } else {
    rcutils_error_string_t prev_error_string = rcutils_get_error_string();
    rcutils_reset_error();

    if(auto tscpp = get_service_typesupport_handle(
           svc_ts, TypeGeneratorInfo<TypeGenerator::ROSIDL_Cpp>::get_identifier()))
    {
      auto typed = static_cast<const TypeGeneratorInfo<TypeGenerator::ROSIDL_Cpp>::MetaService *>(
        tscpp->data);

      auto req_members{typed->request_members_};
      auto req_topic_data_type{std::make_shared<ServiceTopicDataType>()};
      req_topic_data_type->set_struct_size(req_members->size_of_);
      auto req_func{req_members->init_function};
      std::function<void(void *)> req_init_func{[req_func](void *data) -> void {
          req_func(data, rosidl_runtime_cpp::MessageInitialization::ALL);
        }};
      req_topic_data_type->set_proc_func(req_init_func, req_members->fini_function);
      auto req_struct_type{std::make_unique<StructValueType>()};
      ROSIDL_InitMessageDataTypeMembers<TypeGenerator::ROSIDL_Cpp>(req_members, req_struct_type);
      req_topic_data_type->set_members(req_struct_type);
      req_topic_data_type->set_name(
          generate_service_topic_data_type_name<TypeGenerator::ROSIDL_Cpp>(typed, true).c_str());
      req_topic_data_type->type_identifier(
          std::string{TypeGeneratorInfo<TypeGenerator::ROSIDL_Cpp>::get_identifier()});

      auto res_members{typed->response_members_};
      auto res_topic_data_type{std::make_shared<ServiceTopicDataType>()};
      res_topic_data_type->set_struct_size(res_members->size_of_);
      auto res_func{res_members->init_function};
      std::function<void(void *)> res_init_func{[res_func](void *data) -> void {
          res_func(data, rosidl_runtime_cpp::MessageInitialization::ALL);
        }};
      res_topic_data_type->set_proc_func(res_init_func, res_members->fini_function);
      auto res_struct_type{std::make_unique<StructValueType>()};
      ROSIDL_InitMessageDataTypeMembers<TypeGenerator::ROSIDL_Cpp>(res_members, res_struct_type);
      res_topic_data_type->set_members(res_struct_type);
      res_topic_data_type->set_name(
          generate_service_topic_data_type_name<TypeGenerator::ROSIDL_Cpp>(typed, false).c_str());
      res_topic_data_type->type_identifier(
          std::string{TypeGeneratorInfo<TypeGenerator::ROSIDL_Cpp>::get_identifier()});

      return std::make_pair(std::move(req_topic_data_type), std::move(res_topic_data_type));
    } else {
      rcutils_error_string_t error_string = rcutils_get_error_string();
      rcutils_reset_error();

      throw std::runtime_error(
          std::string("Service type support not from this implementation.  Got:\n") + "    " +
          prev_error_string.str + "\n" + "    " + error_string.str + "\n" + "while fetching it");
    }
  }
}

std::pair<std::string, std::string>
get_request_response_value_type_name(rosidl_service_type_support_t const *svc_ts)
{
  if(auto tsc = get_service_typesupport_handle(
         svc_ts, TypeGeneratorInfo<TypeGenerator::ROSIDL_C>::get_identifier()))
  {
    auto typed =
      static_cast<const TypeGeneratorInfo<TypeGenerator::ROSIDL_C>::MetaService *>(tsc->data);

    return std::make_pair(
        generate_service_topic_data_type_name<TypeGenerator::ROSIDL_C>(typed, true),
        generate_service_topic_data_type_name<TypeGenerator::ROSIDL_C>(typed, false));
  } else {
    rcutils_error_string_t prev_error_string = rcutils_get_error_string();
    rcutils_reset_error();

    if(auto tscpp = get_service_typesupport_handle(
           svc_ts, TypeGeneratorInfo<TypeGenerator::ROSIDL_Cpp>::get_identifier()))
    {
      auto typed = static_cast<const TypeGeneratorInfo<TypeGenerator::ROSIDL_Cpp>::MetaService *>(
        tscpp->data);

      return std::make_pair(
          generate_service_topic_data_type_name<TypeGenerator::ROSIDL_Cpp>(typed, true),
          generate_service_topic_data_type_name<TypeGenerator::ROSIDL_Cpp>(typed, false));
    } else {
      rcutils_error_string_t error_string = rcutils_get_error_string();
      rcutils_reset_error();

      throw std::runtime_error(
          std::string("Service type support not from this implementation.  Got:\n") + "    " +
          prev_error_string.str + "\n" + "    " + error_string.str + "\n" + "while fetching it");
    }
  }
}

}  // namespace rmw_swiftdds_shared_cpp
