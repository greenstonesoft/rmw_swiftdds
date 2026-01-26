// Copyright 2023 Open Source Robotics Foundation, Inc.
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

#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <utility>

#include "rcutils/logging_macros.h"

#include "rmw_swiftdds_shared_cpp/custom_participant_info.hpp"

#include "swiftdds/dcps/SwiftDdsExport.h"

CustomTopicListener::CustomTopicListener(EventListenerInterface *event_listener)
{
  add_event_listener(event_listener);
}

void CustomTopicListener::on_inconsistent_topic(
  greenstone::dds::Topic *topic,
  greenstone::dds::InconsistentTopicStatus const & status) noexcept
{
  if(topic == nullptr) {
    return;
  }

  std::lock_guard<std::mutex> lck(event_listeners_mutex_);
  for(EventListenerInterface *listener : event_listeners_) {
    listener->update_inconsistent_topic(status.total_count(), status.total_count_change());
  }
}

void CustomTopicListener::add_event_listener(EventListenerInterface *event_listener)
{
  // We allow for null event listeners; we will just never report on them
  if(event_listener == nullptr) {
    return;
  }

  std::lock_guard<std::mutex> lck(event_listeners_mutex_);
  event_listeners_.insert(event_listener);
}

void CustomTopicListener::remove_event_listener(EventListenerInterface *event_listener)
{
  if(event_listener == nullptr) {
    return;
  }

  std::lock_guard<std::mutex> lck(event_listeners_mutex_);
  event_listeners_.erase(event_listener);
}

greenstone::dds::Topic *
CustomParticipantInfo::find_or_create_topic(
  const std::string & topic_name,
  const std::string & type_name,
  const std::string & type_identifier,
  const greenstone::dds::TopicQos & topic_qos,
  EventListenerInterface *event_listener)
{
  greenstone::dds::Topic *topic = nullptr;

  std::lock_guard<std::mutex> lck(topic_name_to_topic_mutex_);
  std::string const topic_map_key {topic_name + ":" + type_identifier};
  std::map<std::string, std::unique_ptr<UseCountTopic>>::const_iterator it =
    topic_name_to_topic_.find(topic_map_key);
  if(it == topic_name_to_topic_.end()) {
    // Not already in the map, we need to add it
    auto uct = std::make_unique<UseCountTopic>();
    uct->topic_listener = new CustomTopicListener(event_listener);
    topic = participant_->create_topic(topic_name, type_name, type_identifier, topic_qos,
      uct->topic_listener);
    uct->use_count = 1;
    uct->topic = topic;

    topic_name_to_topic_[topic_map_key] = std::move(uct);
  } else {
    // Already in the map, just increase the use count
    it->second->use_count++;
    it->second->topic_listener->add_event_listener(event_listener);
    topic = it->second->topic;
  }

  return topic;
}

void CustomParticipantInfo::delete_topic(
  const greenstone::dds::Topic *topic,
  const std::string & type_identifier,
  EventListenerInterface *event_listener)
{
  if(topic == nullptr) {
    return;
  }

  std::lock_guard<std::mutex> lck(topic_name_to_topic_mutex_);
  std::string const topic_map_key {topic->get_name() + ":" + type_identifier};
  std::map<std::string, std::unique_ptr<UseCountTopic>>::const_iterator it =
    topic_name_to_topic_.find(topic_map_key);

  if(it != topic_name_to_topic_.end()) {
    it->second->use_count--;
    it->second->topic_listener->remove_event_listener(event_listener);
    // Really we only need to check if use_count == 0, but just be cautious and
    // do a <= check.
    if(it->second->use_count <= 0) {
      participant_->delete_topic(it->second->topic);

      delete it->second->topic_listener;

      topic_name_to_topic_.erase(it);
    }
  } else {
    RCUTILS_LOG_WARN_NAMED("rmw_swiftdds_shared_cpp",
                           "Attempted to delete topic '%s', but it was never created.  Ignoring",
                           topic->get_name().c_str());
  }
}
