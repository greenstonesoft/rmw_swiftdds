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

#ifndef RMW_SWIFTDDS_SHARED_CPP__CUSTOM_PUBLISHER_INFO_HPP_
#define RMW_SWIFTDDS_SHARED_CPP__CUSTOM_PUBLISHER_INFO_HPP_

#include <mutex>
#include <set>
#include <memory>

#include "rcpputils/thread_safety_annotations.hpp"
#include "rmw/rmw.h"

#include "rmw_swiftdds_shared_cpp/custom_event_info.hpp"

#include "swiftdds/dcps/SwiftDdsExport.h"

class RMWPublisherEvent;

class CustomDataWriterListener final : public greenstone::dds::DataWriterListener
{
public:
  RMW_SWIFTDDS_SHARED_CPP_PUBLIC
  explicit CustomDataWriterListener(RMWPublisherEvent *pub_event);

  RMW_SWIFTDDS_SHARED_CPP_PUBLIC
  void
  on_publication_matched(
    greenstone::dds::DataWriter *writer,
    const greenstone::dds::PublicationMatchedStatus & status) noexcept override;

  RMW_SWIFTDDS_SHARED_CPP_PUBLIC
  void on_offered_deadline_missed(
    greenstone::dds::DataWriter *writer,
    const greenstone::dds::OfferedDeadlineMissedStatus & status) noexcept override;

  RMW_SWIFTDDS_SHARED_CPP_PUBLIC
  void on_liveliness_lost(
    greenstone::dds::DataWriter *writer,
    const greenstone::dds::LivelinessLostStatus & status) noexcept override;

  RMW_SWIFTDDS_SHARED_CPP_PUBLIC
  void on_offered_incompatible_qos(
    greenstone::dds::DataWriter *,
    const greenstone::dds::OfferedIncompatibleQosStatus & status) noexcept override;

private:
  RMWPublisherEvent *publisher_event_;
};

typedef struct CustomPublisherInfo : public CustomEventInfo
{
  virtual ~CustomPublisherInfo() = default;

  greenstone::dds::DataWriter *data_writer_{nullptr};
  RMWPublisherEvent *publisher_event_{nullptr};
  CustomDataWriterListener *data_writer_listener_{nullptr};
  greenstone::dds::StatusMask data_writer_listener_mask_{0};
  std::shared_ptr<greenstone::dds::TopicDataType> type_support_;
  const void *type_support_impl_{nullptr};
  rmw_gid_t publisher_gid{};
  const char *typesupport_identifier_{nullptr};

  greenstone::dds::Topic *topic_{nullptr};

  RMW_SWIFTDDS_SHARED_CPP_PUBLIC
  EventListenerInterface * get_listener() const final;
} CustomPublisherInfo;

class RMWPublisherEvent final : public EventListenerInterface
{
public:
  RMW_SWIFTDDS_SHARED_CPP_PUBLIC
  explicit RMWPublisherEvent(CustomPublisherInfo *info);

  RMW_SWIFTDDS_SHARED_CPP_PUBLIC
  greenstone::dds::StatusCondition * get_statuscondition() const override;

  RMW_SWIFTDDS_SHARED_CPP_PUBLIC
  bool take_event(rmw_event_type_t event_type, void *event_info) override;

  RMW_SWIFTDDS_SHARED_CPP_PUBLIC
  void set_on_new_event_callback(
    rmw_event_type_t event_type,
    const void *user_data,
    rmw_event_callback_t callback) override;

  RMW_SWIFTDDS_SHARED_CPP_PUBLIC
  void update_inconsistent_topic(uint32_t total_count, uint32_t total_count_change) override;

  /// Add a GUID to the internal set of unique subscriptions matched to this
  /// publisher.
  /**
   * This is so we can provide the RMW layer with an accurate count of matched
   * subscriptions if the user calls rmw_count_subscribers().
   *
   * \param[in] guid The GUID of the newly-matched subscription to track.
   */
  RMW_SWIFTDDS_SHARED_CPP_PUBLIC
  void track_unique_subscription(greenstone::dds::GUID guid);

  /// Remove a GUID from the internal set of unique subscriptions matched to
  /// this publisher.
  /**
   * This is so we can provide the RMW layer with an accurate count of matched
   * subscriptions if the user calls rmw_count_subscribers().
   *
   * \param[in] guid The GUID of the newly-unmatched subscription to track.
   */
  RMW_SWIFTDDS_SHARED_CPP_PUBLIC
  void untrack_unique_subscription(greenstone::dds::GUID guid);

  /// Return the number of unique subscriptions matched to this publisher.
  /**
   * \return Number of unique subscriptions matched to this publisher.
   */
  RMW_SWIFTDDS_SHARED_CPP_PUBLIC
  size_t subscription_count() const;

  RMW_SWIFTDDS_SHARED_CPP_PUBLIC
  void update_deadline(uint32_t total_count, uint32_t total_count_change);

  RMW_SWIFTDDS_SHARED_CPP_PUBLIC
  void update_liveliness_lost(uint32_t total_count, uint32_t total_count_change);

  RMW_SWIFTDDS_SHARED_CPP_PUBLIC
  void update_offered_incompatible_qos(
    greenstone::dds::QosPolicyId_t last_policy_id,
    uint32_t total_count,
    uint32_t total_count_change);

  RMW_SWIFTDDS_SHARED_CPP_PUBLIC
  void update_matched(
    int32_t total_count,
    int32_t total_count_change,
    int32_t current_count,
    int32_t current_count_change);

private:
  CustomPublisherInfo *publisher_info_ = nullptr;

  std::set<greenstone::dds::GUID> subscriptions_ RCPPUTILS_TSA_GUARDED_BY(subscriptions_mutex_);

  mutable std::mutex subscriptions_mutex_;

  bool deadline_changed_ RCPPUTILS_TSA_GUARDED_BY(on_new_event_m_);

  greenstone::dds::OfferedDeadlineMissedStatus
  offered_deadline_missed_status_ RCPPUTILS_TSA_GUARDED_BY(on_new_event_m_);

  bool liveliness_changed_ RCPPUTILS_TSA_GUARDED_BY(on_new_event_m_);

  greenstone::dds::LivelinessLostStatus
  liveliness_lost_status_ RCPPUTILS_TSA_GUARDED_BY(on_new_event_m_);

  bool incompatible_qos_changed_ RCPPUTILS_TSA_GUARDED_BY(on_new_event_m_);

  greenstone::dds::OfferedIncompatibleQosStatus
  incompatible_qos_status_ RCPPUTILS_TSA_GUARDED_BY(on_new_event_m_);

  bool matched_changes_ RCPPUTILS_TSA_GUARDED_BY(on_new_event_m_);

  greenstone::dds::PublicationMatchedStatus
  matched_status_ RCPPUTILS_TSA_GUARDED_BY(on_new_event_m_);

  void trigger_event(rmw_event_type_t event_type);
};

#endif  // RMW_SWIFTDDS_SHARED_CPP__CUSTOM_PUBLISHER_INFO_HPP_
