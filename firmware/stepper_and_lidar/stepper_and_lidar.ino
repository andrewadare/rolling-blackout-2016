
// [stepper_and_lidar]$ arduino --board arduino:avr:uno --port /dev/ttyUSB0 --upload stepper_and_lidar.ino -v

#include <Wire.h>
#include <LIDARLite.h>

// Sensor will pull LIDAR_MODE_PIN low when a new measurement is available.
// This requires using one of the external interrupt pins (Arduino 2 or 3)
#define LIDAR_MODE_PIN 2 // INT0/PD2
#define STEP_PIN 3       // INT1/PD3/OC2B
#define DIR_PIN 4        // Pin controlling rotation direction
#define PERIOD 369       // Number of stepper pulses for 1 scanner revolution

volatile unsigned int steps = 0;

LIDARLite lidar;

// Pin-change interrupt callback - increment step counter
void count()
{
  steps = (steps > PERIOD) ? 0 : steps + 1;
}

void setup()
{
  pinMode(STEP_PIN, OUTPUT);
  pinMode(DIR_PIN, OUTPUT);
  pinMode(LIDAR_MODE_PIN, INPUT);
  // pinMode(11, OUTPUT); // Outputs 250 Hz square wave if enabled

  // Configure Timer 2 to generate a 500 Hz square wave on pin 3 to pulse the
  // stepper motor driver. See comments in test_pwm.ino for details.
  TCCR2A = (1 << WGM20) | (1 << COM2A0) | (1 << COM2B1);
  TCCR2B = (1 << CS22) | (1 << WGM22);
  OCR2A = 250;
  OCR2B = 125;

  // Set rotation direction
  digitalWrite(DIR_PIN, LOW);

  Serial.begin(115200);
  lidar.begin();
  lidar.beginContinuous();

  attachInterrupt(digitalPinToInterrupt(STEP_PIN), count, FALLING);
}

void loop()
{
  // loop_until_bit_is_set(PIND, STEP_PIN);

  // steps = (steps > 369) ? 0 : steps + 1;

  if (!digitalRead(LIDAR_MODE_PIN))
  {
    Serial.print(steps);
    Serial.print(" ");
    Serial.println(lidar.distanceContinuous());
  }

  // loop_until_bit_is_clear(PIND, STEP_PIN);
}