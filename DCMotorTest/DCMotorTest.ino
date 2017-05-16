#include <PWMServo.h>
#include <TimerOne.h>
#include <Encoder.h>

// NOTE: Teensy has library for encoders.  Will just use that.
// NOTE: A is LEFT, B is RIGHT
// NOTE:
// Since we are registering a callback to Timer1, make sure that PWM pins are associated with Timer0/Timer2
// Avoid PWM pins 9, 10 since they are attached to Timer1
#define ENA 23
#define IN1 0
#define IN2 1
#define IN3 2
#define IN4 3
#define ENB 22

#define ENCODERL1   5
#define ENCODERL2   4
#define ENCODERR1   6
#define ENCODERR2   7
#define LED         13
#define GRIPPER     10

#define SPEED_MIN 0
#define SPEED_MAX 95
// motors have different minimum speeds.
// may also want to do this for how it is mapped?
#define PWM_MIN_L   50
#define PWM_MAX_L   250
#define PWM_MIN_R   50
#define PWM_MAX_R   250

#define LoopTime 100 //milliseconds
#define dt       0.1 // seconds

#define WHEEL_BASE          19.4
#define PI_VALUE            3.14
#define ETHRESHOLD          0.5
#define NUM_TARGETS         2

#define TARGET_DISTANCE     60.96 //2 ft in cm
#define PLAYING_FIELD       67.5


#define DEBUG
#define SERVO_ATTACHED
//#define PREDEF
//#define TEST_WHEELBASE

// Constants
float PULSES_REVOLUTION   = 1920.0; // from http://www.robotshop.com/en/micro-6v-160rpm-1201-dc-geared-motor-encoder.html
float WHEEL_CIRCUMFERENCE = 20.42; // in cm, based on 65mm wheel diameter
float ADJUSTMENT_FACTOR   = 1.65; // adjustment factor to deal with squishy wheels.  Seems rather large.
float PULSES_CM           = PULSES_REVOLUTION / WHEEL_CIRCUMFERENCE * ADJUSTMENT_FACTOR; // one cm of travel -- calibrated by driving a distance and seeing how close it gets
float KP                  = 1.0;
float KPW                 = 1.5;
float KI                  = 1.0;
float KD                  = 0.0;




volatile int gripper_pos = 0;
volatile int wheel_c = WHEEL_CIRCUMFERENCE;
volatile long EncCountL_Last = 0;
volatile long EncCountL_Start = 0;
volatile int EncCountL = 0;
volatile int EncCountR = 0;
volatile int EncStateL1 = LOW;
volatile int EncStateL2 = LOW;
volatile int EncStateL1_last = LOW;
volatile int EncStateL2_last = LOW;

volatile float Theta = 0;
volatile float X_pos = 0;
volatile float Y_pos = 0;


volatile long EncCountR_Last = 0;
volatile long EncCountR_Start = 0;
volatile int EncStateR1 = LOW;
volatile int EncStateR2 = LOW;
volatile int EncStateR1_last = LOW;
volatile int EncStateR2_last = LOW;

volatile long LastTime = 0;
volatile long NewTime = 0;
volatile float ErrorTotalL = 0;
volatile float ErrorTotalR = 0;
volatile float DerrorDtL = 0;
volatile float DerrorDtR = 0;
volatile float prev_errorL = 0;
volatile float prev_errorR = 0;

char byteIn = 'n';
int targetDistanceL[NUM_TARGETS], targetDistanceR[NUM_TARGETS];
int nextTargetIdx = -1, currentTargetIdx = 0;

// Setup Encoders
Encoder LeftWheelEncoder(ENCODERL1, ENCODERL2);
Encoder RightWheelEncoder(ENCODERR1, ENCODERR2);
PWMServo gripper;

void setup()
{
  Serial.begin(9600);

  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);
  pinMode(ENA, OUTPUT);
  pinMode(ENB, OUTPUT);
  pinMode(LED, OUTPUT);
  digitalWrite(LED, HIGH);

#ifdef SERVO_ATTACHED
  gripper.attach(GRIPPER);
  Serial.print("Servo Attached: ");
  Serial.println(gripper.attached());
#endif


#ifdef DEBUG
  Serial.print("Initialize Control Loop Frequency: ");
  Serial.println(LoopTime * 1000.0);
  Serial.print("Encoder Counts to 1 cm: ");
  Serial.println(PULSES_CM);
#endif
  Timer1.initialize(LoopTime * 1000.0);
  Timer1.attachInterrupt(ControlThread);
#ifdef PREDEF
  demo1();
#endif

#ifdef TEST_WHEELBASE
  test_wheelbase();
#endif
}

void test_wheelbase(){
  // drive straight
  targetDistanceL[0] = 70;
  targetDistanceR[0] = 70;

  // turn 90 around Right Wheel
  targetDistanceL[1] = WHEEL_BASE * PI_VALUE / 2;
  targetDistanceR[1] = 0;

  // drive straight
  targetDistanceL[2] = 20;
  targetDistanceR[2] = 20;

  // turn 180 around Left Wheel
  targetDistanceL[3] = 0;
  targetDistanceR[3] = WHEEL_BASE * PI_VALUE;
  

  
}

void demo1() {
  targetDistanceL[0] = TARGET_DISTANCE;
  targetDistanceR[0] = TARGET_DISTANCE;

  targetDistanceL[1] = TARGET_DISTANCE;
  targetDistanceR[1] = -1 * TARGET_DISTANCE;

  targetDistanceL[2] = TARGET_DISTANCE;
  targetDistanceR[2] = TARGET_DISTANCE;

  targetDistanceL[3] = -1 * TARGET_DISTANCE;
  targetDistanceR[3] = TARGET_DISTANCE;

  nextTargetIdx = 0;
}

void loop()
{
  // nothing here, everything done by the Control Thread
}

void ControlThread() {
  NewTime = micros();

#ifndef PREDEF
  // read serial to check if message has been sent
  if (Serial.available())
  {
    SerialRead();
  }
#endif



  // if message has been sent, start moving robot to location
  if (currentTargetIdx == nextTargetIdx)
  {
    // Print the targets for both wheels
    //   Serial.println(targetDistanceL[currentTargetIdx % NUM_TARGETS]);
    //   Serial.println(targetDistanceR[currentTargetIdx % NUM_TARGETS]);
    
    // Save last reading to get velocity measurement
    int EncCountL_Last = EncCountL;
    int EncCountR_Last = EncCountR;
    Serial.print("Encoder L: "); Serial.print(EncCountL); Serial.print("\t");
    Serial.print("Encoder R: "); Serial.print(EncCountR); Serial.print("\n");
    
    // Read current encoder position of wheels;
    EncCountL = LeftWheelEncoder.read();
    EncCountR = RightWheelEncoder.read();
    Serial.print(EncCountL);
    Serial.print(EncCountR);

    // Caluclate velocity
    float dl = (EncCountL - EncCountL_Last) / PULSES_CM;
    float dr = (EncCountR - EncCountR_Last) / PULSES_CM;
    float V_L = (float)dl / dt;
    float V_R = (float)dr / dt;
    Serial.println(dl);
    Serial.println(dr);
    Serial.println(V_L);
    Serial.println(V_R);

    // Calculate Omega for robot
    float Omega = (V_R - V_L) / (float) WHEEL_BASE;
    
    // Calcuate radius instantaneous center of rotation
    float R = (float)WHEEL_BASE / 2.0 * (V_L + V_R) / (V_L + V_R);

    // Calculate Instantaneous Center of Roatation
//    float ICC = 

    // Save last position readings
    float X_pos_last = X_pos;
    float Y_pos_last = Y_pos;
    
    // Calculate theta relative to some absolute position.  Useful for keeping global x and y
    Theta = Theta + Omega * dt;
    X_pos = X_pos_last + R * cos(Theta) - R * sin(Theta);
    Y_pos = Y_pos_last + R * sin(Theta) + R * cos(Theta);
    
    // get error for each wheel from desired end point
    float errorL = targetDistanceL[currentTargetIdx % NUM_TARGETS] - ((float)(EncCountL - EncCountL_Start) / PULSES_CM); // cm
    float errorR = targetDistanceR[currentTargetIdx % NUM_TARGETS] - ((float)(EncCountR - EncCountR_Start) / PULSES_CM); // cm

    // integral error
    ErrorTotalL = ErrorTotalL + (dt * errorL);
    ErrorTotalR = ErrorTotalR + (dt * errorR);

    // derivative error
    DerrorDtL = (errorL - prev_errorL) / dt;
    DerrorDtR = (errorR - prev_errorR) / dt;

#ifdef DEBUG
    Serial.print("Error L: ");
    Serial.println(errorL);
    Serial.print("Error R: ");
    Serial.println(errorR);
    Serial.print("Total Error L: ");
    Serial.println(ErrorTotalL);
    Serial.print("Total Error R: ");
    Serial.println(ErrorTotalR);
    Serial.print("Encoder L: ");
    Serial.println(EncCountL);
    Serial.print("Encoder R: ");
    Serial.println(EncCountR);
    Serial.print("Start L: ");
    Serial.println(EncCountL_Start / PULSES_CM);
    Serial.print("Start R: ");
    Serial.println(EncCountR_Start / PULSES_CM);
    Serial.print("Target L: ");
    Serial.println(targetDistanceL[currentTargetIdx % NUM_TARGETS]);
    Serial.print("Target R: ");
    Serial.println(targetDistanceR[currentTargetIdx % NUM_TARGETS]);
#endif

    float EncDifference = ((float)(EncCountL - EncCountR) / PULSES_REVOLUTION) * WHEEL_CIRCUMFERENCE;
    float speed_percentL = KP * errorL + KI * ErrorTotalL + KD * DerrorDtL;
    float speed_percentR = KP * errorR + KI * ErrorTotalR + KD * DerrorDtR;

    // clamp before hand also to ensure there is a differene in speeds
    speed_percentL = max(SPEED_MIN, min(SPEED_MAX, speed_percentL));
    speed_percentR = max(SPEED_MIN, min(SPEED_MAX, speed_percentR));

    // correct for difference between encoders if we are driving straight
    if (targetDistanceL[currentTargetIdx % NUM_TARGETS] == targetDistanceR[currentTargetIdx % NUM_TARGETS])
    {
      if (EncDifference > 0)
        speed_percentL -= KPW * abs(EncDifference);
        speed_percentR += KPW * abs(EncDifference);
      if (EncDifference < 0)
        speed_percentL += KPW * abs(EncDifference);
        speed_percentR -= KPW * abs(EncDifference);
    }
    //    float state1 = (float)EncCountL / PULSES_REVOLUTION;
    float state_d = 0.5 * ((float)(EncCountL - EncCountL_Start) / PULSES_CM) + 0.5 * ((float)(EncCountR - EncCountR_Start) / PULSES_CM);
//    float x_pos = 
//    float state_h = (float)(atan2(EncCountL, EncCountR) * 90.0 / 3.14);
    prev_errorL = errorL;
    prev_errorR = errorR;


#ifdef DEBUG
    Serial.print("Left Command Signal: ");
    Serial.println(speed_percentL);
    Serial.print("Right Command Signal: ");
    Serial.println(speed_percentR);
    Serial.print("Current Distance Travelled: ");
    Serial.println(state_d);
    Serial.print("Current Heading: "); Serial.println(Theta);
    Serial.print("Current Position (x,y): ");
    Serial.print(X_pos); Serial.print(", "); Serial.println(Y_pos);
#endif

    if (speed_percentL > SPEED_MAX)
      speed_percentL = SPEED_MAX;

    if (speed_percentR > SPEED_MAX)
      speed_percentR = SPEED_MAX;

    // send motor command
    if (errorL > 0) {
      forwardL((int)speed_percentL);
    }
    else {
      backwardL((int) - speed_percentL);
    }

    if (errorR > 0) {
      forwardR((int)speed_percentR);
    }
    else {
      backwardR((int) - speed_percentR);
    }

    if (abs(errorL) <= ETHRESHOLD && abs(errorR) <= ETHRESHOLD)
    {
      EncCountL_Start = EncCountL;
      EncCountR_Start = EncCountR;
      halt();
      currentTargetIdx++;
      Serial.println("Goal Reached");
#ifdef PREDEF
      nextTargetIdx++;
#endif
    }
  }
}

void forwardL(int speed_percent) {
  int speed_pwm = map(speed_percent, SPEED_MIN, SPEED_MAX, PWM_MIN_L, PWM_MAX_L);

  analogWrite(ENA, speed_pwm);
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);         // left wheel goes forward
}

void backwardL(int speed_percent) {
  int speed_pwm = map(speed_percent, SPEED_MIN, SPEED_MAX, PWM_MIN_L, PWM_MAX_L);

  analogWrite(ENA, speed_pwm);
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);         // left wheel goes forward
}

void forwardR(int speed_percent) {
  int speed_pwm = map(speed_percent, SPEED_MIN, SPEED_MAX, PWM_MIN_R, PWM_MAX_R);

  analogWrite(ENB, speed_pwm);
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);         // right wheel goes forward
}

void backwardR(int speed_percent) {
  int speed_pwm = map(speed_percent, SPEED_MIN, SPEED_MAX, PWM_MIN_R, PWM_MAX_R);

  analogWrite(ENB, speed_pwm);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);         // right wheel goes forward
}

void halt() {
  digitalWrite(ENA, LOW);
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);        //left wheel holds still
  digitalWrite(ENB, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);        // right wheel holds still
}

void SerialRead() {
  byteIn = Serial.read();
  if (byteIn == 't' || byteIn == 'T') // t for target
  {
    nextTargetIdx++;
    targetDistanceL[nextTargetIdx % NUM_TARGETS] = Serial.parseInt();
    targetDistanceR[nextTargetIdx % NUM_TARGETS] = Serial.parseInt();
  }
  else if (byteIn == 'c' || byteIn == 'C') {
    CloseGripper();
  }
  else if (byteIn == 'o' || byteIn == 'O') {
    OpenGripper();
  }

  if (byteIn == 'd') {
    wheel_c = wheel_c + (float)Serial.parseInt() / 10.0;
    Serial.print("New Wheel Base: "); Serial.println(wheel_c);
    
  }
}

//void mapSpeed(int val) {
//
//}

void CloseGripper() {
  while (gripper_pos <= 180) {
    gripper.write(gripper_pos);
    Serial.print("Closing Gripper: ");
    Serial.println(gripper_pos);
    gripper_pos += 10;
    delay(1000);
  }
  gripper_pos = 0;
}

void OpenGripper() {
  gripper.write(0);
  Serial.println("Opening Gripper");
  gripper_pos = 0;
}
