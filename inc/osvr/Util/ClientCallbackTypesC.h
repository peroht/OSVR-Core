/** @file
    @brief Header

    Must be c-safe!

    @date 2014

    @author
    Sensics, Inc.
    <http://sensics.com/osvr>
*/

/*
// Copyright 2014 Sensics, Inc.
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
*/

#ifndef INCLUDED_ClientCallbackTypesC_h_GUID_4D43A675_C8A4_4BBF_516F_59E6C785E4EF
#define INCLUDED_ClientCallbackTypesC_h_GUID_4D43A675_C8A4_4BBF_516F_59E6C785E4EF

/* Internal Includes */
#include <osvr/Util/APIBaseC.h>
#include <osvr/Util/ReturnCodesC.h>
#include <osvr/Util/Pose3C.h>
#include <osvr/Util/TimeValueC.h>
#include <osvr/Util/StdInt.h>

/* Library/third-party includes */
/* none */

/* Standard includes */
/* none */

OSVR_EXTERN_C_BEGIN

/** @addtogroup ClientKit
    @{
*/

#define OSVR_DEFINE_CLIENT_CALLBACK_TYPE(TYPE)                                 \
    typedef void (*OSVR_##TYPE##Callback)(                                     \
        void *userdata, const struct OSVR_TimeValue *timestamp,                \
        const struct OSVR_##TYPE##Report *report)

/** @brief C function type for a position callback on a tracker interface */
typedef void (*OSVR_PositionCallback)(void *userdata,
                                      const struct OSVR_TimeValue *timestamp,
                                      const struct OSVR_PositionReport *report);

/** @brief C function type for an orientation callback on a tracker interface */
typedef void (*OSVR_OrientationCallback)(
    void *userdata, const struct OSVR_TimeValue *timestamp,
    const struct OSVR_OrientationReport *report);

/** @brief C function type for a pose (position and orientation) callback on a
    tracker interface
*/
typedef void (*OSVR_PoseCallback)(void *userdata,
                                  const struct OSVR_TimeValue *timestamp,
                                  const struct OSVR_PoseReport *report);

/** @brief C function type for a callback on a button interface
*/
typedef void (*OSVR_ButtonCallback)(void *userdata,
                                    const struct OSVR_TimeValue *timestamp,
                                    const struct OSVR_ButtonReport *report);

/** @brief C function type for a callback on an analog interface */
OSVR_DEFINE_CLIENT_CALLBACK_TYPE(Analog);

/** @brief C function type for a callback on an location2D interface */
OSVR_DEFINE_CLIENT_CALLBACK_TYPE(Location2D);

/** @brief C function type for a callback on an location2D interface */
OSVR_DEFINE_CLIENT_CALLBACK_TYPE(Direction);

/** @brief C function type for a callback for EyeTracker2D */
OSVR_DEFINE_CLIENT_CALLBACK_TYPE(EyeTracker2D);

/** @brief C function type for a callback for EyeTracker3D */
OSVR_DEFINE_CLIENT_CALLBACK_TYPE(EyeTracker3D);

/** @brief C function type for a callback for EyeTrackerBlink */
OSVR_DEFINE_CLIENT_CALLBACK_TYPE(EyeTrackerBlink);

/** @brief C function type for a callback for NavigationVelocity */
OSVR_DEFINE_CLIENT_CALLBACK_TYPE(NaviVelocity);

/** @brief C function type for a callback for NavigationPosition */
OSVR_DEFINE_CLIENT_CALLBACK_TYPE(NaviPosition);

/** @brief C function type for a callback for Gesture event */
OSVR_DEFINE_CLIENT_CALLBACK_TYPE(Gesture);

#undef OSVR_DEFINE_CALLBACK
/** @} */
OSVR_EXTERN_C_END

#endif
