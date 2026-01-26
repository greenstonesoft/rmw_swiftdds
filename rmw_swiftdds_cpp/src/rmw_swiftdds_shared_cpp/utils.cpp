// Copyright 2021 Proyectos y Sistemas de Mantenimiento SL (eProsima).
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

#include <atomic>
#include <cstdint>
#include <string>

#include "rmw_swiftdds_shared_cpp/utils.hpp"

#include "rmw/rmw.h"

#include "swiftdds/dcps/SwiftDdsExport.h"

const char *const CONTENT_FILTERED_TOPIC_POSTFIX = "_filtered_name";

namespace rmw_swiftdds_shared_cpp
{

rmw_ret_t cast_error_dds_to_rmw(greenstone::dds::ReturnCode_t code)
{
  // not switch because it is not an enum class
  if(greenstone::dds::ReturnCode_t::RETCODE_OK == code) {
    return RMW_RET_OK;
  } else if(greenstone::dds::ReturnCode_t::RETCODE_ERROR == code) {
    // repeats the error to avoid too many 'if' comparisons
    return RMW_RET_ERROR;
  } else if(greenstone::dds::ReturnCode_t::RETCODE_TIMEOUT == code) {
    return RMW_RET_TIMEOUT;
  } else if(greenstone::dds::ReturnCode_t::RETCODE_UNSUPPORTED == code) {
    return RMW_RET_UNSUPPORTED;
  } else if(greenstone::dds::ReturnCode_t::RETCODE_BAD_PARAMETER == code) {
    return RMW_RET_INVALID_ARGUMENT;
  } else if(greenstone::dds::ReturnCode_t::RETCODE_OUT_OF_RESOURCES == code) {
    // Could be that out of resources comes from a different source than a bad
    // allocation
    return RMW_RET_BAD_ALLOC;
  } else {
    return RMW_RET_ERROR;
  }
}

bool find_and_check_topic_and_type(
  const CustomParticipantInfo *participant_info,
  const std::string & topic_name,
  const std::string & type_name,
  greenstone::dds::TopicDescription **returned_topic,
  std::shared_ptr<greenstone::dds::TopicDataType> returned_type)
{
  // Searchs for an already existing topic
  *returned_topic = participant_info->participant_->lookup_topicdescription(topic_name);
  if(nullptr != *returned_topic) {
    if((*returned_topic)->get_type_name() != type_name) {
      return false;
    }
  }

  // NOTE(methylDragon): This only finds a type that's been previously
  // registered to the participant
  returned_type->set_name(type_name.c_str());
  participant_info->participant_->register_type(returned_type.get());
  return true;
}

void remove_topic_and_type(
  CustomParticipantInfo *participant_info,
  EventListenerInterface *event_listener,
  const greenstone::dds::TopicDescription *topic_desc,
  greenstone::dds::TopicDataType *type)
{
  // TODO(MiguelCompany): We only create Topic instances at the moment, but this
  // may change in the future if we start supporting other kinds of
  // TopicDescription (like ContentFilteredTopic)
  auto topic = dynamic_cast<const greenstone::dds::Topic *>(topic_desc);

  if(nullptr != topic) {
    participant_info->delete_topic(topic, type->type_identifier(), event_listener);
  }

  if(type) {
    participant_info->participant_->unregister_type(type->get_type_name(), type->type_identifier());
  }
}

bool create_content_filtered_topic(
  greenstone::dds::DomainParticipant *participant,
  greenstone::dds::TopicDescription *topic_desc,
  const std::string & topic_name_mangled,
  const rmw_subscription_content_filter_options_t *options,
  greenstone::dds::ContentFilteredTopic **content_filtered_topic)
{
  std::vector<std::string> expression_parameters;
  for(size_t i = 0; i < options->expression_parameters.size; ++i) {
    expression_parameters.push_back(options->expression_parameters.data[i]);
  }

  auto topic = dynamic_cast<greenstone::dds::Topic *>(topic_desc);
  static std::atomic<uint32_t> cft_counter{0};
  std::string cft_topic_name = topic_name_mangled + CONTENT_FILTERED_TOPIC_POSTFIX + "_" +
    std::to_string(cft_counter.fetch_add(1));
  greenstone::dds::ContentFilteredTopic *filtered_topic = participant->create_contentfilteredtopic(
      cft_topic_name, topic, options->filter_expression, expression_parameters);
  if(filtered_topic == nullptr) {
    return false;
  }

  *content_filtered_topic = filtered_topic;
  return true;
}

bool create_datareader(
  const greenstone::dds::DataReaderQos & datareader_qos,
  const rmw_subscription_options_t *subscription_options,
  greenstone::dds::Subscriber *subscriber,
  greenstone::dds::TopicDescription *des_topic,
  CustomDataReaderListener *listener,
  greenstone::dds::StatusMask mask,
  greenstone::dds::DataReader **data_reader)
{
  greenstone::dds::DataReaderQos updated_qos = datareader_qos;
  switch(subscription_options->require_unique_network_flow_endpoints) {
    default:
    case RMW_UNIQUE_NETWORK_FLOW_ENDPOINTS_SYSTEM_DEFAULT:
    case RMW_UNIQUE_NETWORK_FLOW_ENDPOINTS_NOT_REQUIRED:
    // Unique network flow endpoints not required. We leave the decission to
    // the XML profile.
      break;

    case RMW_UNIQUE_NETWORK_FLOW_ENDPOINTS_OPTIONALLY_REQUIRED:
    case RMW_UNIQUE_NETWORK_FLOW_ENDPOINTS_STRICTLY_REQUIRED: {
    // Ensure we request unique network flow endpoints
        using PropertyPolicyHelper = greenstone::dds::PropertyPolicyHelper;
        if(std::string("") == PropertyPolicyHelper::find_property(updated_qos.property(),
                                                              "swiftdds.unique_network_flows"))
        {
          greenstone::dds::PropertyQosPolicy tempProperty = updated_qos.property();
          gstone::Property property_data("swiftdds.unique_network_flows", "", true);
          tempProperty.add_property(property_data);
          updated_qos.property(tempProperty);
        }
        break;
      }
  }

  // Creates DataReader (with subscriber name to not change name policy)
  *data_reader = subscriber->create_datareader(
      des_topic, updated_qos, listener, mask);
  if(!(*data_reader) && (RMW_UNIQUE_NETWORK_FLOW_ENDPOINTS_OPTIONALLY_REQUIRED ==
    subscription_options->require_unique_network_flow_endpoints))
  {
    greenstone::dds::DataReaderQos reader_qos = datareader_qos;
    *data_reader = subscriber->create_datareader(
        des_topic, reader_qos, listener, mask);
  }

  if(!(*data_reader)) {
    return false;
  }

  return true;
}

void apply_qos_resource_limits_for_keys(
  const greenstone::dds::HistoryQosPolicy & history_qos,
  greenstone::dds::ResourceLimitsQosPolicy & res_limits_qos)
{
  // res_limits_qos.max_instances = dds::qos::LENGTH_UNLIMITED;
  // res_limits_qos.max_samples = dds::qos::LENGTH_UNLIMITED;
  // if (history_qos.kind ==
  // greenstone::dds::HistoryQosPolicyKind::KEEP_LAST_HISTORY_QOS) {
  //   res_limits_qos.max_samples_per_instance = history_qos.depth;
  // } else {
  //   res_limits_qos.max_samples_per_instance = dds::qos::LENGTH_UNLIMITED;
  // }
}

}  // namespace rmw_swiftdds_shared_cpp
