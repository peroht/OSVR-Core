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

#ifndef INCLUDED_TrackedBodyTarget_h_GUID_E4315530_5C4F_4DB4_2497_11686F0F6E0E
#define INCLUDED_TrackedBodyTarget_h_GUID_E4315530_5C4F_4DB4_2497_11686F0F6E0E

// Internal Includes
#include "Types.h"
#include "BeaconIdTypes.h"

// Library/third-party includes
#include <osvr/Kalman/PureVectorState.h>
#include <boost/assert.hpp>

// Standard includes
#include <vector>

namespace osvr {
namespace vbtracker {
    /// @todo refactor? ported directly
    struct BeaconData {
        bool seen = false;
        double size = 0;
        cv::Point2d measurement = {0, 0};
        cv::Point2d residual = {0, 0};
        double variance = 0;
        void reset() { *this = BeaconData{}; }
    };

    class TrackedBody;
    /// Corresponds to a rigid arrangements of discrete beacons detected by
    /// video-based tracking - typically IR LEDs.
    class TrackedBodyTarget {
      public:
        TrackedBodyTarget(TrackedBody &body,
                          Eigen::Isometry3d const &targetToBody);
        ~TrackedBodyTarget();
        EIGEN_MAKE_ALIGNED_OPERATOR_NEW

        std::vector<BeaconData> const &getBeaconDebugData() const {
            return m_beaconDebugData;
        }

        detail::UnderlyingBeaconIdType getNumBeacons() const {
            return m_beacons.size();
        }

        /// Used to extract autocalibration results in a standalone calibration
        /// app
        Eigen::Vector3d getBeaconAutocalibPosition(ZeroBasedBeaconId i) const;
        /// Used to extract autocalibration results in a standalone calibration
        /// app
        Eigen::Vector3d getBeaconAutocalibVariance(ZeroBasedBeaconId i) const;

      private:
        void m_verifyInvariants() const {
            BOOST_ASSERT_MSG(m_beacons.size() ==
                                 m_beaconMeasurementVariance.size(),
                             "Must have the same number of beacons as default "
                             "measurement variances.");
            BOOST_ASSERT_MSG(
                m_beacons.size() == m_beaconFixed.size(),
                "Must have the same number of beacons as beacon fixed flags.");
            BOOST_ASSERT_MSG(m_beacons.size() ==
                                 m_beaconEmissionDirection.size(),
                             "Must have the same number of beacons as beacon "
                             "emission directions.");
        }
        using BeaconState = kalman::PureVectorState<3>;
        using BeaconStateVec = std::vector<std::unique_ptr<BeaconState>>;

        /// Reference to our parent body.
        TrackedBody &m_body;

        /// Transformation from target coordinate frame to body coordinate
        /// frame.
        Eigen::Isometry3d m_targetToBody;

        /// @name Parallel vectors, one entry per beacon
        /// @brief All should have the same number of values at all times.
        /// @{
        /// Vector of beacon states (with errors - for autocalibration)
        BeaconStateVec m_beacons;
        std::vector<double> m_beaconMeasurementVariance;
        /// Should this beacon be "fixed" (no auto-calibration?)
        std::vector<bool> m_beaconFixed;
        Vec3Vector m_beaconEmissionDirection;
        ///@}
        /// @todo will this always have the same number of entries?
        std::vector<BeaconData> m_beaconDebugData;

        /// private implementation
        struct Impl;
        std::unique_ptr<Impl> m_impl;
    };
} // namespace vbtracker
} // namespace osvr

#endif // INCLUDED_TrackedBodyTarget_h_GUID_E4315530_5C4F_4DB4_2497_11686F0F6E0E
