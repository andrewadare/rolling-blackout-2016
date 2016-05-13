// LidarLite code from the PulsedLight3D Distance_Continuous example
// Encoder code from the PJRC Encoder library
//
// - SDA (A4) and SCL (A5) read sensor over I2C
// - Quadrature decoder is interrupt-driven (encoder.read() polls encoder value)
//
// To build and upload to an uno:
// arduino --board arduino:avr:uno --port /dev/ttyUSB0 --upload angle_and_distance.ino -v
// For a nano, use --board arduino:avr:nano:cpu=atmega328

#include <Wire.h>
#include <LIDARLite.h>
#include <Encoder.h>

// Sensor pulls this  pin low when a new measurement is available.
// Only works with external interrupt pins (Arduino 2,3)
#define LIDAR_MODE_PIN 2 // INT0/PD2

// Best encoder reading performance with external interrupt pins (Arduino 2,3);
// but next-best performance with one interrupt pin
#define ENCODER_CH_A 3   // INT1/PD3
#define ENCODER_CH_B 4   // T0/PD4

LIDARLite lidar;
Encoder encoder(ENCODER_CH_A, ENCODER_CH_B);

void setup()
{
  Serial.begin(115200);
  lidar.begin();
  lidar.beginContinuous();

  pinMode(LIDAR_MODE_PIN, INPUT);
}

void loop()
{
  if (!digitalRead(LIDAR_MODE_PIN))
  {
    int ticks = encoder.read();

    Serial.print(ticks);
    Serial.print(" ");
    Serial.println(lidar.distanceContinuous());
  }
}
