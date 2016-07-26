
#define DIR_PIN 7
#define RST_PIN 8
#define PWM_PIN 9 // same output also available on 10

byte incomingByte = 0; // for incoming serial data
byte dutyCycle = 0;

void setup()
{
  Serial.begin(115200);

  pinMode(DIR_PIN, OUTPUT);
  pinMode(RST_PIN, OUTPUT);
  digitalWrite(RST_PIN, HIGH); // Enable motor. To reset, write !RESET low

  // PWM pins: 5, 6 (Timer 0); 9, 10 (Timer 1); 3, 11 (Timer 2).
  // Base frequencies: 31350 Hz on 3, 9, 10, and 11; ~62.5 kHz on 5, 6.
  // Reconfiguring Timer 1 does not affect millis() or delay().
  // Default PWM frequencies are ~490 Hz, except for 5,6 at 980 Hz.

  // Clear default Timer 1 prescale setting, then reassign w.r.t. base frequency
  // See ATmega*8 datasheet Table 16-5
  TCCR1B &= !0x07;
  TCCR1B |= (1 << CS11); // 31350/8 = 3918 Hz
}

void loop()
{
  if (Serial.available() > 0)
  {
    incomingByte = Serial.read();
    switch (incomingByte)
    {
    case 100: // d - change rotation direction
      digitalWrite(DIR_PIN, !digitalRead(DIR_PIN));
      break;
    case 104: // h - make duty cycle higher
      dutyCycle = dutyCycle < 255 ? dutyCycle + 1 : 255;
      analogWrite(PWM_PIN, dutyCycle);
      Serial.print("dutyCycle: ");
      Serial.println(dutyCycle);
      break;
    case 108: // l - make duty cycle lower
      dutyCycle = dutyCycle > 0 ? dutyCycle - 1 : 0;
      analogWrite(PWM_PIN, dutyCycle);
      Serial.print("dutyCycle: ");
      Serial.println(dutyCycle);
      break;
    }
  }
}

