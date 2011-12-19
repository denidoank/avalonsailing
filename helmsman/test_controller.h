// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
// Steffen Grundmann, November 2011

// The test controller checks drive response times, maximum speeds
// and sensor functionality.

#ifndef HELMSMAN_TEST_CONTROLLER_H
#define HELMSMAN_TEST_CONTROLLER_H

#include <string>
#include <vector>

#include "common/probe.h"
#include "helmsman/controller.h"
#include "helmsman/sail_controller.h"

using std::vector;
using std::string;

class TestController : public Controller {
 public:
  TestController(SailController* sail_controller);
  virtual ~TestController();
  virtual void Entry(const ControllerInput& in,
                     const FilteredMeasurements& filtered);
  virtual void Run(const ControllerInput& in,
                   const FilteredMeasurements& filtered,
                   ControllerOutput* out);
  virtual bool Done();
  virtual void Exit();
  virtual const char* Name() {
    return "Test";
  };
  
 private:
  enum Phase {
    HOME,          // home the drives, settle the filters.
    ZERO,          // all drives to zero, to have a clean initial state for the drive tests.
    DRIVE_TESTS,   // test response time and maximum speed of all 3 drives
    ZERO_2,        // all drives to zero, to have a clean initial state for the sensor tests.
    WIND_SENSOR,   // plausibilty check of wind sensor data
    FAILED,        // failure terminal state (retry later)
    DONE           // success terminal state, may continue to InitialController.
  };

  void Reset();
  void NextPhase(Phase phase);

  // Parameters to perform an angular position step response test and measure
  // delay and maximum rotational speed.
  enum Drive {RUDDER_LEFT, RUDDER_RIGHT, SAIL};
  typedef struct DriveTestParamStruct {
    Drive drive;        // The reference or desired value for this drive jumps from start_rad to final_rad.
    double start_rad;   // initial and
    double final_rad;   // final reference value.
    double timeout_s;   // timeout for the case of total failures
    const char* name;
  } DriveTestParam;

  void ZeroOut(ControllerOutput* out);
  bool AreDrivesAtZero(const ControllerInput& in);

  bool Until(double fraction_of_timeout);
  bool In(double fraction_of_timeout_from, double fraction_of_timeout_to);
  void SetupTest();
  bool PrepareNextTest();
  bool DoDriveTest(const ControllerInput& in, ControllerOutput* out);
  void StoreTestResults();
  void PrintTestSummary();

  static const char* all_phase_names_[];
  static const DriveTestParam test_param_[];
  // Fractions of the unit jump used to measure response times.
  static const double fractions_[];

  SailController* sail_controller_;
  Phase phase_;
  int count_;  // counts ticks in each phase
  size_t test_index_;  // index into test_param_
  // step response test variables
  double test_start_;  // the value of the reference prior to the step.
  double test_final_;  // the value of the reference after the step.
  char* test_name_;
  int64_t test_time_start_ms_;
  double test_timeout_;
  double test_delta_;   // = test_final_ - test_start_;
  double test_sign_;    // Sign(test_delta_);
  vector<double> test_times_;
  vector<double> thresholds_;
  Probe start_error_;
  Probe final_error_;
  bool test_success_;
  vector<string> test_result_text_;
};

#endif  // HELMSMAN_TEST_CONTROLLER_H
