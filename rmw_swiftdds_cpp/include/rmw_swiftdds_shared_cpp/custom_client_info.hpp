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

#ifndef RMW_SWIFTDDS_SHARED_CPP__CUSTOM_CLIENT_INFO_HPP_
#define RMW_SWIFTDDS_SHARED_CPP__CUSTOM_CLIENT_INFO_HPP_

#include <atomic>
#include <memory>
#include <mutex>
#include <set>
#include <utility>
#include <string>

#include "rcpputils/thread_safety_annotations.hpp"

#include "rmw/event_callback_type.h"

#include "rmw_swiftdds_shared_cpp/TypeSupport.hpp"

#include "swiftdds/dcps/SwiftDdsExport.h"

class ClientListener;
class ClientPubListener;

typedef struct CustomClientInfo
{
  std::shared_ptr<greenstone::dds::TopicDataType> request_type_support_{};
  const void *request_type_support_impl_{nullptr};
  std::shared_ptr<greenstone::dds::TopicDataType> response_type_support_{};
  const void *response_type_support_impl_{nullptr};
  greenstone::dds::DataReader *response_reader_{nullptr};
  greenstone::dds::DataWriter *request_writer_{nullptr};

  std::string request_topic_name_;
  std::string response_topic_name_;

  greenstone::dds::Topic *request_topic_{nullptr};
  greenstone::dds::Topic *response_topic_{nullptr};

  ClientListener *listener_{nullptr};
  greenstone::dds::StatusMask listener_mask_{0};
  greenstone::dds::GUID writer_guid_;
  greenstone::dds::GUID reader_guid_;

  const char *typesupport_identifier_{nullptr};
  ClientPubListener *pub_listener_{nullptr};
  greenstone::dds::StatusMask pub_listener_mask_{0};
  std::atomic_size_t response_subscriber_matched_count_;
  std::atomic_size_t request_publisher_matched_count_;
} CustomClientInfo;

typedef struct CustomClientResponse
{
  greenstone::dds::SampleIdentity sample_identity_;
} CustomClientResponse;

class ClientListener : public greenstone::dds::DataReaderListener
{
public:
  explicit ClientListener(CustomClientInfo *info)
  : info_(info)
  {
  }

  void on_data_available(greenstone::dds::DataReader *) noexcept
  {
    std::unique_lock<std::mutex> lock_mutex(on_new_response_m_);

    if(on_new_response_cb_) {
      auto unread_responses = get_unread_responses();

      if(0 < unread_responses) {
        on_new_response_cb_(user_data_, unread_responses);
      }
    }
  }

  void
  on_subscription_matched(
    greenstone::dds::DataReader *reader,
    const greenstone::dds::SubscriptionMatchedStatus & info) noexcept final
  {
    (void)reader;
    if(info_ == nullptr) {
      return;
    }

    greenstone::dds::GUID endpoint_guid;
    endpoint_guid.from_octet16(info.last_publication_handle().value);
    if(info.current_count_change() == 1) {
      publishers_.insert(endpoint_guid);
    } else if(info.current_count_change() == -1) {
      publishers_.erase(endpoint_guid);
    } else {
      return;
    }
    info_->response_subscriber_matched_count_.store(publishers_.size());
  }

  size_t get_unread_responses()
  {
    return info_->response_reader_->get_unread_cache_count(true);
  }

  // Provide handlers to perform an action when a
  // new event from this listener has occurred
  void set_on_new_response_callback(const void *user_data, rmw_event_callback_t callback)
  {
    if(callback) {
      auto unread_responses = get_unread_responses();

      std::lock_guard<std::mutex> lock_mutex(on_new_response_m_);

      if(0 < unread_responses) {
        callback(user_data, unread_responses);
      }

      user_data_ = user_data;
      on_new_response_cb_ = callback;

      info_->listener_mask_ |= greenstone::dds::StatusKind::DATA_AVAILABLE_STATUS;
      info_->response_reader_->set_listener(this, info_->listener_mask_);
    } else {
      std::lock_guard<std::mutex> lock_mutex(on_new_response_m_);

      info_->listener_mask_ &= ~greenstone::dds::StatusKind::DATA_AVAILABLE_STATUS;
      info_->response_reader_->set_listener(this, info_->listener_mask_);

      user_data_ = nullptr;
      on_new_response_cb_ = nullptr;
    }
  }

private:
  CustomClientInfo *info_;

  std::set<greenstone::dds::GUID> publishers_;

  rmw_event_callback_t on_new_response_cb_{nullptr};

  const void *user_data_{nullptr};

  std::mutex on_new_response_m_;
};

class ClientPubListener : public greenstone::dds::DataWriterListener
{
public:
  explicit ClientPubListener(CustomClientInfo *info)
  : info_(info)
  {
  }

  void
  on_publication_matched(
    greenstone::dds::DataWriter *writer,
    const greenstone::dds::PublicationMatchedStatus & info) noexcept final
  {
    (void)writer;
    if(info_ == nullptr) {
      return;
    }
    greenstone::dds::GUID endpoint_guid;
    endpoint_guid.from_octet16(info.last_subscription_handle().value);
    if(info.current_count_change() == 1) {
      subscriptions_.insert(endpoint_guid);
    } else if(info.current_count_change() == -1) {
      subscriptions_.erase(endpoint_guid);
    } else {
      return;
    }
    info_->request_publisher_matched_count_.store(subscriptions_.size());
  }

private:
  CustomClientInfo *info_;
  std::set<greenstone::dds::GUID> subscriptions_;
};

#endif  // RMW_SWIFTDDS_SHARED_CPP__CUSTOM_CLIENT_INFO_HPP_
