// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
// Steffen Grundmann, November 2011

#include "helmsman/test_controller.h"

#include <stdio.h>
#include <sys/time.h>

#include "common/delta_angle.h"
#include "common/polar_diagram.h"
#include "common/array_size.h"
#include "common/probe.h"
#include "common/sign.h"
#include "helmsman/boat.h"
#include "helmsman/sampling_period.h"  
#include "helmsman/wind_strength.h"
#include "helmsman/wind.h"

using std::string;

extern int debug;  // global shared

namespace {

const double kZeroTolerance = Deg2Rad(5);
const double kZeroTimeoutCount = 10.0 / kSamplingPeriod;
// This depends on the homing speed (?)
// and the angle range of the rudders (see actuator.c, 150 degrees) .
const double kHomingTimeoutCount = 30.0 / kSamplingPeriod;
const double kRepeatTestAfterFailureCount = 30.0 / kSamplingPeriod;  // TODO SWITCH BACK TO 5 minutes!

uint64_t now_millis() {
  timeval tv;
  CHECK_GE(gettimeofday(&tv, NULL), 0);
  return tv.tv_sec * 1000LL + tv.tv_usec / 1000;
}

}

const char* TestController::all_phase_names_[] = {"HOME",
                                                  "ZERO",
                                                  "DRIVE_TESTS",
                                                  "ZERO_2",
                                                  "WIND_SENSOR",
                                                  "FAILURE",
                                                  "DONE"};

// Our timing resolution is the kSamplingPeriod, so we should move for at least
// 1 s to get less than 10% error from sampling.
const TestController::DriveTestParam TestController::test_param_[] = {
  // drive       start_rad      final_rad  timeout_s   name
  {RUDDER_LEFT,  0,             Deg2Rad( 30), 5,  "rudder left 1"},
  {RUDDER_LEFT,  Deg2Rad( 30),  Deg2Rad(-30), 5,  "rudder left 2"},

  {RUDDER_RIGHT, 0,             Deg2Rad(-30), 5,  "rudder right 1"},
  {RUDDER_RIGHT, Deg2Rad(-30),  Deg2Rad( 30), 5,  "rudder right 2"},

  {SAIL,         0,             Deg2Rad( 30), 6,  "sail 1"},
  {SAIL,         Deg2Rad( 30),  Deg2Rad(-30), 12, "sail 2"}
};

// stay at start for 0.25 timeout
// at t0 : the reference value jumps to final
// t 1: time needed to turn by the first degree.
// t30: Output reaches 30% of delta, t70 t90: ditto.
const double TestController::fractions_[] = {-1, 0.3, 0.7, 0.9};  // first element unused.

TestController::TestController(SailController* sail_controller)
    : sail_controller_(sail_controller),
      test_success_(true) {
  Reset();
}
  
void TestController::TestController::Reset() {
  phase_ = HOME;
  count_ = 0;
}

void TestController::NextPhase(Phase phase) {
  if (debug) fprintf(stderr, "Next phase %s\n", all_phase_names_[phase]);
  phase_ = phase;
  count_ = 0;
  CHECK(strcmp("ZERO_2", all_phase_names_[ZERO_2]) == 0);
  CHECK(strcmp("DONE", all_phase_names_[DONE]) == 0);
}

void TestController::SetupTest() {
  if (debug) fprintf(stderr, "setup testing %s\n", test_param_[test_index_].name);

  //test_name_ = test_param_[test_index_].name;
  test_start_ = test_param_[test_index_].start_rad;
  test_final_ = test_param_[test_index_].final_rad;
  test_time_start_ms_ = -1;
  test_timeout_ = test_param_[test_index_].timeout_s;
  double delta = test_final_ - test_start_;
  test_sign_ = Sign(delta);
  test_times_.clear();
  thresholds_.clear();
  actuals_.clear();
  for (size_t n = 0; n < ARRAY_SIZE(fractions_); ++n) {
    test_times_.push_back(-1);
    actuals_.push_back(-1);
    if (n == 0)
      thresholds_.push_back(test_sign_ * (test_start_ + Deg2Rad(1) * test_sign_));
    else
      thresholds_.push_back(test_sign_ * (test_start_ + fractions_[n] * delta));
  }
  CHECK_GT(test_timeout_, 4);
  CHECK_EQ(4, thresholds_.size());
  CHECK_EQ(4, actuals_.size());
  CHECK_EQ(4, test_times_.size());
  start_error_.Reset();
  final_error_.Reset();
}

bool TestController::Until(double fraction_of_timeout) {
  return count_ < fraction_of_timeout * test_timeout_ / kSamplingPeriod;
}

bool TestController::In(double fraction_of_timeout_from, double fraction_of_timeout_to) {
  return count_ >= fraction_of_timeout_from * test_timeout_ / kSamplingPeriod &&
         count_ <  fraction_of_timeout_to   * test_timeout_ / kSamplingPeriod;
}

void TestController::ZeroOut(ControllerOutput* out) {
  out->drives_reference.gamma_rudder_star_left_rad = 0;
  out->drives_reference.gamma_rudder_star_right_rad = 0;
  out->drives_reference.gamma_sail_star_rad = 0;
}

bool TestController::AreDrivesAtZero(const ControllerInput& in) {
  return ((fabs(in.drives.gamma_rudder_left_rad) < kZeroTolerance ||
           fabs(in.drives.gamma_rudder_right_rad) < kZeroTolerance) &&
          fabs(in.drives.gamma_sail_rad) < kZeroTolerance);
}

bool TestController::PrepareNextTest() {
  count_ = 0;
  if (++test_index_ < ARRAY_SIZE(test_param_)) {
    return true;
  } else {
    test_index_ = 0;
    test_success_ = true;
    return false;
  }
}

bool TestController::DoDriveTest(const ControllerInput& in,
                                 ControllerOutput* out) {
  //test_param_
  if (debug) fprintf(stderr, "testing %s\n", test_param_[test_index_].name);
  // input and output
  double actual = 0;
  double* reference;

  fprintf(stderr, "count %d\n", count_);
  if (count_ <= 1)
     SetupTest();

  switch (test_param_[test_index_].drive) {
    case RUDDER_LEFT:
      actual = in.drives.homed_rudder_left ?
          in.drives.gamma_rudder_left_rad :
          0 ;
      reference = &out->drives_reference.gamma_rudder_star_left_rad;
      break;
    case RUDDER_RIGHT:
      actual = in.drives.homed_rudder_right ?
            in.drives.gamma_rudder_right_rad :
            0;
      reference = &out->drives_reference.gamma_rudder_star_right_rad;
      break;
    case SAIL:
      actual = in.drives.homed_sail ?
            in.drives.gamma_sail_rad :
            0;
      reference = &out->drives_reference.gamma_sail_star_rad;
      break;
    default:
      CHECK(0);
  }
  *reference = test_final_;

  // Test sequence:
  // rest for 1/4 of the timeout,
  // jump to new reference value (final), this should take about 1/2 of the timeout,
  // rest for 1/4 of the timeout.
  if (Until(0.25)) {
    *reference = test_start_;
  } else if (Until(1)) {
    *reference = test_final_;
    if (test_time_start_ms_ < 0)
      test_time_start_ms_ = now_millis();
    // measure times
    for (size_t i = 0; i < thresholds_.size(); ++i) {
      if (test_times_[i] < 0 && actual * test_sign_ > thresholds_[i]) {
        test_times_[i] = now_millis() - test_time_start_ms_;
        actuals_[i] = (actual - test_start_) * test_sign_;
      }
    }
  } else {
    StoreTestResults();
    return PrepareNextTest();
  }

  // measure control error
  if (In(0.2, 0.24)) {
    start_error_.Measure(actual - *reference);
  } else if (In(0.95, 0.99)) {
    final_error_.Measure(actual - *reference);
  }
  return true;
}

// Store and evaluate test results.
void TestController::StoreTestResults() {
  fprintf(stderr, "\n\n\n %s\n", test_param_[test_index_].name);
  for (size_t i = 0; i < thresholds_.size(); ++i) {
    fprintf(stderr, "%6.0lf deg %6.2lfms, %6.2lf deg,\n",
            Rad2Deg(thresholds_[i]), test_times_[i], Rad2Deg(actuals_[i]));
  }
  // Measure response time from start to 1 degree, speed from 30% to 90%.
  double t_response;
  double speed = -1;  // rotational, in deg/s
  if (test_times_[3] - test_times_[1] > 0) {
      speed = Rad2Deg(actuals_[3] - actuals_[1]) /
          ((test_times_[3] - test_times_[1]) / 1000.0);
      // deduct angle bigger than 1 degree.
      t_response = test_times_[0] - 1000 * (Rad2Deg(actuals_[0]) - 1) / speed ;
      // fprintf(stderr, "*** %lf %lf %lf %lf\n", test_times_[0], Rad2Deg(actuals_[0]), speed, t_response);
  } else {
    t_response = test_times_[0];
  }

  // Check test results
  double expected_speed = test_param_[test_index_].drive == SAIL ?
      kOmegaMaxSail:    // 13.8 deg/s 0.241661 rad/s
      kOmegaMaxRudder;  // 30 deg/s 0.523599 rad/s
  bool success = t_response < 400 &&  // 400ms for the first degree
                 speed > 0.8 * expected_speed;  // 20% tolerance (10% from sampling)
  test_success_ = test_success_ && success;

  char buffer[400];
  snprintf(buffer, sizeof(buffer), "%15s: delay: %6.2lfms speed: %6.2lfdeg/s, errors: %lf %lf",
           test_param_[test_index_].name, t_response, speed,
           Rad2Deg(start_error_.Value()), Rad2Deg(final_error_.Value()));
  test_result_text_.push_back(string(buffer));
  fprintf(stderr, "%s\n\n\n", buffer);
}

void TestController::PrintTestSummary() {
  fprintf(stderr, "\nTEST SUMMARY\ntest %s.\n", test_success_ ? "succeeded" : "failed");
  for (int i = 0; i < test_result_text_.size(); ++i)
    fprintf(stderr, "%s\n", test_result_text_[i].c_str());
  test_result_text_.clear();
}


void TestController::Run(const ControllerInput& in,
                         const FilteredMeasurements& filtered,
                         ControllerOutput* out) {

  if (debug) fprintf(stderr, "----TestController::Run-------\n");

  out->Reset();
  if (!filtered.valid) {
    return;
  }
  if (!in.drives.homed_sail ||
      (!in.drives.homed_rudder_left && !in.drives.homed_rudder_right)) {
    if (debug) fprintf(stderr, "Drives not ready\n %s %s %s",
		      in.drives.homed_rudder_left ? "" : "sail",
		      in.drives.homed_rudder_left ? "" : "left",
		      in.drives.homed_rudder_left ? "" : "right");
  }
  
  if (debug) {
    fprintf(stderr, "WindSensor: %s\n", in.wind_sensor.ToString().c_str());
    fprintf(stderr, "DriveActuals: %s\n", in.drives.ToString().c_str());
    fprintf(stderr, "AppFiltered: %fdeg %fm/s\n", Rad2Deg(filtered.angle_app), filtered.mag_app);
  }
  
  /*
  At Buoy:
  homing
  rudder left and right
  sail
  *sail=0, wait 100s, or until true wind is available, then measure for 10s
  - speed == 0
  - angle of attack is around zero
  - wind app alpha = 180 (boat is free to turn)
  - mag app > 0
  - mag app == mag true (speed is zero)
  - wind true alpha == phi_z - 180, stable and fits to real wind
  */

  if (debug) fprintf(stderr, "phase %s\n", all_phase_names_[phase_]);
  switch (phase_) {
    case HOME:
      // 2 drives homed : ok
      // After homing timeout, 1 drive homed, ok, limp along
      //                       0 drive homed, failure
      if (in.drives.homed_sail &&
          in.drives.homed_rudder_left &&
          in.drives.homed_rudder_right) {
        NextPhase(ZERO);
      }
      if (count_ > kHomingTimeoutCount) {
        if (in.drives.homed_sail &&
            (in.drives.homed_rudder_left ||
             in.drives.homed_rudder_right)) {
          NextPhase(ZERO);
          fprintf(stderr, "Limping along with just one rudder drive.");
        } else {
          NextPhase(FAILED);
        }
      }
      ZeroOut(out);
      break;
    case ZERO:
      if (count_ > kZeroTimeoutCount) {
        fprintf(stderr, "Drives do not go to zero after homing, Test controller failed.");
        NextPhase(FAILED);
        break;
      }
      // At least one rudder must work.
      if (AreDrivesAtZero(in)) {
        NextPhase(DRIVE_TESTS);
      }
      ZeroOut(out);
      break;
    case DRIVE_TESTS:
      if (!DoDriveTest(in, out)) {
        NextPhase(ZERO_2);
      }
      break;
    case ZERO_2:
      if (count_ > kZeroTimeoutCount) {
        fprintf(stderr, "Drives do not go to zero after tests, Test controller failed.");
        NextPhase(FAILED);
        break;
      }
      // At least one rudder must work.
      if (AreDrivesAtZero(in)) {
        NextPhase(WIND_SENSOR);
      }
      ZeroOut(out);
      break;
    case WIND_SENSOR:
      PrintTestSummary();
      if (test_success_)
        NextPhase(DONE);
      else
        NextPhase(FAILED);
      break;
    case DONE:
      ZeroOut(out);
      break;
    case FAILED:
      ZeroOut(out);
      // Try again after a while.
      if (count_ > kRepeatTestAfterFailureCount)
        Reset();
      break;
    default:
      CHECK(0);
  }

  if (debug) {
    fprintf(stderr, "out gamma_sail:%lf deg\n",
            Rad2Deg(out->drives_reference.gamma_sail_star_rad));
    fprintf(stderr, "out gamma_rudder_star left/right:%lf deg\n",
            Rad2Deg( out->drives_reference.gamma_rudder_star_right_rad));
  }
  ++count_;
}

TestController::~TestController() {}

void TestController::Entry(const ControllerInput& in,
                           const FilteredMeasurements& filtered) {
  Reset();
}

void TestController::Exit() {}

bool TestController::Done() {
  const bool done = phase_ == DONE &&
                    count_ > 1.0 / kSamplingPeriod;
  if (done && debug) fprintf(stderr, "TestController::Done\n");
  return done;
}
