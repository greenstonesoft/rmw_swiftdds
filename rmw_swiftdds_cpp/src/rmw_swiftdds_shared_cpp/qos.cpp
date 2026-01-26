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

#include <limits>
#include <string>
#include <vector>

#include "rcutils/logging_macros.h"

#include "rmw_swiftdds_shared_cpp/qos.hpp"
#include "rmw_swiftdds_shared_cpp/qos_config.hpp"

#include "rmw/error_handling.h"

#include "rmw_dds_common/qos.hpp"

#include "rosidl_runtime_c/type_hash.h"

#include "time_utils.hpp"

#include "swiftdds/dcps/SwiftDdsExport.h"

static bool is_rmw_duration_unspecified(const rmw_time_t & time)
{
  return rmw_time_equal(time, RMW_DURATION_UNSPECIFIED);
}

rmw_time_t dds_duration_to_rmw(const greenstone::dds::Duration_t & duration)
{
  if(duration == greenstone::dds::Duration::duration_infinite()) {
    return RMW_DURATION_INFINITE;
  }
  rmw_time_t result = {(uint64_t)duration.seconds(), (uint64_t)duration.nanosec()};
  return result;
}

// Private function to encapsulate DataReader and DataWriter, together with
// Topic, filling entities DDS QoS from the RMW QoS profile.
template<typename DDSEntityQos>
bool fill_entity_qos_from_profile(const rmw_qos_profile_t & qos_policies, DDSEntityQos & entity_qos)
{
  greenstone::dds::DurabilityQosPolicy durability;
  switch(qos_policies.durability) {
    case RMW_QOS_POLICY_DURABILITY_TRANSIENT_LOCAL:
      durability.kind(greenstone::dds::DurabilityQosPolicyKind::TRANSIENT_LOCAL_DURABILITY_QOS);
      entity_qos.durability(durability);
      break;
    case RMW_QOS_POLICY_DURABILITY_VOLATILE:
      durability.kind(greenstone::dds::DurabilityQosPolicyKind::VOLATILE_DURABILITY_QOS);
      entity_qos.durability(durability);
      break;
    case RMW_QOS_POLICY_DURABILITY_SYSTEM_DEFAULT:
      break;
    default:
      RMW_SET_ERROR_MSG("Unknown QoS durability policy");
      return false;
  }

  greenstone::dds::ReliabilityQosPolicy reliability;
  switch(qos_policies.reliability) {
    case RMW_QOS_POLICY_RELIABILITY_BEST_EFFORT:
      reliability.kind(greenstone::dds::ReliabilityQosPolicyKind::BEST_EFFORT_RELIABILITY_QOS);
      entity_qos.reliability(reliability);
      break;
    case RMW_QOS_POLICY_RELIABILITY_RELIABLE:
      reliability.kind(greenstone::dds::ReliabilityQosPolicyKind::RELIABLE_RELIABILITY_QOS);
      entity_qos.reliability(reliability);
      break;
    case RMW_QOS_POLICY_RELIABILITY_SYSTEM_DEFAULT:
      break;
    default:
      RMW_SET_ERROR_MSG("Unknown QoS reliability policy");
      return false;
  }

  greenstone::dds::HistoryQosPolicy history;
  // ensure the history depth is at least the requested queue size
  assert(entity_qos.history().depth() >= 0);
  if(qos_policies.depth != RMW_QOS_POLICY_DEPTH_SYSTEM_DEFAULT &&
    static_cast<size_t>(entity_qos.history().depth()) < qos_policies.depth)
  {
    if(qos_policies.depth > static_cast<size_t>((std::numeric_limits<int32_t>::max)())) {
      RMW_SET_ERROR_MSG("failed to set history depth since the requested queue size exceeds "
                        "the DDS type");
      return false;
    }
    history.depth(qos_policies.depth);
  }
  switch(qos_policies.history) {
    case RMW_QOS_POLICY_HISTORY_KEEP_LAST:
      history.kind(greenstone::dds::HistoryQosPolicyKind::KEEP_LAST_HISTORY_QOS);
      entity_qos.history(history);
      break;
    case RMW_QOS_POLICY_HISTORY_KEEP_ALL:
      history.kind(greenstone::dds::HistoryQosPolicyKind::KEEP_ALL_HISTORY_QOS);
      entity_qos.history(history);
      break;
    case RMW_QOS_POLICY_HISTORY_SYSTEM_DEFAULT:
      break;
    default:
      RMW_SET_ERROR_MSG("Unknown QoS history policy");
      return false;
  }

  // if (DDS::EntityType::DATAWRITER == kind || DDS::EntityType::TOPIC == kind)
  // {
  //   greenstone::dds::LifespanQosPolicy lifespan;
  //   if (!is_rmw_duration_unspecified(qos_policies.lifespan)) {
  //     lifespan.duration(rmw_swiftdds_shared_cpp::rmw_time_to_swiftdds(qos_policies.lifespan));
  //     entity_qos.life_span(lifespan);
  //   }
  // }

  greenstone::dds::DeadlineQosPolicy deadline;
  if(!is_rmw_duration_unspecified(qos_policies.deadline)) {
    deadline.period(rmw_swiftdds_shared_cpp::rmw_time_to_swiftdds(qos_policies.deadline));
    entity_qos.deadline(deadline);
  }

  greenstone::dds::LivelinessQosPolicy liveliness;
  switch(qos_policies.liveliness) {
    case RMW_QOS_POLICY_LIVELINESS_AUTOMATIC:
      liveliness.kind(greenstone::dds::LivelinessQosPolicyKind::AUTOMATIC_LIVELINESS_QOS);
      break;
    case RMW_QOS_POLICY_LIVELINESS_MANUAL_BY_TOPIC:
      liveliness.kind(greenstone::dds::LivelinessQosPolicyKind::MANUAL_BY_TOPIC_LIVELINESS_QOS);
      break;
    case RMW_QOS_POLICY_LIVELINESS_SYSTEM_DEFAULT:
      break;
    default:
      RMW_SET_ERROR_MSG("Unknown QoS Liveliness policy");
      return false;
  }
  if(!is_rmw_duration_unspecified(qos_policies.liveliness_lease_duration)) {
    liveliness.lease_duration(
        rmw_swiftdds_shared_cpp::rmw_time_to_swiftdds(qos_policies.liveliness_lease_duration));
  }
  entity_qos.liveliness(liveliness);

  return true;
}

template<typename DDSEntityQos>
bool fill_data_entity_qos_from_profile(
  const rmw_qos_profile_t & qos_policies,
  const rosidl_type_hash_t & type_hash,
  DDSEntityQos & entity_qos,
  const rosidl_type_hash_t * ser_type_hash = nullptr)
{
  if(!fill_entity_qos_from_profile(qos_policies, entity_qos)) {
    return false;
  }
  std::string user_data_str;
  if(RMW_RET_OK != rmw_dds_common::encode_type_hash_for_user_data_qos(type_hash, user_data_str)) {
    RCUTILS_LOG_WARN_NAMED("rmw_swiftdds_shared_cpp",
                           "Failed to encode type hash for topic, will not "
                           "distribute it in USER_DATA.");
    user_data_str.clear();
    // Since we are going to go on without a hash, we clear the error so other
    // code won't overwrite it.
    rmw_reset_error();
  }
  if (ser_type_hash) {
    std::string typehash_str;
    if (RMW_RET_OK ==
      rmw_dds_common::encode_sertype_hash_for_user_data_qos(*ser_type_hash, typehash_str))
    {
      user_data_str += typehash_str;
    }
  }
  std::vector<uint8_t> user_data(user_data_str.begin(), user_data_str.end());
  greenstone::dds::UserDataQosPolicy ud;
  ud.value(user_data);
  entity_qos.user_data(ud);
  return true;
}

bool get_datareader_qos(
  const rmw_qos_profile_t & qos_policies,
  const rosidl_type_hash_t & type_hash,
  greenstone::dds::DataReaderQos & datareader_qos,
  const rosidl_type_hash_t * ser_type_hash)
{
  //////////////////////
  greenstone::dds::DataReaderAttributes attr;
  attr.push_back_prefer_transport_kind(gstone::rtps::TransportKind_t::TRANSPORT_KIND_SHM);
  attr.only_recv_by_udp(false);
  datareader_qos.attributes(attr);

  if(fill_data_entity_qos_from_profile(qos_policies, type_hash, datareader_qos, ser_type_hash)) {
    return true;
  }

  return false;
}

bool get_datawriter_qos(
  const rmw_qos_profile_t & qos_policies,
  const rosidl_type_hash_t & type_hash,
  greenstone::dds::DataWriterQos & datawriter_qos,
  const rosidl_type_hash_t * ser_type_hash)
{
  ////////////////////////////
  greenstone::dds::DataWriterAttributes attr;
  attr.push_back_prefer_transport_kind(gstone::rtps::TransportKind_t::TRANSPORT_KIND_SHM);
  attr.only_recv_by_udp(false);
  {
    std::lock_guard<std::mutex> lock{rmw_swiftdds_shared_cpp::QosConfig::getInstance().
      qosConfig_mutex};
    attr.is_sync(rmw_swiftdds_shared_cpp::QosConfig::getInstance().send_sync);
    attr.enable_zero_copy(rmw_swiftdds_shared_cpp::QosConfig::getInstance().enable_zero_copy);
    if(attr.enable_zero_copy()) {
      attr.zero_copy_memory_size(
      rmw_swiftdds_shared_cpp::QosConfig::getInstance().zero_copy_shm_size);
    }
  }
  datawriter_qos.attributes(attr);

  greenstone::dds::LifespanQosPolicy lifespan;
  if(!is_rmw_duration_unspecified(qos_policies.lifespan)) {
    lifespan.duration(rmw_swiftdds_shared_cpp::rmw_time_to_swiftdds(qos_policies.lifespan));
    datawriter_qos.life_span(lifespan);
  }
  if(fill_data_entity_qos_from_profile(qos_policies, type_hash, datawriter_qos, ser_type_hash)) {
    return true;
  }

  return false;
}

bool get_topic_qos(const rmw_qos_profile_t & qos_policies, greenstone::dds::TopicQos & topic_qos)
{
  greenstone::dds::LifespanQosPolicy lifespan;
  if(!is_rmw_duration_unspecified(qos_policies.lifespan)) {
    lifespan.duration(rmw_swiftdds_shared_cpp::rmw_time_to_swiftdds(qos_policies.lifespan));
    topic_qos.life_span(lifespan);
  }
  return fill_entity_qos_from_profile(qos_policies, topic_qos);
}

bool is_valid_qos(const rmw_qos_profile_t & /* qos_policies */)
{
  return true;
}

template void
dds_qos_to_rmw_qos<greenstone::dds::DataWriterQos>(
  const greenstone::dds::DataWriterQos & dds_qos,
  rmw_qos_profile_t *qos);

template void
dds_qos_to_rmw_qos<greenstone::dds::DataReaderQos>(
  const greenstone::dds::DataReaderQos & dds_qos,
  rmw_qos_profile_t *qos);
