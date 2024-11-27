#ifndef MOTORS_H
#define MOTORS_H

#include <Arduino.h>

typedef enum {
  SPIN_CONTINUOUS,
  STOP_HANDS,
  MOVE_TO_HOME,
  SET_TIME,
  MIN_ADVANCE,
} MotorCommandType;

typedef struct {
  MotorCommandType type;  // The type of motor command
  int speed;              // Speed for spinning (used for SPIN_CONTINUOUS)
  bool direction;         // Direction (true for forward, false for backward)
  bool proportional;      // Proportional spinning (used for SPIN_PROPORTIONAL)
  int hour;               // Hour for SET_TIME command
  int minute;             // Minute for SET_TIME command
} MotorCommand;

// Configuration for the stepper motors controlling min and HR hands
#define PUL_PIN_HR 22   // Pulse pin connected to PUL+ on DM542 for Hour hand motor
#define PUL_PIN_MIN 18  // Pulse pin connected to PUL+ on DM542 for min hand motor
#define DIR_PIN 19      // Direction pin connected to DIR+ on both DM542

#define PULS_PER_REV 8000  // The number of pulses to make one full revolution
const float GEAR_RATIO_HR = 45.0 / 30.0;  // Shaft gear / Motor Gear
const float GEAR_RATIO_MIN = 40.0 / 20.0; // Shaft gear / Motor Gear
const int HR_STEPS_PER_REV = PULS_PER_REV * GEAR_RATIO_HR; // The number of steps to make one full revolution on the hour hand
const int MIN_STEPS_PER_REV = PULS_PER_REV * GEAR_RATIO_MIN; // The number of steps to make one full revolution on the minute hand

extern volatile int CURRENT_HR_STEPS;
extern volatile int CURRENT_MIN_STEPS;

// Function declarations
void motorControlTask(void* pvParameters);
void pulseMotor(int pulPin, int speedMultiplier);
void updatePos(bool isMinMotor, bool direction);
void spinMotor(bool isMinuteMotor, bool clockwise, int steps, int speedMultiplier);
void spinContinuous(int speed, bool clockwise, bool isProportional);
void spinProportional(int hrSteps, int minSteps, bool clockwise, int maxSpeedMultiplier, bool noRamp = false);
void spinProportionalDuration(int durationMs, bool clockwise, int maxSpeedMultiplier);
int getCurrentHour();
int getCurrentMin() ;
void moveToHome();
void setTime(int hr, int min, int speed=15, int extraRevs=0, bool noRamp=false);
void checkTime();
void correctTimePos(int hr, int min);
void initToHome();
void spinTest();
void timeTest();
void advanceRealMin();


#endif // MOTORS_H