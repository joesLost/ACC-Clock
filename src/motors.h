#ifndef MOTORS_H
#define MOTORS_H`

#include <Arduino.h>

typedef enum {
  SPIN_CONTINUOUS,
  STOP_HANDS,
  MOVE_TO_HOME,
  SET_TIME
} MotorCommandType;

typedef struct {
  MotorCommandType type;  // The type of motor command
  int speed;              // Speed for spinning (used for SPIN_CONTINUOUS)
  bool direction;         // Direction (true for forward, false for backward)
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

volatile int CURRENT_HR_STEPS = HR_STEPS_PER_REV / 2; // Initial position is 6 o'clock
volatile int CURRENT_MIN_STEPS = MIN_STEPS_PER_REV / 2; // Initial position is 30 minutes

void pulseMotor(int pulPin, int speedMultiplier);
void spinMotor(bool isMinuteMotor, bool clockwise, int steps, int speedMultiplier);
void spinProportional(int hrSteps, int minSteps, bool clockwise, int maxSpeedMultiplier);
void spinProportionalDuration(int durationMs, bool clockwise, int maxSpeedMultiplier);
int getCurrentHour();
String getCurrentMin() ;
void moveToHome();
void setTime(int hr, int min);
void checkTime();
void correctTimePosition(int hr, int min);
void initToHome();
void spinTest();
void timeTest();

void spinContinuous(int speed, bool clockwise);




#endif // MOTORS_H