// PWM demo for Teensy 3.1/3.2
// From http://www.pjrc.com/teensy/td_pulse.html, the default PWM frequency is 488.28 Hz.
// FTM0 pins: 5, 6, 9, 10, 20, 21, 22, 23
// FTM1 pins: 3, 4
// FTM2 pins: 25, 32

#define PWM_PIN 3

int incomingByte = 0; // for incoming serial data

unsigned int frequency = 488;
unsigned int dutyCycle = 127;

void setup()
{
  analogWrite(PWM_PIN, dutyCycle);
  Serial.begin(115200);
  Serial.println("Enter h/l to change frequency and f/s to change duty cycle")
}

void loop()
{
  if (Serial.available() > 0)
  {
    incomingByte = Serial.read();

    if (incomingByte == 104 || incomingByte == 108) // h or l: higher or lower frequency
    {
      frequency = incomingByte == 104 ? frequency + 1 : frequency -1;

      // Note: all other pins in the timer group are also changed!
      analogWriteFrequency(PWM_PIN, frequency);
      Serial.print("f: ");
      Serial.println(frequency);
    }
    if (incomingByte == 102 || incomingByte == 115) // f or s: faster or slower
    {
      dutyCycle = incomingByte == 102 ? dutyCycle + 1 : dutyCycle -1;
      analogWrite(PWM_PIN, dutyCycle);
      Serial.print("dc: ");
      Serial.println(dutyCycle);
    }

  }
}