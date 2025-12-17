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

#ifndef RMW_SWIFTDDS_SHARED_CPP__QOS_HPP_
#define RMW_SWIFTDDS_SHARED_CPP__QOS_HPP_

#include "rmw/rmw.h"

#include "rmw_swiftdds_shared_cpp/visibility_control.h"

#include "rosidl_runtime_c/type_hash.h"

#include "swiftdds/dcps/SwiftDdsExport.h"

RMW_SWIFTDDS_SHARED_CPP_PUBLIC
bool is_valid_qos(const rmw_qos_profile_t & qos_policies);

RMW_SWIFTDDS_SHARED_CPP_PUBLIC
bool get_datareader_qos(
  const rmw_qos_profile_t & qos_policies,
  const rosidl_type_hash_t & type_hash,
  greenstone::dds::DataReaderQos & reader_qos,
  const rosidl_type_hash_t * ser_type_hash = nullptr);

RMW_SWIFTDDS_SHARED_CPP_PUBLIC
bool get_datawriter_qos(
  const rmw_qos_profile_t & qos_policies,
  const rosidl_type_hash_t & type_hash,
  greenstone::dds::DataWriterQos & writer_qos,
  const rosidl_type_hash_t * ser_type_hash = nullptr);

RMW_SWIFTDDS_SHARED_CPP_PUBLIC
bool get_topic_qos(const rmw_qos_profile_t & qos_policies, greenstone::dds::TopicQos & topic_qos);

RMW_SWIFTDDS_SHARED_CPP_PUBLIC
rmw_time_t dds_duration_to_rmw(const greenstone::dds::Duration_t & duration);

/*
 * Converts the DDS QOS Policy; of type DataWriterQos or DataReaderQos into
 * rmw_qos_profile_t.
 *
 * \param[in] dds_qos of type DataWriterQos or DataReaderQos
 * \param[out] qos the equivalent of the data in dds_qos as a rmw_qos_profile_t
 */
template<typename DDSQoSPolicyT>
void dds_qos_to_rmw_qos(const DDSQoSPolicyT & dds_qos, rmw_qos_profile_t *qos)
{
  switch(dds_qos.reliability().kind()) {
    case greenstone::dds::ReliabilityQosPolicyKind::BEST_EFFORT_RELIABILITY_QOS:
      qos->reliability = RMW_QOS_POLICY_RELIABILITY_BEST_EFFORT;
      break;
    case greenstone::dds::ReliabilityQosPolicyKind::RELIABLE_RELIABILITY_QOS:
      qos->reliability = RMW_QOS_POLICY_RELIABILITY_RELIABLE;
      break;
    default:
      qos->reliability = RMW_QOS_POLICY_RELIABILITY_UNKNOWN;
      break;
  }

  switch(dds_qos.durability().kind()) {
    case greenstone::dds::DurabilityQosPolicyKind::TRANSIENT_LOCAL_DURABILITY_QOS:
      qos->durability = RMW_QOS_POLICY_DURABILITY_TRANSIENT_LOCAL;
      break;
    case greenstone::dds::DurabilityQosPolicyKind::VOLATILE_DURABILITY_QOS:
      qos->durability = RMW_QOS_POLICY_DURABILITY_VOLATILE;
      break;
    default:
      qos->durability = RMW_QOS_POLICY_DURABILITY_UNKNOWN;
      break;
  }

  qos->deadline = dds_duration_to_rmw(dds_qos.deadline().period());
  // qos->lifespan = dds_duration_to_rmw(dds_qos.life_span().duration());

  switch(dds_qos.liveliness().kind()) {
    case greenstone::dds::LivelinessQosPolicyKind::AUTOMATIC_LIVELINESS_QOS:
      qos->liveliness = RMW_QOS_POLICY_LIVELINESS_AUTOMATIC;
      break;
    case greenstone::dds::LivelinessQosPolicyKind::MANUAL_BY_TOPIC_LIVELINESS_QOS:
      qos->liveliness = RMW_QOS_POLICY_LIVELINESS_MANUAL_BY_TOPIC;
      break;
    default:
      qos->liveliness = RMW_QOS_POLICY_LIVELINESS_UNKNOWN;
      break;
  }

  qos->liveliness_lease_duration = dds_duration_to_rmw(dds_qos.liveliness().lease_duration());

  switch(dds_qos.history().kind()) {
    case greenstone::dds::HistoryQosPolicyKind::KEEP_LAST_HISTORY_QOS:
      qos->history = RMW_QOS_POLICY_HISTORY_KEEP_LAST;
      break;
    case greenstone::dds::HistoryQosPolicyKind::KEEP_ALL_HISTORY_QOS:
      qos->history = RMW_QOS_POLICY_HISTORY_KEEP_ALL;
      break;
    default:
      qos->history = RMW_QOS_POLICY_HISTORY_UNKNOWN;
      break;
  }
  qos->depth = static_cast<size_t>(dds_qos.history().depth());
}

/*
 * Converts the RTPS QOS Policy; of type WriterQos or ReaderQos into
 * rmw_qos_profile_t. Since WriterQos or ReaderQos do not have information about
 * history and depth, these values are not set by this function.
 *
 * \param[in] rtps_qos of type WriterQos or ReaderQos
 * \param[out] qos the equivalent of the data in rtps_qos as a rmw_qos_profile_t
 */
template<typename BuiltinData>
void rtps_qos_to_rmw_qos(const BuiltinData & rtps_qos, rmw_qos_profile_t *qos)
{
  switch(rtps_qos.reliability().kind()) {
    case greenstone::dds::ReliabilityQosPolicyKind::BEST_EFFORT_RELIABILITY_QOS:
      qos->reliability = RMW_QOS_POLICY_RELIABILITY_BEST_EFFORT;
      break;
    case greenstone::dds::ReliabilityQosPolicyKind::RELIABLE_RELIABILITY_QOS:
      qos->reliability = RMW_QOS_POLICY_RELIABILITY_RELIABLE;
      break;
    default:
      qos->reliability = RMW_QOS_POLICY_RELIABILITY_UNKNOWN;
      break;
  }

  switch(rtps_qos.durability().kind()) {
    case greenstone::dds::DurabilityQosPolicyKind::TRANSIENT_LOCAL_DURABILITY_QOS:
      qos->durability = RMW_QOS_POLICY_DURABILITY_TRANSIENT_LOCAL;
      break;
    case greenstone::dds::DurabilityQosPolicyKind::VOLATILE_DURABILITY_QOS:
      qos->durability = RMW_QOS_POLICY_DURABILITY_VOLATILE;
      break;
    default:
      qos->durability = RMW_QOS_POLICY_DURABILITY_UNKNOWN;
      break;
  }

  qos->deadline = dds_duration_to_rmw(rtps_qos.deadline().period());
  qos->lifespan = dds_duration_to_rmw(rtps_qos.lifespan().duration());

  switch(rtps_qos.liveliness().kind()) {
    case greenstone::dds::LivelinessQosPolicyKind::AUTOMATIC_LIVELINESS_QOS:
      qos->liveliness = RMW_QOS_POLICY_LIVELINESS_AUTOMATIC;
      break;
    case greenstone::dds::LivelinessQosPolicyKind::MANUAL_BY_TOPIC_LIVELINESS_QOS:
      qos->liveliness = RMW_QOS_POLICY_LIVELINESS_MANUAL_BY_TOPIC;
      break;
    default:
      qos->liveliness = RMW_QOS_POLICY_LIVELINESS_UNKNOWN;
      break;
  }
  qos->liveliness_lease_duration = dds_duration_to_rmw(rtps_qos.liveliness().lease_duration());

  if(rtps_qos.history().optional_flag()) {
    switch(rtps_qos.history().get_value().kind()) {
      case greenstone::dds::HistoryQosPolicyKind::KEEP_LAST_HISTORY_QOS:
        qos->history = RMW_QOS_POLICY_HISTORY_KEEP_LAST;
        break;
      case greenstone::dds::HistoryQosPolicyKind::KEEP_ALL_HISTORY_QOS:
        qos->history = RMW_QOS_POLICY_HISTORY_KEEP_ALL;
        break;
      default:
        qos->history = RMW_QOS_POLICY_HISTORY_UNKNOWN;
        break;
    }
    qos->depth = static_cast<size_t>(rtps_qos.history().get_value().depth());
  } else {
    qos->history = RMW_QOS_POLICY_HISTORY_UNKNOWN;
    qos->depth = RMW_QOS_POLICY_DEPTH_SYSTEM_DEFAULT;
  }
}

extern template RMW_SWIFTDDS_SHARED_CPP_PUBLIC void
dds_qos_to_rmw_qos<greenstone::dds::DataWriterQos>(
  const greenstone::dds::DataWriterQos & dds_qos,
  rmw_qos_profile_t *qos);

extern template RMW_SWIFTDDS_SHARED_CPP_PUBLIC void
dds_qos_to_rmw_qos<greenstone::dds::DataReaderQos>(
  const greenstone::dds::DataReaderQos & dds_qos,
  rmw_qos_profile_t *qos);

#endif  // RMW_SWIFTDDS_SHARED_CPP__QOS_HPP_
