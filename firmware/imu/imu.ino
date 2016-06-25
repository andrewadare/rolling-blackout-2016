// Compile and flash to Arduino Uno from command line (replace with your port):
// arduino --board arduino:avr:uno --port /dev/ttyUSB0 --upload thisfile.ino -v
// arduino --board arduino:avr:nano:cpu=atmega328 --port /dev/cu.wchusbserial1410 --upload thisfile.ino -v

#include "NAxisMotion.h"
#include <Wire.h>

// Interface class instance for BNO055 sensor
NAxisMotion imu;

// Input command, formatted like "goto 123\n".
String command;

void setup()
{
  Serial.begin(115200);

  // I2C and IMU sensor initialization
  I2C.begin();
  imu.initSensor();
  imu.setOperationMode(OPERATION_MODE_NDOF);
  imu.setUpdateMode(MANUAL);
}

void handleCommands()
{
  if (Serial.available())
  {
    char c = Serial.read();
    if (c == '\n')
    {
      // Assuming command is formatted like "goto 123\n", split command into
      // an operation and a value/parameter/argument
      String op = command.substring(0, command.indexOf(" "));
      int arg = command.substring(command.indexOf(" ") + 1).toInt();

      if (op.equalsIgnoreCase("status"))
      {
        // Collect calibration status codes (each 0-3) into a 4-digit number
        String s = String("AMGS:")
                   + imu.readAccelCalibStatus()
                   + imu.readMagCalibStatus()
                   + imu.readGyroCalibStatus()
                   + imu.readSystemCalibStatus()

                   // Quaternion components (unitless integers - see BNO055 datasheet table 3-31)
                   + String(",qw:")
                   + imu.readQuatW()
                   + String(",qx:")
                   + imu.readQuatX()
                   + String(",qy:")
                   + imu.readQuatY()
                   + String(",qz:")
                   + imu.readQuatZ();

        Serial.println(s);
      }

      //
      // Handle additional commands here
      //

      // Fall-through case
      else
      {
        Serial.print("Unknown command ");
        Serial.println(op);
      }

      // Reset
      command = "";
    }
    else
    {
      command += c;
    }
  }
}

void loop()
{
  imu.updateQuat();
  imu.updateCalibStatus();
  handleCommands();
}
