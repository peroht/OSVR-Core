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

#ifndef INCLUDED_TrackedBody_h_GUID_1FC60169_196A_4F89_551C_E1B41531BBC8
#define INCLUDED_TrackedBody_h_GUID_1FC60169_196A_4F89_551C_E1B41531BBC8

// Internal Includes
#include "ConfigParams.h"
#include "BodyIdTypes.h"

// Library/third-party includes
#include <osvr/Util/EigenCoreGeometry.h>

// Standard includes
#include <memory>

namespace osvr {
namespace vbtracker {
    class TrackingSystem;
    class TrackedBodyIMU;
    class TrackedBodyTarget;

    /// This is the class representing a tracked rigid body in the system. It
    /// may be tracked by one (or eventually more) video-based "target"
    /// (constellation of beacons in a known pattern with other known traits),
    /// and optionally by an IMU/AHRS - an orientation/angular-velocity-only
    /// high speed sensor.
    ///
    /// This class has overall state
    class TrackedBody {
      public:
        /// Constructor
        TrackedBody(TrackingSystem &system);
        /// Destructor - explicit so we can use unique_ptr for our pimpls.
        ~TrackedBody();
        /// Noncopyable
        TrackedBody(TrackedBody const &) = delete;
        /// Non-copy-assignable
        TrackedBody &operator=(TrackedBody const &) = delete;

        /// Creates a fully-integrated IMU data source (that is, one that
        /// reports a reliable quaternion, and potentially angular velocity) to
        /// add to this body.
        ///
        /// @todo eventually fix: Right now assumes that there is only one IMU
        /// per body
        /// @todo assumes the IMU is at the origin of the body (most important
        /// for velocity)
        ///
        /// You do not own the pointer you get back - the tracked body does.
        ///
        /// @return nullptr if an error occurred (such as an IMU already being
        /// added to this body)
        TrackedBodyIMU *createIntegratedIMU();

        /// Creates a video-based tracking target (constellation of beacons) to
        /// add to this body.
        ///
        /// @todo eventually fix: Right now assumes that there is only one
        /// target per body
        /// @todo assumes the target is at the origin of the body
        ///
        /// You do not own the pointer you get back - the tracked body does.
        ///
        /// @return nullptr if an error occurred (such as a target already being
        /// added to this body)
        TrackedBodyTarget *createTarget(Eigen::Isometry3d const &targetToBody);

        /// Gets the body ID within the tracking system.
        BodyId getId() const;

        /// @todo refactor
        ConfigParams const &getParams() const;

      private:
        TrackingSystem &m_system;
        /// private implementation data
        struct ImplData;
        std::unique_ptr<ImplData> m_data;
        std::unique_ptr<TrackedBodyIMU> m_imu;
        std::unique_ptr<TrackedBodyTarget> m_target;
    };
} // namespace vbtracker
} // namespace osvr

#endif // INCLUDED_TrackedBody_h_GUID_1FC60169_196A_4F89_551C_E1B41531BBC8
