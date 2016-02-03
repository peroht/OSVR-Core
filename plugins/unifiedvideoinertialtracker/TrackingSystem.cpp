/** @file
    @brief Implementation

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

// Internal Includes
#include "TrackingSystem.h"
#include "TrackedBody.h"
#include "TrackedBodyTarget.h"
#include "SBDBlobExtractor.h"
#include "UndistortMeasurements.h"
#include "ForEachTracked.h"

// Library/third-party includes
#include <boost/assert.hpp>
#include <util/Stride.h>

#include <opencv2/highgui/highgui.hpp>

// Standard includes
#include <algorithm>
#include <iterator>
#include <iostream>
#include <string>
#include <stdexcept>

namespace osvr {
namespace vbtracker {
    enum class DebugDisplayMode { InputImage, Thresholding, Blobs };
    static const auto DEBUG_WINDOW_NAME = "OSVR Tracker Debug Window";
    static const auto DEBUG_FRAME_STRIDE = 11;
    class TrackingDebugDisplay {
      public:
        TrackingDebugDisplay(ConfigParams const &params)
            : m_enabled(params.debug), m_debugStride(DEBUG_FRAME_STRIDE) {
            if (!m_enabled) {
                return;
            }
            // cv::namedWindow(m_windowName);

            std::cout << "\nVideo-based tracking debug windows help:\n";
            std::cout
#if 0
                << "  - press 's' to show the detected blobs and the status of "
                   "recognized beacons (default)\n"
#endif
                << "  - press 'b' to show the labeled blobs and the "
                   "reprojected beacons\n"
                << "  - press 'i' to show the raw input image\n"
                << "  - press 't' to show the blob-detecting threshold image\n"
#if 0
                << "  - press 'p' to dump the current auto-calibrated beacon "
                   "positions to a CSV file\n"
#endif
                << "  - press 'q' to quit the debug windows (tracker will "
                   "continue operation)\n"
                << std::endl;
        }

        TrackingDebugDisplay(TrackingDebugDisplay const &) = delete;
        TrackingDebugDisplay &operator=(TrackingDebugDisplay const &) = delete;

        void displayNewFrame(TrackingSystem::Impl const &impl);

        void showDebugImage(cv::Mat const &image) {
            image.copyTo(m_displayedFrame);
            cv::imshow(m_windowName, m_displayedFrame);
        }

        void quitDebug() {
            if (!m_enabled) {
                return;
            }
            cv::destroyWindow(m_windowName);
            m_enabled = false;
        }

      private:
        bool m_enabled;
        DebugDisplayMode m_mode = DebugDisplayMode::Blobs;
        std::string m_windowName = DEBUG_WINDOW_NAME;
        cv::Mat m_displayedFrame;
        ::util::Stride m_debugStride;
    };

    struct TrackingSystem::Impl {
        Impl(ConfigParams const &params)
            : blobExtractor(new SBDBlobExtractor(params)),
              debugDisplay(params) {}

        void sendFrameToDebugDisplay() { debugDisplay.displayNewFrame(*this); }

        /// @name Cached data from the ImageProcessingOutput updated in phase 2
        /// @{
        /// Cached copy of the last grey frame
        cv::Mat frame;
        /// Cached copy of the last grey frame
        cv::Mat frameGray;
        /// Cached copy of the last (undistorted) camera parameters to be used.
        CameraParameters camParams;
        util::time::TimeValue lastFrame;
        /// @}

        LedUpdateCount updateCount;
        std::unique_ptr<SBDBlobExtractor> blobExtractor;
        TrackingDebugDisplay debugDisplay;
    };

    void
    TrackingDebugDisplay::displayNewFrame(TrackingSystem::Impl const &impl) {
        if (!m_enabled) {
            /// We're not displaying things.
            return;
        }

        m_debugStride.advance();
        if (!m_debugStride) {
            /// not our turn.
            return;
        }

        /// Update the display
        switch (m_mode) {
        case DebugDisplayMode::InputImage:
            showDebugImage(impl.frame);
            break;
        case DebugDisplayMode::Thresholding:
            showDebugImage(impl.blobExtractor->getDebugThresholdImage());
            break;
        case DebugDisplayMode::Blobs:
            showDebugImage(impl.blobExtractor->getDebugBlobImage());
            break;
        }

        /// Run the event loop briefly to see if there were keyboard presses.
        int key = cv::waitKey(1);
        switch (key) {
#if 0
        case 's':
        case 'S':
            // Show the concise "status" image (default)
            /// @todo
            m_mode = DebugDisplayMode::Status;
            break;
#endif

        case 'b':
        case 'B':
            // Show the blob/keypoints image
            std::cout << "'b' pressed - Switching to the blobs image in the "
                         "debug window."
                      << std::endl;
            m_mode = DebugDisplayMode::Blobs;
            break;

        case 'i':
        case 'I':
            // Show the input image.
            std::cout << "'i' pressed - Switching to the input image in the "
                         "debug window."
                      << std::endl;
            m_mode = DebugDisplayMode::InputImage;
            break;

        case 't':
        case 'T':
            // Show the thresholded image
            std::cout << "'t' pressed - Switching to the thresholded image in "
                         "the debug window."
                      << std::endl;
            m_mode = DebugDisplayMode::Thresholding;
            break;

#if 0
        case 'p':
        case 'P':
            // Dump the beacon positions to file.
            /// @todo
            break;
#endif

        case 'q':
        case 'Q':
            // Close the debug window.
            std::cout << "'q' pressed - quitting the debug window."
                      << std::endl;
            quitDebug();
            break;

        default:
            // something else or nothing at all, no worries.
            break;
        }
    }

    TrackingSystem::TrackingSystem(ConfigParams const &params)
        : m_params(params), m_impl(new Impl(params)) {}

    TrackingSystem::~TrackingSystem() {}

    TrackedBody *TrackingSystem::createTrackedBody() {
        auto newId = BodyId(m_bodies.size());
        BodyPtr newBody(new TrackedBody(*this, newId));
        m_bodies.emplace_back(std::move(newBody));
        return m_bodies.back().get();
    }

    TrackedBodyTarget *TrackingSystem::getTarget(BodyTargetId target) {
        return getBody(target.first).getTarget(target.second);
    }

    ImageOutputDataPtr TrackingSystem::performInitialImageProcessing(
        util::time::TimeValue const &tv, cv::Mat const &frame,
        cv::Mat const &frameGray, CameraParameters const &camParams) {

        ImageOutputDataPtr ret(new ImageProcessingOutput);
        ret->tv = tv;
        ret->frame = frame;
        ret->frameGray = frameGray;
        ret->camParams = camParams.createUndistortedVariant();
        auto rawMeasurements =
            m_impl->blobExtractor->extractBlobs(ret->frameGray);
        ret->ledMeasurements = undistortLeds(rawMeasurements, camParams);
        return ret;
    }

    LedUpdateCount const &
    TrackingSystem::updateLedsFromVideoData(ImageOutputDataPtr &&imageData) {
        /// Clear internal data, we're invalidating things here.
        m_updated.clear();
        auto &updateCount = m_impl->updateCount;
        updateCount.clear();

        /// Update our frame cache, since we're taking ownership of the image
        /// data now.
        m_impl->frame = imageData->frame;
        m_impl->frameGray = imageData->frameGray;
        m_impl->camParams = imageData->camParams;

        /// Go through each target and try to process the measurements.
        forEachTarget(*this, [&](TrackedBodyTarget &target) {
            auto usedMeasurements =
                target.processLedMeasurements(imageData->ledMeasurements);
            if (usedMeasurements != 0) {
                updateCount[target.getQualifiedId()] = usedMeasurements;
            }
        });
        return updateCount;
    }

    BodyIndices const &
    TrackingSystem::updateBodiesFromVideoData(ImageOutputDataPtr &&imageData) {
        /// Do the second phase of stuff
        updateLedsFromVideoData(std::move(imageData));

        /// Do the third phase of tracking.
        updatePoseEstimates();

        /// Trigger debug display, if activated.
        m_impl->sendFrameToDebugDisplay();

        return m_updated;
    }

    void TrackingSystem::updatePoseEstimates() {

        auto const &updateCount = m_impl->updateCount;
        for (auto &bodyTargetWithMeasurements : updateCount) {
            auto targetPtr = getTarget(bodyTargetWithMeasurements.first);
            BOOST_ASSERT_MSG(targetPtr != nullptr, "We should never be "
                                                   "retrieving a nullptr for a "
                                                   "target with measurements!");
            if (!targetPtr) {
                throw std::logic_error("Logical impossibility: Couldn't "
                                       "retrieve a valid pointer for a target "
                                       "that we were just told updated its "
                                       "LEDs from data this frame.");
            }
            auto &target = *targetPtr;

            /// @todo call into the target here.
            auto gotPose = target.updatePoseEstimateFromLeds(m_impl->camParams);

            /// @todo deduplicate in making this list.
            m_updated.push_back(target.getBody().getId());
        }
    }

    bool TrackedBody::hasPoseEstimate() const {
        /// @todo handle IMU here.
        auto ret = false;
        forEachTarget([&ret](TrackedBodyTarget &target) {
            ret = ret || target.hasPoseEstimate();
        });
        return ret;
    }

} // namespace vbtracker
} // namespace osvr
