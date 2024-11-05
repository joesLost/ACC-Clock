#include "motors.h"
#include "globals.h" 


void motorControlTask(void *pvParameters) {
  MotorCommand cmd;
  bool isSpinning = false;
  bool spinDirection = true; // Default spin direction
  int spinSpeed = 0;         // Default speed

  while (true) {
    // Check if there is a new command in the queue
    if (xQueueReceive(motorCommandQueue, &cmd, 0) == pdPASS) {
      switch (cmd.type) {
        case SPIN_CONTINUOUS:
          isSpinning = true;
          spinSpeed = cmd.speed;
          spinDirection = cmd.direction;
          break;
        case STOP_HANDS:
          isSpinning = false;
          xQueueReset(motorCommandQueue);
          break;
        case MOVE_TO_HOME:
          isSpinning = false;
          moveToHome();
          break;
        case SET_TIME:
          isSpinning = false;
          setTime(cmd.hour, cmd.minute);
          break;
      }
    }
    // If we are in spinning mode, keep the motor running
    if (isSpinning) {
      spinContinuous(spinSpeed, spinDirection);
    }
    // Add a small delay to yield CPU time to other tasks
    vTaskDelay(10 / portTICK_PERIOD_MS);
  }
}


void pulseMotor(int pulPin, int speedMultiplier) {
  int delayTime = max(1000 / speedMultiplier, 10);
  digitalWrite(pulPin, HIGH);
  delayMicroseconds(20);
  digitalWrite(pulPin, LOW);
  delayMicroseconds(delayTime);
}

void spinMotor(bool isMinuteMotor, bool clockwise, int steps, int speedMultiplier) {
  int pulPin = isMinuteMotor ? PUL_PIN_MIN : PUL_PIN_HR;

  digitalWrite(DIR_PIN, clockwise ? LOW : HIGH);
  Serial.println(steps);
  for (int i = 0; i < steps; i++) {
    pulseMotor(pulPin, speedMultiplier);

    // Update the current position
    if (isMinuteMotor) {
      if (clockwise) {
        CURRENT_MIN_STEPS = (CURRENT_MIN_STEPS + 1) % MIN_STEPS_PER_REV;
      } else {
        CURRENT_MIN_STEPS = (CURRENT_MIN_STEPS - 1 + MIN_STEPS_PER_REV) % MIN_STEPS_PER_REV;
      }
    } else {
      if (clockwise) {
        CURRENT_HR_STEPS = (CURRENT_HR_STEPS + 1) % HR_STEPS_PER_REV;
      } else {
        CURRENT_HR_STEPS = (CURRENT_HR_STEPS - 1 + HR_STEPS_PER_REV) % HR_STEPS_PER_REV;
      }
    }
  }
}

void spinContinuous(int speed, bool clockwise) {

  digitalWrite(DIR_PIN, clockwise ? LOW : HIGH);

  pulseMotor(PUL_PIN_MIN, speed);
  pulseMotor(PUL_PIN_HR, speed);

  // Update the current position
  if (clockwise) {
    CURRENT_MIN_STEPS = (CURRENT_MIN_STEPS + 1) % MIN_STEPS_PER_REV;
    CURRENT_HR_STEPS = (CURRENT_HR_STEPS + 1) % HR_STEPS_PER_REV;
  } else {
    CURRENT_MIN_STEPS = (CURRENT_MIN_STEPS - 1 + MIN_STEPS_PER_REV) % MIN_STEPS_PER_REV;
    CURRENT_HR_STEPS = (CURRENT_HR_STEPS - 1 + HR_STEPS_PER_REV) % HR_STEPS_PER_REV;
  }
}

void spinProportional(int minSteps, int hrSteps, bool clockwise, int maxSpeedMultiplier) {
  int totalSteps = max(minSteps, hrSteps);
  int rampUpSteps = totalSteps / 15;
  int rampDownSteps = totalSteps / 15;

  int currentSpeed = 1;
  int minCounter = 0;
  int hrCounter = 0;

  digitalWrite(DIR_PIN, clockwise ? LOW : HIGH);

  for (int step = 0; step < totalSteps; step++) {

    if (step < rampUpSteps) {
      currentSpeed = map(step, 0, rampUpSteps, 1, maxSpeedMultiplier);
    } else if (step > totalSteps - rampDownSteps) {
      currentSpeed = map(step, totalSteps - rampDownSteps, totalSteps, maxSpeedMultiplier, 1);
    } else {
      currentSpeed = maxSpeedMultiplier;
    }


    if (minCounter < minSteps) {
      pulseMotor(PUL_PIN_MIN, currentSpeed);
      minCounter++;
      if (clockwise) {
        CURRENT_MIN_STEPS = (CURRENT_MIN_STEPS + 1) % MIN_STEPS_PER_REV;
      } else {
        CURRENT_MIN_STEPS = (CURRENT_MIN_STEPS - 1 + MIN_STEPS_PER_REV) % MIN_STEPS_PER_REV;
      }
    }


    if (hrCounter < hrSteps && minCounter % 12 == 0) {
      pulseMotor(PUL_PIN_HR, currentSpeed);
      hrCounter++;
      if (clockwise) {
        CURRENT_HR_STEPS = (CURRENT_HR_STEPS + 1) % HR_STEPS_PER_REV;
      } else {
        CURRENT_HR_STEPS = (CURRENT_HR_STEPS - 1 + HR_STEPS_PER_REV) % HR_STEPS_PER_REV;
      }
    }
  }
}

void spinProportionalDuration(int durationMs, bool clockwise, int maxSpeedMultiplier) {
  unsigned long endTime = millis() + durationMs;
  digitalWrite(DIR_PIN, clockwise ? LOW : HIGH);

  int currentSpeed = 1;
  int rampUpSteps = 1000;
  int rampDownSteps = 1000;
  int totalSteps = durationMs * maxSpeedMultiplier;
  int minCounter = 0;

  for (int step = 0; step < totalSteps; step++) {

    if (step < rampUpSteps) {
      currentSpeed = map(step, 0, rampUpSteps, 1, maxSpeedMultiplier);
    } else if (step > totalSteps - rampDownSteps) {
      currentSpeed = map(step, totalSteps - rampDownSteps, totalSteps, maxSpeedMultiplier, 1);
    } else {
      currentSpeed = maxSpeedMultiplier;
    }

    pulseMotor(PUL_PIN_MIN, currentSpeed);

    minCounter++;
    if (minCounter % 12 == 0) {
      pulseMotor(PUL_PIN_HR, currentSpeed);
    }

    // Update the current position
    if (clockwise) {
        CURRENT_MIN_STEPS = (CURRENT_MIN_STEPS + 1) % MIN_STEPS_PER_REV;
        if (minCounter % 12 == 0) {
            CURRENT_HR_STEPS = (CURRENT_HR_STEPS + 1) % HR_STEPS_PER_REV;
        }
    } else {
        CURRENT_MIN_STEPS = (CURRENT_MIN_STEPS - 1 + MIN_STEPS_PER_REV) % MIN_STEPS_PER_REV;
        if (minCounter % 12 == 0) {
            CURRENT_HR_STEPS = (CURRENT_HR_STEPS - 1 + HR_STEPS_PER_REV) % HR_STEPS_PER_REV;
        }
    }
  }
}

int getCurrentHour() {
  int hour = (CURRENT_HR_STEPS % HR_STEPS_PER_REV) / (HR_STEPS_PER_REV / 12);
  return (hour == 0 ? 12 : hour);
}
String getCurrentMin() {
  int minute = (CURRENT_MIN_STEPS % MIN_STEPS_PER_REV) / (MIN_STEPS_PER_REV / 60);
  return String(minute < 10 ? "0" : "") + String(minute);
}

void checkTime() {
  Serial.print("Current Time: ");
  Serial.print(getCurrentHour());
  Serial.print(":");
  Serial.println(getCurrentMin());
  Serial.println("Current Steps: ");
  Serial.print("Hour: ");
  Serial.print(CURRENT_HR_STEPS);
  Serial.print(" Min: ");
  Serial.println(CURRENT_MIN_STEPS);
}

void moveToHome() {
  Serial.println("Moving to home position...");
  // Calculate the steps needed to move to the home position (12 o'clock)
  int hrStepsToHome = (0 - CURRENT_HR_STEPS + HR_STEPS_PER_REV) % HR_STEPS_PER_REV;
  int minStepsToHome = (0 - CURRENT_MIN_STEPS + MIN_STEPS_PER_REV) % MIN_STEPS_PER_REV;

  spinMotor(false, true, hrStepsToHome, 3);
  spinMotor(true, true, minStepsToHome, 3);
  // Update the current positions
  CURRENT_HR_STEPS = 0;
  CURRENT_MIN_STEPS = 0;
}

void setTime(int hr, int min) {
  Serial.print("Prior time: ");
  Serial.print(getCurrentHour());
  Serial.print(":");
  Serial.print(getCurrentMin());
  Serial.print(" Setting time to ");
  Serial.print(hr);
  Serial.print(":");
  Serial.println(min);

  int targetHrSteps = map(hr, 0, 12, 0, HR_STEPS_PER_REV);
  int targetMinSteps = map(min, 0, 60, 0, MIN_STEPS_PER_REV);
  int hrSteps = (targetHrSteps - CURRENT_HR_STEPS + HR_STEPS_PER_REV) % HR_STEPS_PER_REV;
  int minSteps = (targetMinSteps - CURRENT_MIN_STEPS + MIN_STEPS_PER_REV) % MIN_STEPS_PER_REV;

  spinProportional(minSteps, hrSteps, false, 15);

  Serial.println("Steps to target: ");
  Serial.print("Hour: ");
  Serial.print(targetHrSteps);
  Serial.print(" Min: ");
  Serial.println(targetMinSteps);
  CURRENT_HR_STEPS = targetHrSteps;
  CURRENT_MIN_STEPS = targetMinSteps;
}

void correctTimePosition(int hr, int min) {
  int hrSteps = map(hr, 0, 12, 0, HR_STEPS_PER_REV);
  int minSteps = map(min, 0, 59, 0, MIN_STEPS_PER_REV);
  CURRENT_HR_STEPS = hrSteps;
  CURRENT_MIN_STEPS = minSteps;
  moveToHome();
}

void initToHome() {
  Serial.println("Moving to home position...");
  spinMotor(true, true, MIN_STEPS_PER_REV / 2 , 3);
  spinMotor(false, true, HR_STEPS_PER_REV / 2, 3);
  CURRENT_HR_STEPS = 0;
  CURRENT_MIN_STEPS = 0;
}

void spinTest() {
  Serial.println("Starting spin test");
  for(int i = 10; i <=100; i+=10){ //0-59
    Serial.print("Spin time 5s. Speed: ");
    Serial.println(i);
    spinProportionalDuration(5000, true, i);
    delay(1000);
  }
}

void timeTest() {
  Serial.println("Starting time test");
  for (int h = 1; h<= 12; h++){ //1-12
    for(int m = 4; m <=59; m+=5){ //0-59
      setTime(h,m);
      delay(1000);
    }
  }
}

void advanceRealMinute() {
  static unsigned long lastUpdateTime = 0;
  unsigned long currentTime = millis();
  if (currentTime - lastUpdateTime >= 60000) { // Check if a minute has passed
      int minSteps = MIN_STEPS_PER_REV / 60;
      int hrSteps = HR_STEPS_PER_REV / (60 * 12);
      spinProportional(minSteps, hrSteps, true, 10);
      lastUpdateTime = currentTime;
  }
}

void stopHands() {
  Serial.println("Stopping hands...");
  // Stop the motors
  digitalWrite(PUL_PIN_HR, LOW);
  digitalWrite(PUL_PIN_MIN, LOW);
}