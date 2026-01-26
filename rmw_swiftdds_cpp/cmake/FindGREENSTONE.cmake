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

find_path(GREENSTONE_INCLUDE_DIR
  NAMES swiftdds/dcps/SwiftDdsExport.h
  PATHS
    ${PROJECT_SOURCE_DIR}/lib/include
    /usr/include
  DOC "DDS header file directory"
)

find_library(GREENSTONE_LIBRARIES
  NAMES
    greenstone-DCPS
  PATHS
    ${PROJECT_SOURCE_DIR}/lib
    /usr/lib
  DOC "DDS core library"
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(GREENSTONE
  DEFAULT_MSG
  GREENSTONE_LIBRARIES
  GREENSTONE_INCLUDE_DIR
)

set(GREENSTONE-DCPS_INCLUDE_DIRS ${GREENSTONE_INCLUDE_DIR})
set(GREENSTONE-DCPS_LIBRARIES ${GREENSTONE_LIBRARIES})

mark_as_advanced(GREENSTONE-DCPS_LIBRARIES GREENSTONE-DCPS_INCLUDE_DIRS)
