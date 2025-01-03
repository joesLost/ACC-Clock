#include "motors.h"
#include "globals.h" 
#include "esp_task_wdt.h"

volatile int CURRENT_HR_STEPS = HR_STEPS_PER_REV / 2; // Initial position is 6 o'clock
volatile int CURRENT_MIN_STEPS = MIN_STEPS_PER_REV / 2; // Initial position is 30 minutes

void motorControlTask(void *pvParameters) {
  MotorCommand cmd;
  //Set updefaults
  bool isSpinning = false;
  bool isMinAdvance = false; 
  bool spinDirection = true; 
  bool isProportional = false;
  bool waitForReset = false;
  int spinSpeed = 15;
  while (true) {
    // Check if there is a new command in the queue
    if (xQueueReceive(motorCommandQueue, &cmd, 0) == pdPASS) {
      switch (cmd.type) {
        case SPIN_CONTINUOUS:
          if (!waitForReset){
            isSpinning = true;
          }
          spinSpeed = max(abs(cmd.speed), 1); //abs shouold not be nessesary but just in case
          spinDirection = cmd.direction;
          isProportional = cmd.proportional;
          break;
        case STOP_HANDS:
          isSpinning = false;
          isMinAdvance = false;
          waitForReset = false;
          xQueueReset(motorCommandQueue);
          break;
        case MOVE_TO_HOME:
          if (!waitForReset){
            isSpinning = false;
            moveToHome();
          }
          break;
        case SET_TIME:
          if(isSpinning){
            setTime(cmd.hour, cmd.minute, spinSpeed, 1, true);
            isSpinning = false;
            waitForReset = true;
          }else{
            setTime(cmd.hour, cmd.minute, max(cmd.speed, 10), 0);
          }
          break;
        case MIN_ADVANCE:
          isSpinning = false;
          isMinAdvance = true;
          break;
      }
    }
    if (isSpinning) {
      spinContinuous(spinSpeed, spinDirection, isProportional);     
    } 
    else if (isMinAdvance) {
      advanceRealMin();
    }
    taskYIELD(); //Feed the watchdog
  }
}

void pulseMotor(int pulPin, int speedMultiplier) { //A single pulse
  int delayTime = max(1000 / speedMultiplier, 10); //10 is min pulse delay creating the fastest speed
  digitalWrite(pulPin, HIGH);
  delayMicroseconds(5); // pulse width high
  digitalWrite(pulPin, LOW);
  delayMicroseconds(delayTime);
}

void updatePos(bool isMinMotor, bool direction){ //logging each hand position
  if (isMinMotor) {
    if (direction) {
      CURRENT_MIN_STEPS = (CURRENT_MIN_STEPS + 1) % MIN_STEPS_PER_REV;
    } else {
      CURRENT_MIN_STEPS = (CURRENT_MIN_STEPS - 1 + MIN_STEPS_PER_REV) % MIN_STEPS_PER_REV;
    }
  } else {
    if (direction) {
      CURRENT_HR_STEPS = (CURRENT_HR_STEPS + 1) % HR_STEPS_PER_REV;
    } else {
      CURRENT_HR_STEPS = (CURRENT_HR_STEPS - 1 + HR_STEPS_PER_REV) % HR_STEPS_PER_REV;
    }
  }
}

void spinMotor(bool isMinMotor, bool clockwise, int steps, int speedMultiplier) {
  int pulPin = isMinMotor ? PUL_PIN_MIN : PUL_PIN_HR;

  digitalWrite(DIR_PIN, clockwise ? LOW : HIGH);
  Serial.println(steps);
  for (int i = 0; i < steps; i++) {
    pulseMotor(pulPin, speedMultiplier);
    updatePos(isMinMotor, clockwise);
  }
}

void spinContinuous(int speed, bool clockwise, bool isProportional) {
  static int previousSpeed = 1;
  int currentSpeed = speed;
  const int pulseBatchSize = 1200;  // Number of pulses to generate before yielding, to avoid triggering the watchdog
  digitalWrite(DIR_PIN, clockwise ? LOW : HIGH);
  for (int step = 0; step < pulseBatchSize; ++step) {
    if (speed != previousSpeed) {
      currentSpeed = map(step, 1, pulseBatchSize, previousSpeed, speed);
    } else {
      currentSpeed = speed;
    }
    pulseMotor(PUL_PIN_MIN, currentSpeed);
    updatePos(true, clockwise);
    if(!isProportional ){
      pulseMotor(PUL_PIN_HR, currentSpeed);
      updatePos(false, clockwise);
    } else if (step % 12 == 0) {
      pulseMotor(PUL_PIN_HR, currentSpeed);
      updatePos(false, clockwise);
    }
  }
  previousSpeed = speed;
}

void spinProportional(int minSteps, int hrSteps, bool clockwise, int maxSpeedMultiplier, bool noRamp) {
  static int currentSpeed = 15;
  int totalSteps = max(minSteps, hrSteps);
  int rampUpSteps = totalSteps / 15;
  int rampDownSteps = totalSteps / 15;
  int minCounter = 0;
  int hrCounter = 0;
  int step = 0;

  digitalWrite(DIR_PIN, clockwise ? LOW : HIGH);

  while (step < totalSteps) {
    if (!noRamp && step < rampUpSteps) {
      currentSpeed = map(step, 0, rampUpSteps, 15, maxSpeedMultiplier);
    } else if (step > totalSteps - rampDownSteps) {
      currentSpeed = map(step, totalSteps - rampDownSteps, totalSteps, maxSpeedMultiplier, 0);
    } else {
      currentSpeed = maxSpeedMultiplier;
    } 

    // Pulse the minute motor
    if (minCounter < minSteps) {
      pulseMotor(PUL_PIN_MIN, currentSpeed);
      minCounter++;
      updatePos(true, clockwise);
    }

    // Pulse the hour motor if necessary
    if (hrCounter < hrSteps && minCounter % 16 == 0) { //Need to account for the ratio between the hour and min hands  12 (12:1 is the ratio of a clock) * 16000 (number of steps for min hand) / 12000 (number of steps for the hour hand) = 16
      pulseMotor(PUL_PIN_HR, currentSpeed);
      hrCounter++;
      updatePos(false, clockwise);
    }
    step++;

    if (step % 1200 == 0) {
      taskYIELD();  // Feed the watchdog, yield control every 1200 steps, more frequenctly will cause jerky movement
    }
  }
}

void spinProportionalDuration(int durationMs, bool clockwise, int maxSpeedMultiplier) { //not used probably needs rework
  unsigned long startTime = millis();
  unsigned long endTime = startTime + durationMs;
  digitalWrite(DIR_PIN, clockwise ? LOW : HIGH);

  int currentSpeed = 1;
  int rampUpSteps = 1000;
  int rampDownSteps = 1000;
  int totalSteps = durationMs * maxSpeedMultiplier;
  int minCounter = 0;

  int step = 0;

  while (millis() < endTime) {
    // Handle ramp up, constant speed, and ramp down
    if (step < rampUpSteps) {
      currentSpeed = map(step, 0, rampUpSteps, 1, maxSpeedMultiplier);
    } else if (step > totalSteps - rampDownSteps) {
      currentSpeed = map(step, totalSteps - rampDownSteps, totalSteps, maxSpeedMultiplier, 1);
    } else {
      currentSpeed = maxSpeedMultiplier;
    }

    // Pulse the motor
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

    // Increment step counter
    step++;

    // Yield control every 100 steps to prevent blocking and avoid watchdog reset
    if (step % 100 == 0) {
      vTaskDelay(10 / portTICK_PERIOD_MS); // Short delay to yield control
    }
  }
}

int getCurrentHour() {
  int hour = (CURRENT_HR_STEPS % HR_STEPS_PER_REV) / (HR_STEPS_PER_REV / 12);
  return (hour == 0 ? 12 : hour);
}

int getCurrentMin() {
  int min = (CURRENT_MIN_STEPS % MIN_STEPS_PER_REV) / (MIN_STEPS_PER_REV / 60);
  return min;
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

void initToHome() {
  Serial.println("Moving to home position...");
  spinMotor(true, true, MIN_STEPS_PER_REV / 2 , 3);
  spinMotor(false, true, HR_STEPS_PER_REV / 2, 3);
  CURRENT_HR_STEPS = 0;
  CURRENT_MIN_STEPS = 0;
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

int calculateminDiff(int oldHour, int oldmin, int newHour, int newmin) {
    Serial.print("Old Time: ");
    Serial.print(oldHour);
    Serial.print(":");
    Serial.println(oldmin);
    Serial.print("New Time: ");
    Serial.print(newHour);
    Serial.print(":");
    Serial.println(newmin);
    // Calculate the total mins for the old time and the new time.
    int oldTotalmins = (oldHour % 12) * 60 + oldmin;
    int newTotalmins = (newHour % 12) * 60 + newmin;
    Serial.print("Old Total Mins: ");
    Serial.println(oldTotalmins);
    Serial.print("New Total Mins: ");
    Serial.println(newTotalmins);

    // Calculate the Diff in mins.
    int minDiff = newTotalmins - oldTotalmins;

    // Handle the wrap-around scenario for going forward in time (e.g., 11:45 to 1:15).
    if (minDiff < 0) {
        minDiff += 12 * 60;  // Wrap around the 12-hour clock.
    }

    return minDiff;
}


void setTime(int hr, int min, int speed, int extraRevs, bool noRamp) {
  Serial.print(" Setting time to ");
  Serial.print(hr);
  Serial.print(":");
  Serial.println(min);

  int targetHrSteps = map(hr, 0, 12, 0, HR_STEPS_PER_REV);
  int targetMinSteps = map(min, 0, 60, 0, MIN_STEPS_PER_REV);
  checkTime();
  Serial.print("Target Step Pos: Hr:");
  Serial.print(targetHrSteps);
  Serial.print(" Min:");
  Serial.println(targetMinSteps);

  int hrSteps = (targetHrSteps - CURRENT_HR_STEPS + HR_STEPS_PER_REV) % HR_STEPS_PER_REV;
  int minSteps = (targetMinSteps - CURRENT_MIN_STEPS + MIN_STEPS_PER_REV) % MIN_STEPS_PER_REV;
  

  int minsToAdvance = calculateminDiff(getCurrentHour(), getCurrentMin(), hr, min);

  Serial.print("Mins to advance: ");
  Serial.println(minsToAdvance);
  if( minsToAdvance > 60){
    int fullMinRevs = (minsToAdvance / 60) * MIN_STEPS_PER_REV; //Int division not floating point 75/60 = 1
    minSteps += fullMinRevs;
  }
  // Make sure that if the hour hand is not supposed to change, it still moves slightly based on minute hand movement
  if (hrSteps == 0 && minSteps > 0) {
    // For every 1/12th of a revolution of the minute hand, move the hour hand 1 step
    int proportionalHrSteps = (minSteps / MIN_STEPS_PER_REV) * (HR_STEPS_PER_REV / 16);//Need to account for the ratio between the hour and min hands  12 (12:1 is the ratio of a clock) * 16000 (number of steps for min hand) / 12000 (number of steps for the hour hand) = 16
    hrSteps += proportionalHrSteps;
  }
  // Add extra revolutions if specified
  if(extraRevs > 0){
    hrSteps += HR_STEPS_PER_REV * extraRevs;
    minSteps += MIN_STEPS_PER_REV * extraRevs;
  }

  Serial.println("Steps to target: ");
  Serial.print("Hour: ");
  Serial.print(hrSteps);
  Serial.print(" Min: ");
  Serial.println(minSteps);

  spinProportional(minSteps, hrSteps, true, abs(speed), noRamp);
}

void correctTimePos(int hr, int min) {
  int hrSteps = map(hr, 0, 12, 0, HR_STEPS_PER_REV);
  int minSteps = map(min, 0, 59, 0, MIN_STEPS_PER_REV);
  CURRENT_HR_STEPS = hrSteps;
  CURRENT_MIN_STEPS = minSteps;
  moveToHome();
}

void spinTest() {
  Serial.println("Starting spin test");
  for(int i = 10; i <=100; i+=10){ //0-59
    Serial.print("Spin time 5s. Speed: ");
    Serial.println(i);
    spinProportionalDuration(5000, true, i);
    vTaskDelay(1000);
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

void advanceRealMin() {
  static unsigned long lastUpdateTime = 0;
  unsigned long currentTime = millis();
  if (currentTime - lastUpdateTime >= 60000) { // Check if a minute has passed
      int minSteps = MIN_STEPS_PER_REV / 60;
      int hrSteps = HR_STEPS_PER_REV / (60 * 12);
      spinProportional(minSteps, hrSteps, true, 10);
      lastUpdateTime = currentTime;
  }
}
