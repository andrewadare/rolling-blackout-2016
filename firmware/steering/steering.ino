
// Compile and flash to Arduino Uno from command line (replace with your port):
// arduino --board arduino:avr:uno --port /dev/ttyUSB0 --upload steering.ino -v

#include "NAxisMotion.h"
#include <Wire.h>

#define STEP_PIN   3  // INT1/PD3/OC2B
#define DIR_PIN    4  // Pin controlling rotation direction
#define ENABLE_PIN 5  // Asserted low (connected to stepper driver)
#define POT_PIN   A0  // Pin to read voltage from potentiometer

// A4 and A5 are also used as the I2C data and clock lines for the BNO055 board.

// Number of steps taken by motor after a command is issued.
volatile unsigned long steps = 0;

// For incoming serial data
byte incomingByte = 0;

// Digitized reading from potentiometer (running average - see ewma() below)
unsigned int angleADC = 0;

// Steering angle and limits. Orientation follows that of IMU sensor (positive
// clockwise). Since map() uses integer math, the limits (+/- 30 deg) are
// represented as integral units of 0.01 degree to limit loss of precision.
int leftSteerMax = -3000;
int rightSteerMax = 3000;
float steerAngle = 0;

// Steering angle limits as ADC readings. Empirically determined.
int leftADC = 577;
int rightADC = 254;

// Timestamp of previous update step and update interval in ms
unsigned long prevTime = 0;
const unsigned long dt = 20;

// Interface class instance for BNO055 sensor
NAxisMotion imu;

// Pin-change interrupt callback - increment step counter
void count()
{
  steps++;
}

// Exponentially weighted moving average for integer data. Used for over-
// sampling noisy ADC measurements in a time series. 'Stir in' x to x8.
// When using the result, it must be divided by 8: x8 >> 3.
void ewma(unsigned int x, unsigned int &x8)
{
  // Compute weights like 1/8*(current x) + 7/8*(prev running avg x), except
  // multiplied through by 8 to avoid precision loss from int division:
  // 8*xavg = x + 8*xavg - (8*xavg - 8/2)/8
  x8 = x + x8 - ((x8 - 4) >> 3);
}

void setup()
{
  pinMode(STEP_PIN, OUTPUT);
  pinMode(DIR_PIN, OUTPUT);
  // pinMode(11, OUTPUT); // Outputs 250 Hz square wave if enabled

  // Configure Timer 2 to generate a 500 Hz square wave on pin 3 to pulse the
  // stepper motor driver. See comments in test_pwm.ino for details.
  TCCR2A = (1 << WGM20) | (1 << COM2A0) | (1 << COM2B1);
  TCCR2B = (1 << CS22) | (1 << WGM22);
  OCR2A = 250;
  OCR2B = 125;

  // Set rotation direction
  digitalWrite(DIR_PIN, LOW);

  // Enable motor
  digitalWrite(ENABLE_PIN, LOW);

  Serial.begin(115200);

  // I2C and IMU sensor initialization
  I2C.begin();
  imu.initSensor();
  imu.setOperationMode(OPERATION_MODE_NDOF);
  imu.setUpdateMode(MANUAL);

  // Set up count() as an interrupt handler
  attachInterrupt(digitalPinToInterrupt(STEP_PIN), count, FALLING);
}

void loop()
{

  if (millis() - prevTime >= dt)
  {
    prevTime = millis();

    // Update Euler angle measurements and sensor calibration status
    imu.updateEuler();
    imu.updateCalibStatus(); // TODO: print warning when not ready

    // Timestamp of current step
    Serial.print(prevTime);

    // Azimuthal bearing [deg]
    Serial.print(" H: ");
    Serial.print(imu.readEulerHeading());

    // Digitized voltage drop on turnpot
    Serial.print(" ADC: ");
    ewma(analogRead(POT_PIN), angleADC);
    Serial.print(angleADC >> 3);

    // Computed steering angle [deg]
    steerAngle = 0.01 * map(angleADC >> 3, leftADC, rightADC, leftSteerMax, rightSteerMax);
    Serial.print(" ");
    Serial.println(steerAngle);
  }
}
