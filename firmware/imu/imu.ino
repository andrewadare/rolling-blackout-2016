// Compile and flash to Arduino Uno from command line (replace with your port):
// arduino --board arduino:avr:uno --port /dev/ttyUSB0 --upload thisfile.ino -v
// arduino --board arduino:avr:nano:cpu=atmega328 --port /dev/cu.wchusbserial1410 --upload thisfile.ino -v

#include "NAxisMotion.h"
#include <Wire.h>

// Interface class instance for BNO055 sensor
NAxisMotion imu;

// Time of update step and length of update interval in ms
unsigned long timeMarker = 0;
const unsigned long dt = 20;

// Sensor calibration status values 0 - 3
byte accCalStatus = 0;
byte gyrCalStatus = 0;
byte magCalStatus = 0;
byte sysCalStatus = 0;

void setup()
{
  Serial.begin(115200);

  // I2C and IMU sensor initialization
  I2C.begin();
  imu.initSensor();
  imu.setOperationMode(OPERATION_MODE_NDOF);
  imu.setUpdateMode(MANUAL);

  Serial.println("Streaming in:");
  Serial.print("3...");
  delay(1000);
  Serial.print("2...");
  delay(1000);
  Serial.println("1...");
  delay(1000);
}

void loop()
{
  if (millis() - timeMarker >= dt)
  {
    timeMarker = millis();

    imu.updateQuat();
    imu.updateCalibStatus();
    accCalStatus = imu.readAccelCalibStatus();
    gyrCalStatus = imu.readMagCalibStatus();
    magCalStatus = imu.readGyroCalibStatus();
    sysCalStatus = imu.readSystemCalibStatus();

    // Time of current step (since boot)
    Serial.print("t:");
    Serial.print(timeMarker);

    // Print calibration status codes (each 0-3) as a 4-digit number
    Serial.print(",AMGS:");
    Serial.print(accCalStatus);
    Serial.print(magCalStatus);
    Serial.print(gyrCalStatus);
    Serial.print(sysCalStatus);

    // Quaternion components (unitless integers - see BNO055 datasheet table 3-31)
    Serial.print(",qw:");
    Serial.print(imu.readQuatW());
    Serial.print(",qx:");
    Serial.print(imu.readQuatX());
    Serial.print(",qy:");
    Serial.print(imu.readQuatY());
    Serial.print(",qz:");
    Serial.print(imu.readQuatZ());

    // Current azimuthal heading
    // imu.updateEuler();
    // Serial.print(" H: ");
    // Serial.print(imu.readEulerHeading());

    Serial.println();
  }
}