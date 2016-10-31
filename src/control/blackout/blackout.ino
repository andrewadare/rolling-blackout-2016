//
// Rolling Blackout vehicle control firmware written for Teensy 3.2
//

#include "NAxisMotion.h"
#include <Wire.h>

// Pin definitions
// A4, A5 occupied for I2C.
#define STEER_DIR_PIN 2
#define STEER_PWM_PIN 3
#define THROTTLE_DIR_PIN 5
#define THROTTLE_PWM_PIN 6
#define LIDAR_ENC_PIN 7 // Lidar encoder ch. A (purple)
#define LIDAR_REV_PIN 8 // Photointerrupter (gray)
#define LIDAR_PWM_PIN 9 // Input w/580 ohm pulldown (yellow).
#define ODO_ENC_PIN_A 11
#define ODO_ENC_PIN_B 12
#define LED_PIN 13
#define STEER_ANGLE_PIN A0

// Global constants
const unsigned int UPDATE_INTERVAL = 20; // Time between communication updates (ms)
const unsigned int LIDAR_PERIOD = 1346;  // Encoder pulses per rotation
const unsigned int ADC_FULL_LEFT  = 300; // 10-bit ADC value at -45 degrees TODO: refine
const unsigned int ADC_FULL_RIGHT = 900; // Same; +45 degrees TODO: refine

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

// Steering angle and limits. Sense is positive clockwise to match IMU heading.
unsigned int adc16 = 0; // 16*ADC value from turnpot
int steeringAngle = 0;  // Front wheel angle (deg)

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
    // TODO: convert odometerValue to distance traveled in cm.
    // Need to measure wheel circumference

    Serial.print("t:");
    Serial.print(millis());
    printCalibStatus();
    printQuaternions();
    Serial.print(",sa:");
    Serial.print(steeringAngle);
    Serial.print(",odo:");
    Serial.print(odometerValue);
    printLidar();
    Serial.println();
  }

  // Read potentiometer and update steering angle
  ewma(analogRead(STEER_ANGLE_PIN), adc16);
  steeringAngle = map(adc16 >> 4, ADC_FULL_LEFT, ADC_FULL_RIGHT, -45, 45);
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

