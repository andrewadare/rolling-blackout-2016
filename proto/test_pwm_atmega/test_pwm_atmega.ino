// This sketch explores PWM frequency and duty cycle control by modifying
// the ATMega328 Timer 2 configuration registers.
// See https://www.arduino.cc/en/Tutorial/SecretsOfArduinoPWM
//
// Serial input (using keys f,s,h,l) controls the PWM parameters.
// This works well for real-time tuning of PWM input to stepper motor drivers.
// Or, just modify OCR2A,B and re-flash.
//
// Below, Timer 2 is configured for phase-correct (count-up-count-down) PWM
// mode, with variable PWM frequency.
// With the WGM22 and COM2A0 bits set, OC2A toggles based on the OCR2A value.
// Pin 11 frequency and duty cycle:
// f = 16MHz / 64 / (2*OCR2A) / 2
// d = 50% (fixed)
// Pin 3:
// f = 16MHz / 64 / OCR2A / 2
// d = OCR2B/OCR2A
//
// For a 500 Hz square wave on pin 3 (and 250 Hz on pin 11), use
// OCR2A = 250 and OCR2B = 125.
//

int incomingByte = 0; // for incoming serial data

void setup()
{
  Serial.begin(115200);

  pinMode(3, OUTPUT);
  pinMode(11, OUTPUT);

  TCCR2A = (1 << WGM20) | (1 << COM2A0) | (1 << COM2B1);
  TCCR2B |= (1 << CS22);   // Prescale to F_CPU/64 (datasheet table 18-9)
  TCCR2B |= (1 << WGM22);  // Toggle OC2A on compare match (table 18-4)

  OCR2A = 128;
  OCR2B = 64;

  // Alternative Timer 2 configuration - fixed PWM frequency, variable duty cycle.
  // PWM frequency is 16MHz/64/(2*255) = 490.196 Hz
  // Duty cycle is varied by OCR2A (pin 11) or OCR2B (pin3)
  // TCCR2A |= (1 << WGM20);  // Set to phase-correct PWM mode (table 18-8)
  // TCCR2B |= (1 << CS22);   // Prescale to F_CPU/64 (table 18-9)
  // TCCR2A |= (1 << COM2A1); // Set OC2A behavior (table 18-4)
  // TCCR2A |= (1 << COM2B1); // Set OC2B behavior (table 18-7)

  // Output compare registers control PWM duty cycles - initialize to half max
  // OCR2A = 128; // pin 11
  // OCR2B = 128; // pin 3
}

void loop()
{
  if (Serial.available() > 0)
  {
    incomingByte = Serial.read();

    if (incomingByte == 104) // h - make OCR2A higher (PWM frequency lower)
    {
      OCR2A = OCR2A < 255 ? OCR2A + 1 : 255;
      Serial.print("OCR2A: ");
      Serial.println(OCR2A);
    }
    if (incomingByte == 108) // l - make OCR2A lower (PWM frequency higher)
    {
      OCR2A = OCR2A > 0 ? OCR2A - 1 : 0;
      Serial.print("OCR2A: ");
      Serial.println(OCR2A);
    }

    if (incomingByte == 102) // f - make pulse fatter
    {
      OCR2B = OCR2B < 255 ? OCR2B + 1 : 255;
      Serial.print("OCR2B: ");
      Serial.println(OCR2B);
    }
    if (incomingByte == 115) // s - make pulse skinnier
    {
      OCR2B = OCR2B > 0 ? OCR2B - 1 : 0;
      Serial.print("OCR2B: ");
      Serial.println(OCR2B);
    }

  }
}