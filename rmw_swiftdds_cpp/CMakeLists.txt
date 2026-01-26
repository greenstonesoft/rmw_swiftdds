# Copyright 2016-2018 Proyectos y Sistemas de Mantenimiento SL (eProsima).
# Copyright 2025 GreenStoneSoft, Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

cmake_minimum_required(VERSION 3.20)

project(rmw_swiftdds_cpp)

# Default to C++17
if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 17)
  set(CMAKE_CXX_STANDARD_REQUIRED ON)
endif()

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

if(SECURITY)
  find_package(OpenSSL REQUIRED)
endif()

find_package(ament_cmake_ros_core REQUIRED)

find_package(rcpputils REQUIRED)
find_package(rcutils REQUIRED)
find_package(rmw_dds_common REQUIRED)
find_package(rmw_security_common REQUIRED)
find_package(tracetools REQUIRED)

find_package(rmw REQUIRED)
find_package(rosidl_runtime_c REQUIRED)
find_package(rosidl_typesupport_introspection_c REQUIRED)
find_package(rosidl_typesupport_introspection_cpp REQUIRED)

find_package(nlohmann_json REQUIRED)
find_package(GREENSTONE REQUIRED)

set(ENABLE_SANITIZER OFF)
if(CMAKE_BUILD_TYPE MATCHES "Debug")
  if(ENABLE_SANITIZER)
      message(STATUS "enable sanitizer : ON")
      set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-omit-frame-pointer -fsanitize=address,undefined")
      set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fno-omit-frame-pointer -fsanitize=address,undefined")
  endif()
endif()

add_library(${PROJECT_NAME}
  # base
  src/rmw_swiftdds_cpp/get_client.cpp
  src/rmw_swiftdds_cpp/get_participant.cpp
  src/rmw_swiftdds_cpp/get_publisher.cpp
  src/rmw_swiftdds_cpp/get_service.cpp
  src/rmw_swiftdds_cpp/get_subscriber.cpp
  src/rmw_swiftdds_cpp/identifier.cpp
  src/rmw_swiftdds_cpp/init_rmw_context_impl.cpp
  src/rmw_swiftdds_cpp/publisher.cpp
  src/rmw_swiftdds_cpp/rmw_logging.cpp
  src/rmw_swiftdds_cpp/rmw_client.cpp
  src/rmw_swiftdds_cpp/rmw_compare_gids_equal.cpp
  src/rmw_swiftdds_cpp/rmw_count.cpp
  src/rmw_swiftdds_cpp/rmw_dynamic_message_type_support.cpp
  src/rmw_swiftdds_cpp/rmw_event.cpp
  src/rmw_swiftdds_cpp/rmw_features.cpp
  src/rmw_swiftdds_cpp/rmw_get_gid_for_client.cpp
  src/rmw_swiftdds_cpp/rmw_get_gid_for_publisher.cpp
  src/rmw_swiftdds_cpp/rmw_get_implementation_identifier.cpp
  src/rmw_swiftdds_cpp/rmw_get_serialization_format.cpp
  src/rmw_swiftdds_cpp/rmw_get_service_endpoint_info.cpp
  src/rmw_swiftdds_cpp/rmw_get_topic_endpoint_info.cpp
  src/rmw_swiftdds_cpp/rmw_guard_condition.cpp
  src/rmw_swiftdds_cpp/rmw_init.cpp
  src/rmw_swiftdds_cpp/rmw_node.cpp
  src/rmw_swiftdds_cpp/rmw_node_info_and_types.cpp
  src/rmw_swiftdds_cpp/rmw_node_names.cpp
  src/rmw_swiftdds_cpp/rmw_publish.cpp
  src/rmw_swiftdds_cpp/rmw_publisher.cpp
  src/rmw_swiftdds_cpp/rmw_qos.cpp
  src/rmw_swiftdds_cpp/rmw_request.cpp
  src/rmw_swiftdds_cpp/rmw_response.cpp
  src/rmw_swiftdds_cpp/rmw_serialize.cpp
  src/rmw_swiftdds_cpp/rmw_service.cpp
  src/rmw_swiftdds_cpp/rmw_service_names_and_types.cpp
  src/rmw_swiftdds_cpp/rmw_service_server_is_available.cpp
  src/rmw_swiftdds_cpp/rmw_subscription.cpp
  src/rmw_swiftdds_cpp/rmw_take.cpp
  src/rmw_swiftdds_cpp/rmw_topic_names_and_types.cpp
  src/rmw_swiftdds_cpp/rmw_trigger_guard_condition.cpp
  src/rmw_swiftdds_cpp/rmw_wait.cpp
  src/rmw_swiftdds_cpp/rmw_wait_set.cpp
  src/rmw_swiftdds_cpp/serialization_format.cpp
  src/rmw_swiftdds_cpp/subscription.cpp
  src/rmw_swiftdds_cpp/rmw_get_endpoint_network_flow.cpp
  # shared
  src/rmw_swiftdds_shared_cpp/custom_participant_info.cpp
  src/rmw_swiftdds_shared_cpp/custom_publisher_info.cpp
  src/rmw_swiftdds_shared_cpp/custom_subscriber_info.cpp
  src/rmw_swiftdds_shared_cpp/create_rmw_gid.cpp
  src/rmw_swiftdds_shared_cpp/demangle.cpp
  src/rmw_swiftdds_shared_cpp/init_rmw_context_impl.cpp
  src/rmw_swiftdds_shared_cpp/listener_thread.cpp
  src/rmw_swiftdds_shared_cpp/namespace_prefix.cpp
  src/rmw_swiftdds_shared_cpp/participant.cpp
  src/rmw_swiftdds_shared_cpp/publisher.cpp
  src/rmw_swiftdds_shared_cpp/qos.cpp
  src/rmw_swiftdds_shared_cpp/rmw_client.cpp
  src/rmw_swiftdds_shared_cpp/rmw_compare_gids_equal.cpp
  src/rmw_swiftdds_shared_cpp/rmw_count.cpp
  src/rmw_swiftdds_shared_cpp/rmw_event.cpp
  src/rmw_swiftdds_shared_cpp/rmw_features.cpp
  src/rmw_swiftdds_shared_cpp/rmw_get_endpoint_network_flow.cpp
  src/rmw_swiftdds_shared_cpp/rmw_get_gid_for_client.cpp
  src/rmw_swiftdds_shared_cpp/rmw_get_gid_for_publisher.cpp
  src/rmw_swiftdds_shared_cpp/rmw_get_service_endpoint_info.cpp
  src/rmw_swiftdds_shared_cpp/rmw_get_topic_endpoint_info.cpp
  src/rmw_swiftdds_shared_cpp/rmw_guard_condition.cpp
  src/rmw_swiftdds_shared_cpp/rmw_init.cpp
  src/rmw_swiftdds_shared_cpp/rmw_logging.cpp
  src/rmw_swiftdds_shared_cpp/rmw_node.cpp
  src/rmw_swiftdds_shared_cpp/rmw_node_info_and_types.cpp
  src/rmw_swiftdds_shared_cpp/rmw_node_names.cpp
  src/rmw_swiftdds_shared_cpp/rmw_publish.cpp
  src/rmw_swiftdds_shared_cpp/rmw_publisher.cpp
  src/rmw_swiftdds_shared_cpp/rmw_qos.cpp
  src/rmw_swiftdds_shared_cpp/rmw_request.cpp
  src/rmw_swiftdds_shared_cpp/rmw_response.cpp
  src/rmw_swiftdds_shared_cpp/rmw_security_logging.cpp
  src/rmw_swiftdds_shared_cpp/rmw_service.cpp
  src/rmw_swiftdds_shared_cpp/rmw_service_names_and_types.cpp
  src/rmw_swiftdds_shared_cpp/rmw_service_server_is_available.cpp
  src/rmw_swiftdds_shared_cpp/rmw_subscription.cpp
  src/rmw_swiftdds_shared_cpp/rmw_take.cpp
  src/rmw_swiftdds_shared_cpp/rmw_topic_names_and_types.cpp
  src/rmw_swiftdds_shared_cpp/rmw_trigger_guard_condition.cpp
  src/rmw_swiftdds_shared_cpp/rmw_wait.cpp
  src/rmw_swiftdds_shared_cpp/rmw_wait_set.cpp
  src/rmw_swiftdds_shared_cpp/subscription.cpp
  src/rmw_swiftdds_shared_cpp/time_utils.cpp
  src/rmw_swiftdds_shared_cpp/TypeSupport_impl.cpp
  src/rmw_swiftdds_shared_cpp/utils.cpp
)

target_include_directories(${PROJECT_NAME} PRIVATE ${GREENSTONE-DCPS_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} PRIVATE ${GREENSTONE-DCPS_LIBRARIES} nlohmann_json::nlohmann_json)

target_link_libraries(${PROJECT_NAME} PUBLIC
  rmw::rmw
  rosidl_runtime_c::rosidl_runtime_c
)
target_link_libraries(${PROJECT_NAME} PRIVATE
  rcpputils::rcpputils
  rcutils::rcutils
  rmw_dds_common::rmw_dds_common_library
  rosidl_typesupport_introspection_c::rosidl_typesupport_introspection_c
  rosidl_typesupport_introspection_cpp::rosidl_typesupport_introspection_cpp
  tracetools::tracetools
)
target_include_directories(${PROJECT_NAME} PUBLIC
  "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
  "$<INSTALL_INTERFACE:include/${PROJECT_NAME}>")

configure_rmw_library(${PROJECT_NAME})

# Causes the visibility macros to use dllexport rather than dllimport,
# which is appropriate when building the dll but not consuming it.

# Export old-style CMake variables
ament_export_include_directories("include/${PROJECT_NAME}")
ament_export_libraries(${PROJECT_NAME})
# Export a modern CMake target
ament_export_targets(${PROJECT_NAME})

ament_export_dependencies(rmw)
ament_export_dependencies(rosidl_runtime_c)
ament_export_dependencies(rosidl_typesupport_introspection_c)
ament_export_dependencies(rosidl_typesupport_introspection_cpp)

register_rmw_implementation(
  "c:rosidl_typesupport_introspection_c"
  "cpp:rosidl_typesupport_introspection_cpp")

if(BUILD_TESTING AND (NOT WIN32))
  find_package(ament_lint_auto REQUIRED)
  ament_lint_auto_find_test_dependencies()

  find_package(ament_cmake_gtest REQUIRED)
  find_package(osrf_testing_tools_cpp REQUIRED)
  find_package(test_msgs REQUIRED)

  ament_add_gtest(test_get_native_entities
    test/test_get_native_entities.cpp)
  target_link_libraries(test_get_native_entities
    osrf_testing_tools_cpp::memory_tools
    rcutils::rcutils
    rmw::rmw
    ${PROJECT_NAME}
    ${test_msgs_TARGETS}
    ${GREENSTONE-DCPS_LIBRARIES}
  )
  target_include_directories(test_get_native_entities PRIVATE ${GREENSTONE-DCPS_INCLUDE_DIRS})

  ament_add_gtest(test_logging test/test_logging.cpp)
  target_link_libraries(test_logging
    rmw::rmw
    ${PROJECT_NAME}
    ${GREENSTONE-DCPS_LIBRARIES}
  )
  target_include_directories(test_logging PRIVATE ${GREENSTONE-DCPS_INCLUDE_DIRS})
endif()

ament_package()

install(
  DIRECTORY include/
  DESTINATION include/${PROJECT_NAME}
)

install(
  TARGETS ${PROJECT_NAME} EXPORT ${PROJECT_NAME}
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
  RUNTIME DESTINATION bin
)
