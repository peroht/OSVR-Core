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
#include "PoseEstimator_SCAATKalman.h"
#include "ImagePointMeasurement.h"
#include "LED.h"
#include "cvToEigen.h"

// Library/third-party includes
#include <osvr/Kalman/FlexibleKalmanFilter.h>
#include <osvr/Kalman/AugmentedProcessModel.h>
#include <osvr/Kalman/AugmentedState.h>
#include <osvr/Kalman/ConstantProcess.h>

#include <util/Stride.h>

// Standard includes
#include <algorithm>
#include <iostream>

namespace osvr {
namespace vbtracker {
    static const auto DIM_BEACON_CUTOFF_TO_SKIP_BRIGHTS = 4;
    static const auto BRIGHT_PENALTY = 8.;
    SCAATKalmanPoseEstimator::SCAATKalmanPoseEstimator(
        ConfigParams const &params)
        : m_shouldSkipBright(params.shouldSkipBrightLeds),
          m_maxSquaredResidual(params.maxResidual * params.maxResidual),
          m_maxZComponent(params.maxZComponent),
          m_highResidualVariancePenalty(params.highResidualVariancePenalty),
          m_beaconProcessNoise(params.beaconProcessNoise),
          m_measurementVarianceScaleFactor(
              params.measurementVarianceScaleFactor),
          m_extraVerbose(params.extraVerbose) {
        std::tie(m_minBoxRatio, m_maxBoxRatio) =
            std::minmax({params.boundingBoxFilterRatio,
                         1.f / params.boundingBoxFilterRatio});

        const auto maxSquaredResidual = params.maxResidual * params.maxResidual;
    }
    bool SCAATKalmanPoseEstimator::operator()(CameraParameters const &camParams,
                                              LedPtrList const &leds,
                                              double videoDt,
                                              EstimatorInOutParams const &p) {
        bool gotMeasurement = false;
        double varianceFactor = 1;

        const auto inBoundsID = leds.size();
        // Default to using all the measurements we can
        auto skipBright = false;

        {
            auto inBoundsBright = std::size_t{0};
            auto inBoundsRound = std::size_t{0};

            /// Count up types of beacons
            for (auto const &ledPtr : leds) {
                auto &led = *ledPtr;
                if (led.isBright()) {
                    inBoundsBright++;
                }

                auto boundingBoxRatioResult = inBoundingBoxRatioRange(led);
                if (TriBool::True == boundingBoxRatioResult) {
                    inBoundsRound++;
                }
            }

            /// Decisions based on the counts.
            if (inBoundsID - inBoundsBright >
                    DIM_BEACON_CUTOFF_TO_SKIP_BRIGHTS &&
                m_shouldSkipBright) {
                skipBright = true;
            }
            if (0 == inBoundsID) {
                m_framesWithoutIdentifiedBlobs++;
            } else {
                m_framesWithoutIdentifiedBlobs = 0;
            }
        }

        CameraModel cam;
        cam.focalLength = camParams.focalLength();
        cam.principalPoint = camParams.eiPrincipalPoint();
        ImagePointMeasurement meas{cam};

        kalman::ConstantProcess<kalman::PureVectorState<>> beaconProcess;

        /// Prediction for overall state moved into the body itself

        /// @todo should we be recalculating this for each beacon after each
        /// correction step? The order we filter them in is rather arbitrary...
        Eigen::Matrix3d rotate =
            Eigen::Matrix3d(p.state.getCombinedQuaternion());
        auto numBad = std::size_t{0};
        auto numGood = std::size_t{0};
        static ::util::Stride s{203};
        s.advance();
        for (auto &ledPtr : leds) {
            auto &led = *ledPtr;

            auto id = led.getID();
            auto index = asIndex(id);

            auto &debug = p.beaconDebug[index];
            debug.seen = true;
            debug.measurement = led.getLocation();
            if (skipBright && led.isBright()) {
                continue;
            }
            // Angle of emission checking
            // If we transform the body-local emission vector, an LED pointed
            // right at the camera will be -Z. Anything with a 0 or positive z
            // component is clearly out, and realistically, anything with a z
            // component over -0.5 is probably fairly oblique. We don't want to
            // use such beacons since they can easily introduce substantial
            // error.
            double zComponent =
                (rotate * cvToVector(p.beaconEmissionDirection[index])).z();
            if (zComponent > 0.) {
                if (m_extraVerbose) {
                    std::cout << "Rejecting an LED at " << led.getLocation()
                              << " claiming ID " << led.getOneBasedID().value()
                              << std::endl;
                }
                /// This means the LED is pointed away from us - so we shouldn't
                /// be able to see it.
                led.markMisidentified();

                /// @todo This could be a mis-identification, or it could mean
                /// we're in a totally messed up state. Do we count this against
                /// ourselves?
                numBad++;
                continue;
            } else if (zComponent > m_maxZComponent) {
                /// LED is too askew of the camera to provide reliable data, so
                /// skip it.
                continue;
            }

#if 0
            /// @todo For right now, if we don't have a bounding box, we're
            /// assuming it's square enough (and only testing for
            /// non-squareness on those who actually do have bounding
            /// boxes). This is very much a temporary situation.
            auto boundingBoxRatioResult = inBoundingBoxRatioRange(led);
            if (TriBool::False == boundingBoxRatioResult) {
                /// skip non-circular blobs.
                numBad++;
                continue;
            }
#endif

            auto localVarianceFactor = varianceFactor;
            auto newIdentificationVariancePenalty =
                std::pow(2.0, led.novelty());

            /// Stick a little bit of process model uncertainty in the beacon,
            /// if it's meant to have some
            if (p.beaconFixed[asIndex(id)]) {
                beaconProcess.setNoiseAutocorrelation(0);
            } else {
                beaconProcess.setNoiseAutocorrelation(m_beaconProcessNoise);
                kalman::predict(*(p.beacons[index]), beaconProcess, videoDt);
            }

            meas.setMeasurement(
                Eigen::Vector2d(led.getLocation().x, led.getLocation().y));
            led.markAsUsed();
            auto state =
                kalman::makeAugmentedState(p.state, *(p.beacons[index]));
            meas.updateFromState(state);
            Eigen::Vector2d residual = meas.getResidual(state);
            if (residual.squaredNorm() > m_maxSquaredResidual) {
                // probably bad
                numBad++;
                localVarianceFactor *= m_highResidualVariancePenalty;
            } else {
                numGood++;
            }
            debug.residual.x = residual.x();
            debug.residual.y = residual.y();
            auto effectiveVariance =
                localVarianceFactor * m_measurementVarianceScaleFactor *
                newIdentificationVariancePenalty *
                (led.isBright() ? BRIGHT_PENALTY : 1.) *
                p.beaconMeasurementVariance[index] / led.getMeasurement().area;
            debug.variance = effectiveVariance;
            meas.setVariance(effectiveVariance);

            /// Now, do the correction.
            auto model = kalman::makeAugmentedProcessModel(p.processModel,
                                                           beaconProcess);
            kalman::correct(state, model, meas);
            gotMeasurement = true;

            if (s) {
                std::cout << "M: " << debug.measurement
                          << "  R: " << debug.residual
                          << "  s2: " << debug.variance << "\n";
            }
        }

        /// Probation: Dealing with ratios of bad to good residuals
        bool incrementProbation = false;
        if (0 == m_framesInProbation) {
            // Let's try to keep a 3:2 ratio of good to bad when not "in
            // probation"
            incrementProbation = (numBad * 3 > numGood * 2);

        } else {
            // Already in trouble, add a bit of hysteresis and raising the bar
            // so we don't hop out easily.
            incrementProbation = numBad * 2 > numGood;
            if (!incrementProbation) {
                // OK, we're good again
                m_framesInProbation = 0;
            }
        }
        if (incrementProbation) {
            m_framesInProbation++;
        }

        /// Frames without measurements: dealing with getting in a bad state
        if (gotMeasurement) {
            m_framesWithoutUtilizedMeasurements = 0;
        } else {
            if (inBoundsID > 0) {
                /// We had a measurement, we rejected it. The problem may be the
                /// plank in our own eye, not the speck in our beacon's eye.
                m_framesWithoutUtilizedMeasurements++;
            }
        }
        return true;
    }
    SCAATKalmanPoseEstimator::TriBool
    SCAATKalmanPoseEstimator::inBoundingBoxRatioRange(Led const &led) {
        if (led.getMeasurement().knowBoundingBox) {
            auto boundingBoxRatio = led.getMeasurement().boundingBox.height /
                                    led.getMeasurement().boundingBox.width;
            if (boundingBoxRatio > m_minBoxRatio &&
                boundingBoxRatio < m_maxBoxRatio) {
                return TriBool::True;
            }
            return TriBool::False;
        }
        return TriBool::Unknown;
    }

    // The total number of frames that we can have dodgy Kalman tracking for
    // before RANSAC takes over again.
    static const std::size_t MAX_PROBATION_FRAMES = 10;

    static const std::size_t MAX_FRAMES_WITHOUT_MEASUREMENTS = 50;

    static const std::size_t MAX_FRAMES_WITHOUT_ID_BLOBS = 10;

    SCAATKalmanPoseEstimator::TrackingHealth
    SCAATKalmanPoseEstimator::getTrackingHealth() {

        auto needsReset = (m_framesInProbation > MAX_PROBATION_FRAMES) ||
                          (m_framesWithoutUtilizedMeasurements >
                           MAX_FRAMES_WITHOUT_MEASUREMENTS);
        if (needsReset) {
            return TrackingHealth::NeedsResetNow;
        }
        /// Additional case: We've been a while without blobs...
        if (m_framesWithoutIdentifiedBlobs > MAX_FRAMES_WITHOUT_ID_BLOBS) {
            // In this case, we force ransac once we get blobs once again.
            return TrackingHealth::ResetWhenBeaconsSeen;
        }
        /// Otherwise, we're doing OK!
        return TrackingHealth::Functioning;
    }
} // namespace vbtracker
} // namespace osvr
