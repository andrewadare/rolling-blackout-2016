//
// Rolling Blackout vehicle control firmware written for Teensy 3.2
//
#include "PIDControl.h"
#include "NAxisMotion.h"
#include <Wire.h>

#define DEBUG 0

// Pin definitions
// A4, A5 occupied for I2C.
#define LIDAR_ENC_PIN 7 // Lidar encoder ch. A (purple)
#define LIDAR_REV_PIN 8 // Photointerrupter (gray)
#define LIDAR_PWM_PIN 9 // Input w/580 ohm pulldown (yellow).
#define ODO_ENC_PIN_A 11
#define ODO_ENC_PIN_B 12
#define LED_PIN 13
#define STEER_ANGLE_PIN A0
#define STEER_DIR_PIN 20 // HIGH: right, LOW: left (white)
#define STEER_PWM_PIN 21 // (gray)
#define THROTTLE_DIR_PIN 22
#define THROTTLE_PWM_PIN 23

// Global constants
const int UPDATE_INTERVAL = 20;         // Time between communication updates (ms)
const int LIDAR_PERIOD = 1346;          // Encoder pulses per rotation
const int ADC_FULL_LEFT  = 430;         // ADC reading at left steer angle limit
const int ADC_FULL_RIGHT = 830;         // and at right limit
const float DEG_FULL_LEFT  = -27.0;     // Angle in degrees at left and right
const float DEG_FULL_RIGHT =  27.0;     // limits
const float METERS_PER_TICK = 1.07/700; // Wheel circumference/(ticks per rev)
const float STEER_PID_DEADBAND = 0.01;

// PID parameters
float kp = 1, ki = 0, kd = 0;
unsigned long pidTimeStep = 20; // ms

// Center steering position in "PID units" (i.e. scaled to the [-1,1] interval)
float initialSteerSetpoint = 0;

float adcToDegrees(int adc)
{
  return (float)map(adc, ADC_FULL_LEFT, ADC_FULL_RIGHT, 100*DEG_FULL_LEFT, 100*DEG_FULL_RIGHT)/100;
}

// Convert steering readout in ADC units to PID units in [-1,1]
float adcToPidUnits(int adc)
{
  return (float)map(adc, ADC_FULL_LEFT, ADC_FULL_RIGHT, -1000, 1000)/1000;
}

// Convert steer angle in degrees to PID units in [-1,1]
float degreesToPidUnits(float angle)
{
  return (float)map(1000*angle, 1000*DEG_FULL_LEFT, 1000*DEG_FULL_RIGHT, -1000, 1000)/1000;
}

PIDControl steerPid(kp, ki, kd, initialSteerSetpoint, pidTimeStep);

// Bosch BNO055 absolute orientation sensor
NAxisMotion imu;

// Timer for update interval in ms
elapsedMillis printTimer = 0;

// Timer for Lidar signal (PWM pulse width measurement)
elapsedMicros lidarTimer = 0;

volatile unsigned int lidarPulseWidth = 0; // microseconds
volatile unsigned int lidarAngleCounter = 0; // Single-channel pulse counter

// Quadrature encoder variables - modified in interrupt handlers
volatile long odoTicks = 0;
volatile byte prevA = 0, prevB = 0;

// These are copied from their volatile counterparts to avoid race conditions
unsigned int lidarRange = 0; // cm
unsigned int lidarAngle = 0; // deg
long odometerValue = 0;

// Distance traveled by vehicle in meters
float odometerDistance = 0;

// Steering angle and limits. Sense is positive clockwise to match IMU heading.
unsigned int adc16 = 0;  // 16*ADC value from turnpot
float steeringAngle = 0;  // Front wheel angle (deg)

String cmd = "";

// From the PID output value (which is in the -1 to +1 range), set the direction
// and the 8-bit PWM duty cycle for the steering motor.
void updateSteering(float pidValue)
{
  // TODO: implement STEER_PID_DEADBAND here
  int dutyCycle = map(1e6*fabs(pidValue), 0, 1e6, 0, 255);
  dutyCycle = constrain(dutyCycle, 0, 255);

#if DEBUG
  Serial.print(degreesToPidUnits(steeringAngle));
  if (pidValue > 0)
    Serial.print(" Set +");
  else
    Serial.print(" Set -");
  Serial.println(dutyCycle);
#endif

  digitalWrite(STEER_DIR_PIN, pidValue > 0 ? HIGH : LOW);
  analogWrite(STEER_PWM_PIN, dutyCycle);
}

void setup()
{
  pinMode(STEER_DIR_PIN, OUTPUT);
  pinMode(STEER_PWM_PIN, OUTPUT);
  pinMode(THROTTLE_DIR_PIN, OUTPUT);
  pinMode(THROTTLE_PWM_PIN, OUTPUT);
  pinMode(LIDAR_ENC_PIN, INPUT);
  pinMode(LIDAR_REV_PIN, INPUT);
  pinMode(LIDAR_PWM_PIN, INPUT);
  pinMode(ODO_ENC_PIN_A, INPUT);
  pinMode(ODO_ENC_PIN_B, INPUT);
  pinMode(LED_PIN, OUTPUT);

  steerPid.minOutput = -1; // Full speed left
  steerPid.maxOutput = +1; // Full speed right

  // I2C and IMU sensor initialization
  I2C.begin();
  imu.initSensor();
  imu.setOperationMode(OPERATION_MODE_NDOF);
  imu.setUpdateMode(MANUAL);

  // Quadrature decoder setup
  prevA = digitalRead(ODO_ENC_PIN_A);
  prevB = digitalRead(ODO_ENC_PIN_B);
  attachInterrupt(digitalPinToInterrupt(ODO_ENC_PIN_A), decodeA, RISING);
  attachInterrupt(digitalPinToInterrupt(ODO_ENC_PIN_B), decodeB, CHANGE);

  // Attach ISR for pulse width measurement
  attachInterrupt(digitalPinToInterrupt(LIDAR_PWM_PIN), onLidarPinChange, CHANGE);

  // Lidar angle measurement ISRs
  attachInterrupt(digitalPinToInterrupt(LIDAR_ENC_PIN), onLidarEncoderRise, RISING);
  attachInterrupt(digitalPinToInterrupt(LIDAR_REV_PIN), onNewLidarRevolution, RISING);

  Serial.begin(115200);

  // Wait for serial port to connect
  while (!Serial) {;}
}

// Exponentially weighted moving average for integer data. Used for over-
// sampling noisy measurements in a time series.
// When using the result, it must be divided by 16: x16 >> 4.
void ewma(unsigned int x, unsigned int &x16)
{
  // Compute weights like 1/16*(current x) + 15/16*(prev running avg x), except
  // multiplied through by 16 to avoid precision loss from int division:
  // 16*xavg = x + 16*xavg - (16*xavg - 16/2)/16
  x16 = x + x16 - ((x16 - 8) >> 4);
}

// Print calibration status codes (each 0-3) as a 4-digit number
void printCalibStatus()
{
  imu.updateCalibStatus();
  Serial.print(",AMGS:");
  Serial.print(imu.readAccelCalibStatus());
  Serial.print(imu.readMagCalibStatus());
  Serial.print(imu.readGyroCalibStatus());
  Serial.print(imu.readSystemCalibStatus());
}

// Quaternion components (unitless integers - see BNO055 datasheet table 3-31)
void printQuaternions()
{
  imu.updateQuat();
  Serial.print(",qw:");
  Serial.print(imu.readQuatW());
  Serial.print(",qx:");
  Serial.print(imu.readQuatX());
  Serial.print(",qy:");
  Serial.print(imu.readQuatY());
  Serial.print(",qz:");
  Serial.print(imu.readQuatZ());
}

void printLidar()
{
  noInterrupts();
  lidarAngle = lidarAngleCounter;
  lidarRange = lidarPulseWidth;
  interrupts();

  lidarRange /= 10; // Convert to cm, which is the minimum resolution
  lidarAngle *= 360.0/LIDAR_PERIOD; // Convert to degrees

  if (lidarAngle > 360)
  {
    lidarAngle -= 360;
  }

  Serial.print(",r:");
  Serial.print(lidarRange);
  Serial.print(",b:");
  Serial.print(lidarAngle);
}

void handleByte(byte b)
{
  if (b == '-')
    cmd = "-";

  // kp
  else if (b == 'p') // increase kp
    kp += 0.01;
  else if (b == 'l') // decrease kp
    kp -= 0.01;

  // ki
  else if (b == 'i') // increase ki
    ki += 0.1;
  else if (b == 'k') // decrease ki
    ki -= 0.1;

  // kd
  else if (b == 'd') // increase kd
    kd += 0.001;
  else if (b == 'c') // decrease kd
    kd -= 0.001;

  else if (b == '.')
    cmd += b;
  else if (isDigit(b))
  {
    byte digit = b - 48;
    cmd += digit;
  }
  else if (b == '\r' || b == '\n')
  {
    // Assume here that cmd is a signed angle setpoint in degrees.
    // Convert to a fraction in [0,1] for PID input
    float setPoint = degreesToPidUnits(cmd.toFloat());
    steerPid.setpoint = constrain(setPoint, steerPid.minOutput, steerPid.maxOutput);
    cmd = "";
  }
  else
  {
    steerPid.setPID(kp, ki, kd);
  }
}

void loop()
{
  // Send out vehicle state every UPDATE_INTERVAL milliseconds
  // "t:%d,AMGS:%d%d%d%d,qw:%d,qx:%d,qy:%d,qz:%d,sa:%d,odo:%d,r:%d,b:%d\r\n"
  if (printTimer >= UPDATE_INTERVAL)
  {
    printTimer -= UPDATE_INTERVAL;

    noInterrupts();
    odometerValue = odoTicks;
    interrupts();
    odometerDistance = odometerValue*METERS_PER_TICK;

    Serial.print("t:");
    Serial.print(millis());
    printCalibStatus();
    printQuaternions();
    Serial.print(",sa:");
    Serial.print(steeringAngle);
    Serial.print(",odo:");
    Serial.print(odometerDistance);
    printLidar();
    Serial.println();

    float input = adcToPidUnits(adc16 >> 4);
    steerPid.update(input, millis());
    updateSteering(steerPid.output);
  }

  if (Serial.available() > 0)
  {
    byte rxByte = Serial.read();
    handleByte(rxByte);
  }

  // Read potentiometer and update steering angle
  ewma(analogRead(STEER_ANGLE_PIN), adc16);

  steeringAngle = adcToDegrees(adc16 >> 4);
}

// ISRs for odometer decoder readout
void decodeA()
{
  prevB ? odoTicks-- : odoTicks++;
}
void decodeB()
{
  prevB = !prevB;
}

// ISR to measure PWM pulse width from LidarLite
void onLidarPinChange()
{
  if (digitalRead(LIDAR_PWM_PIN) == HIGH)
    lidarTimer = 0;
  else
    lidarPulseWidth = lidarTimer;
}

// ISRs for lidar encoder and photointerrupter
void onNewLidarRevolution()
{
  lidarAngleCounter = 0;
}
void onLidarEncoderRise()
{
  lidarAngleCounter++;
}

