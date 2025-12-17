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

#ifndef RMW_SWIFTDDS_SHARED_CPP__QOS_CONFIG_HPP_
#define RMW_SWIFTDDS_SHARED_CPP__QOS_CONFIG_HPP_

#include <mutex>

class QosConfig final
{
private:
  QosConfig() = default;
  QosConfig(const QosConfig &) = delete;
  QosConfig & operator=(const QosConfig &) = delete;

public:
  static QosConfig & getInstance() {
    static QosConfig instance;
    return instance;
  }

  std::mutex qosConfig_mutex{};
  bool send_sync{false};
};
#endif  // RMW_SWIFTDDS_SHARED_CPP__QOS_CONFIG_HPP_
