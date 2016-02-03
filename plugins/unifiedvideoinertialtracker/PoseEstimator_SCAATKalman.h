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

#ifndef INCLUDED_PoseEstimator_SCAATKalman_h_GUID_F1FC2154_E59B_4598_70C2_253F8EA31485
#define INCLUDED_PoseEstimator_SCAATKalman_h_GUID_F1FC2154_E59B_4598_70C2_253F8EA31485

// Internal Includes
#include "ConfigParams.h"
#include "PoseEstimatorTypes.h"

// Library/third-party includes
// - none

// Standard includes
// - none

namespace osvr {
namespace vbtracker {
    class SCAATKalmanPoseEstimator {
      public:
        SCAATKalmanPoseEstimator(ConfigParams const &params);
        bool operator()(CameraParameters const &camParams,
                        LedPtrList const &leds, BeaconStateVec &beacons,
                        BodyState &state);
    };
} // namespace vbtracker
} // namespace osvr

#endif // INCLUDED_PoseEstimator_SCAATKalman_h_GUID_F1FC2154_E59B_4598_70C2_253F8EA31485
