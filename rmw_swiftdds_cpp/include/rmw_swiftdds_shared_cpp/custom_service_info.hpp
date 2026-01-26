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

#ifndef RMW_SWIFTDDS_SHARED_CPP__CUSTOM_SERVICE_INFO_HPP_
#define RMW_SWIFTDDS_SHARED_CPP__CUSTOM_SERVICE_INFO_HPP_

#include <condition_variable>
#include <mutex>
#include <unordered_set>
#include <unordered_map>
#include <memory>

#include "rcpputils/thread_safety_annotations.hpp"

#include "rmw/event_callback_type.h"

#include "rmw_swiftdds_shared_cpp/guid_utils.hpp"
#include "rmw_swiftdds_shared_cpp/TypeSupport.hpp"
#include "swiftdds/dcps/SwiftDdsExport.h"

class ServiceListener;
class ServicePubListener;

enum class client_present_t
{
  FAILURE,  // an error occurred when checking
  MAYBE,    // reader not matched, writer still present
  YES,      // reader matched
  GONE      // neither reader nor writer
};

typedef struct CustomServiceInfo
{
  std::shared_ptr<greenstone::dds::TopicDataType> request_type_support_{};
  const void *request_type_support_impl_{nullptr};
  std::shared_ptr<greenstone::dds::TopicDataType> response_type_support_{};
  const void *response_type_support_impl_{nullptr};
  greenstone::dds::DataReader *request_reader_{nullptr};
  greenstone::dds::DataWriter *response_writer_{nullptr};

  greenstone::dds::Topic *request_topic_{nullptr};
  greenstone::dds::Topic *response_topic_{nullptr};

  ServiceListener *listener_{nullptr};
  greenstone::dds::StatusMask listener_mask_{0};
  ServicePubListener *pub_listener_{nullptr};
  greenstone::dds::StatusMask pub_listener_mask_{0};

  const char *typesupport_identifier_{nullptr};
} CustomServiceInfo;

typedef struct CustomServiceRequest
{
  greenstone::dds::SampleIdentity sample_identity_;
} CustomServiceRequest;

class ServicePubListener : public greenstone::dds::DataWriterListener
{
  using subscriptions_set_t =
    std::unordered_set<greenstone::dds::GUID, rmw_swiftdds_shared_cpp::hash_swiftdds_guid>;
  using clients_endpoints_map_t = std::unordered_map<greenstone::dds::GUID,
      greenstone::dds::GUID,
      rmw_swiftdds_shared_cpp::hash_swiftdds_guid>;

public:
  explicit ServicePubListener(CustomServiceInfo *info)
  {
    (void)info;
  }

  void
  on_publication_matched(
    greenstone::dds::DataWriter *writer,
    const greenstone::dds::PublicationMatchedStatus & info) noexcept final
  {
    (void)writer;
    std::lock_guard<std::mutex> lock(mutex_);
    greenstone::dds::GUID endpoint_guid;
    endpoint_guid.from_octet16(info.last_subscription_handle().value);
    if(info.current_count_change() == 1) {
      subscriptions_.insert(endpoint_guid);
    } else if(info.current_count_change() == -1) {
      subscriptions_.erase(endpoint_guid);
    } else {
      return;
    }
    cv_.notify_all();
  }

  template<class Rep, class Period>
  bool wait_for_subscription(
    const greenstone::dds::GUID & guid,
    const std::chrono::duration<Rep, Period> & rel_time)
  {
    auto guid_is_present = [this, guid]() RCPPUTILS_TSA_REQUIRES(mutex_)->bool {
      return subscriptions_.find(guid) != subscriptions_.end();
    };

    std::unique_lock<std::mutex> lock(mutex_);
    return cv_.wait_for(lock, rel_time, guid_is_present);
  }

private:
  std::mutex mutex_;
  subscriptions_set_t subscriptions_ RCPPUTILS_TSA_GUARDED_BY(mutex_);
  std::condition_variable cv_;
};

class ServiceListener : public greenstone::dds::DataReaderListener
{
public:
  explicit ServiceListener(CustomServiceInfo *info)
  : info_(info)
  {
  }

  void
  on_subscription_matched(
    greenstone::dds::DataReader *reader,
    const greenstone::dds::SubscriptionMatchedStatus & info) noexcept final
  {
    (void)reader;
    (void)info;
  }

  size_t get_unread_resquests()
  {
    return info_->request_reader_->get_unread_cache_count(true);
  }

  void on_data_available(greenstone::dds::DataReader *) noexcept final
  {
    std::unique_lock<std::mutex> lock_mutex(on_new_request_m_);

    auto unread_requests = get_unread_resquests();

    if(0u < unread_requests) {
      on_new_request_cb_(user_data_, unread_requests);
    }
  }

  // Provide handlers to perform an action when a
  // new event from this listener has occurred
  void set_on_new_request_callback(const void *user_data, rmw_event_callback_t callback)
  {
    if(callback) {
      auto unread_requests = get_unread_resquests();
      std::lock_guard<std::mutex> lock_mutex(on_new_request_m_);

      if(0 < unread_requests) {
        callback(user_data, unread_requests);
      }

      user_data_ = user_data;
      on_new_request_cb_ = callback;

      info_->listener_mask_ |= greenstone::dds::StatusKind::DATA_AVAILABLE_STATUS;
      info_->request_reader_->set_listener(this, info_->listener_mask_);
    } else {
      std::lock_guard<std::mutex> lock_mutex(on_new_request_m_);

      info_->listener_mask_ &= ~greenstone::dds::StatusKind::DATA_AVAILABLE_STATUS;
      info_->request_reader_->set_listener(this, info_->listener_mask_);

      user_data_ = nullptr;
      on_new_request_cb_ = nullptr;
    }
  }

private:
  CustomServiceInfo *info_;

  rmw_event_callback_t on_new_request_cb_{nullptr};

  const void *user_data_{nullptr};

  std::mutex on_new_request_m_;
};

#endif  // RMW_SWIFTDDS_SHARED_CPP__CUSTOM_SERVICE_INFO_HPP_
