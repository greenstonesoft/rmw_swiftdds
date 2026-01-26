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

#ifndef RMW_SWIFTDDS_SHARED_CPP__CUSTOM_SUBSCRIBER_INFO_HPP_
#define RMW_SWIFTDDS_SHARED_CPP__CUSTOM_SUBSCRIBER_INFO_HPP_

#include <algorithm>
#include <limits>
#include <memory>
#include <mutex>
#include <set>
#include <string>
#include <utility>

#include "rcpputils/thread_safety_annotations.hpp"

#include "rmw/impl/cpp/macros.hpp"
#include "rmw/event_callback_type.h"

#include "rmw_dds_common/context.hpp"

#include "rmw_swiftdds_shared_cpp/custom_event_info.hpp"

#include "swiftdds/dcps/SwiftDdsExport.h"

class RMWSubscriptionEvent;

class CustomDataReaderListener final : public greenstone::dds::DataReaderListener
{
public:
  RMW_SWIFTDDS_SHARED_CPP_PUBLIC
  explicit CustomDataReaderListener(RMWSubscriptionEvent *sub_event);

  RMW_SWIFTDDS_SHARED_CPP_PUBLIC
  void
  on_subscription_matched(
    greenstone::dds::DataReader *reader,
    const greenstone::dds::SubscriptionMatchedStatus & info) noexcept override;

  RMW_SWIFTDDS_SHARED_CPP_PUBLIC
  void on_data_available(greenstone::dds::DataReader *reader) noexcept override;

  RMW_SWIFTDDS_SHARED_CPP_PUBLIC
  void on_requested_deadline_missed(
    greenstone::dds::DataReader *reader,
    const greenstone::dds::RequestedDeadlineMissedStatus & status) noexcept override;

  RMW_SWIFTDDS_SHARED_CPP_PUBLIC
  void
  on_liveliness_changed(
    greenstone::dds::DataReader *reader,
    const greenstone::dds::LivelinessChangedStatus & status) noexcept override;

  RMW_SWIFTDDS_SHARED_CPP_PUBLIC
  void on_sample_lost(
    greenstone::dds::DataReader *reader,
    const greenstone::dds::SampleLostStatus & status) noexcept override;

  RMW_SWIFTDDS_SHARED_CPP_PUBLIC
  void on_requested_incompatible_qos(
    greenstone::dds::DataReader *reader,
    const greenstone::dds::RequestedIncompatibleQosStatus & status) noexcept override;

private:
  RMWSubscriptionEvent *subscription_event_;
};

namespace rmw_swiftdds_shared_cpp
{
struct LoanManager;
}  // namespace rmw_swiftdds_shared_cpp

struct CustomSubscriberInfo : public CustomEventInfo
{
  virtual ~CustomSubscriberInfo() = default;

  greenstone::dds::DataReader *data_reader_{nullptr};
  RMWSubscriptionEvent *subscription_event_{nullptr};
  CustomDataReaderListener *data_reader_listener_{nullptr};
  greenstone::dds::StatusMask data_reader_listener_mask_{0};
  std::shared_ptr<greenstone::dds::TopicDataType> type_support_;
  const void *type_support_impl_{nullptr};
  rmw_gid_t subscription_gid_{};
  const char *typesupport_identifier_{nullptr};
  // std::shared_ptr<rmw_swiftdds_shared_cpp::LoanManager> loan_manager_;

  // for re-create or delete content filtered topic
  const rmw_node_t *node_{nullptr};
  rmw_dds_common::Context *common_context_{nullptr};
  greenstone::dds::DomainParticipant *dds_participant_{nullptr};
  greenstone::dds::Subscriber *subscriber_{nullptr};
  std::string topic_name_mangled_;
  greenstone::dds::Topic *topic_{nullptr};
  greenstone::dds::ContentFilteredTopic *filtered_topic_{nullptr};
  greenstone::dds::DataReaderQos datareader_qos_;

  RMW_SWIFTDDS_SHARED_CPP_PUBLIC
  EventListenerInterface * get_listener() const final;
};

class RMWSubscriptionEvent final : public EventListenerInterface
{
public:
  RMW_SWIFTDDS_SHARED_CPP_PUBLIC
  explicit RMWSubscriptionEvent(CustomSubscriberInfo *info);

  // Provide handlers to perform an action when a
  // new event from this listener has occurred
  RMW_SWIFTDDS_SHARED_CPP_PUBLIC
  void set_on_new_message_callback(const void *user_data, rmw_event_callback_t callback);

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

  /// Add a GUID to the internal set of unique publishers matched to this
  /// subscription.
  /**
   * This is so we can provide the RMW layer with an accurate count of matched
   * publishers if the user calls rmw_count_publishers().
   *
   * \param[in] guid The GUID of the newly-matched publisher to track.
   */
  RMW_SWIFTDDS_SHARED_CPP_PUBLIC
  void track_unique_publisher(greenstone::dds::GUID guid);

  /// Remove a GUID from the internal set of unique publishers matched to this
  /// subscription.
  /**
   * This is so we can provide the RMW layer with an accurate count of matched
   * publishers if the user calls rmw_count_publishers().
   *
   * \param[in] guid The GUID of the newly-unmatched publisher to track.
   */
  RMW_SWIFTDDS_SHARED_CPP_PUBLIC
  void untrack_unique_publisher(greenstone::dds::GUID guid);

  /// Return the number of unique publishers matched to this subscription.
  /**
   * \return Number of unique publishers matched to this subscription.
   */
  RMW_SWIFTDDS_SHARED_CPP_PUBLIC
  size_t publisher_count() const;

  RMW_SWIFTDDS_SHARED_CPP_PUBLIC
  void update_data_available();

  RMW_SWIFTDDS_SHARED_CPP_PUBLIC
  void update_requested_deadline_missed(uint32_t total_count, uint32_t total_count_change);

  RMW_SWIFTDDS_SHARED_CPP_PUBLIC
  void update_liveliness_changed(
    uint32_t alive_count,
    uint32_t not_alive_count,
    uint32_t alive_count_change,
    uint32_t not_alive_count_change);

  RMW_SWIFTDDS_SHARED_CPP_PUBLIC
  void update_sample_lost(uint32_t total_count, uint32_t total_count_change);

  RMW_SWIFTDDS_SHARED_CPP_PUBLIC
  void update_requested_incompatible_qos(
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
  CustomSubscriberInfo *subscriber_info_ = nullptr;

  bool deadline_changed_ RCPPUTILS_TSA_GUARDED_BY(on_new_event_m_);

  greenstone::dds::RequestedDeadlineMissedStatus
  requested_deadline_missed_status_ RCPPUTILS_TSA_GUARDED_BY(on_new_event_m_);

  bool liveliness_changed_ RCPPUTILS_TSA_GUARDED_BY(on_new_event_m_);

  greenstone::dds::LivelinessChangedStatus
  liveliness_changed_status_ RCPPUTILS_TSA_GUARDED_BY(on_new_event_m_);

  bool sample_lost_changed_ RCPPUTILS_TSA_GUARDED_BY(on_new_event_m_);

  greenstone::dds::SampleLostStatus sample_lost_status_ RCPPUTILS_TSA_GUARDED_BY(on_new_event_m_);

  bool incompatible_qos_changed_ RCPPUTILS_TSA_GUARDED_BY(on_new_event_m_);

  greenstone::dds::RequestedIncompatibleQosStatus
  incompatible_qos_status_ RCPPUTILS_TSA_GUARDED_BY(on_new_event_m_);

  bool matched_changes_ RCPPUTILS_TSA_GUARDED_BY(on_new_event_m_);

  greenstone::dds::SubscriptionMatchedStatus
  matched_status_ RCPPUTILS_TSA_GUARDED_BY(on_new_event_m_);

  std::set<greenstone::dds::GUID> publishers_ RCPPUTILS_TSA_GUARDED_BY(publishers_mutex_);

  rmw_event_callback_t on_new_message_cb_{nullptr};

  const void *new_message_user_data_{nullptr};

  std::mutex on_new_message_m_;

  mutable std::mutex publishers_mutex_;

  void trigger_event(rmw_event_type_t event_type);
};

#endif  // RMW_SWIFTDDS_SHARED_CPP__CUSTOM_SUBSCRIBER_INFO_HPP_
