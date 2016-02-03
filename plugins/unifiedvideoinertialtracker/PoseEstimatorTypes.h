/** @file
    @brief Header

    @date 2016

    @author
    Sensics, Inc.
    <http://sensics.com/osvr>
*/

// Copyright 2016 Sensics, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//        http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef INCLUDED_PoseEstimatorTypes_h_GUID_A9E792C9_4128_4B84_2108_DD3BA8B88061
#define INCLUDED_PoseEstimatorTypes_h_GUID_A9E792C9_4128_4B84_2108_DD3BA8B88061

// Internal Includes
#include "ModelTypes.h"
#include "CameraParameters.h"

// Library/third-party includes
// - none

// Standard includes
#include <vector>

namespace osvr {
namespace vbtracker {
    class Led;
    using LedPtrList = std::vector<Led *>;
} // namespace vbtracker
} // namespace osvr

#endif // INCLUDED_PoseEstimatorTypes_h_GUID_A9E792C9_4128_4B84_2108_DD3BA8B88061
