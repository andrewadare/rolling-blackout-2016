// Stepper PID control
// Makes use of AVR-specific code for the ATmega328 or equivalent.

#define ENC_PIN_A  2  // INT0/PD2
#define ENC_PIN_B  3  // INT1/PD3
#define ENC_PIN_Z  4  // Changes once/revolution (on fancy encoders)
#define DIR_PIN   10  // Pin controlling rotation direction
#define STEP_PIN  11  // OC2/PB3/PCINT3

int incomingByte = 0; // for incoming serial data

// Empirically-determined OCR2A limits (f = 62500/OCR2A)
const int ocrMin = 80;  // 781 Hz
const int ocrMax = 220; // 284 Hz

// Quadrature encoder variables - mutated in interrupt handlers
volatile int encoderPos = 0;
volatile byte prevA = 0, prevB = 0;
int prevEnc = 0; // Not strictly necessary; just printing to study behavior

// Number of steps taken by motor after a command is issued.
volatile unsigned long steps = 0;

void setup()
{
  Serial.begin(115200);

  // Configure Timer 2 for phase-correct (count-up-count-down) PWM
  // mode, with variable PWM frequency (controlled by OCR2A).
  TCCR2A = (1 << WGM20) | (1 << COM2A0) | (1 << COM2B1);
  TCCR2B |= (1 << CS22);   // Prescale to F_CPU/64 (datasheet table 18-9)
  TCCR2B |= (1 << WGM22);  // Toggle OC2A on compare match (table 18-4)

  // With the WGM22 and COM2A0 bits set, OC2A toggles based on the OCR2A value.
  // Pin 11 frequency:
  // f = 16MHz / 64 / (2*OCR2A) / 2 = 62500 Hz / OCR2A
  // duty cycle = 50% (fixed)
  OCR2A = 100;

  pinMode(STEP_PIN, INPUT); // Enabled within step()
  pinMode(DIR_PIN, OUTPUT);

  // Quadrature decoder setup
  prevA = digitalRead(ENC_PIN_A);
  prevB = digitalRead(ENC_PIN_B);
  attachInterrupt(digitalPinToInterrupt(ENC_PIN_A), decodeA, RISING);
  attachInterrupt(digitalPinToInterrupt(ENC_PIN_B), decodeB, CHANGE);

  // Configure PCINT0_vect to interrupt on pin 11 / PB3.
  PCICR  |= (1 << PCIE0);
  PCMSK0 |= (1 << PB3);
}

// Send n pulses to the stepper driver.
// Frequency and direction are set outside this function.
void step(int n)
{
  if (n <= 0)
    return;

  // Stepping is handled by timer output to STEP_PIN.
  // All that is needed here is to enable output, count steps, and disable.
  steps = 0;
  pinMode(STEP_PIN, OUTPUT);
  while (steps < 2*n) {;}

  pinMode(STEP_PIN, INPUT);
}

void stepTo(int target)
{

  int error = target - encoderPos;

  while (abs(error) > 2)
  {

    if (encoderPos < target)
    {
      digitalWrite(DIR_PIN, HIGH);
      pinMode(STEP_PIN, OUTPUT);
      while (encoderPos < target) {;}
      pinMode(STEP_PIN, INPUT);
    }

    if (encoderPos > target)
    {
      digitalWrite(DIR_PIN, LOW);
      pinMode(STEP_PIN, OUTPUT);
      while (encoderPos > target) {;}
      pinMode(STEP_PIN, INPUT);
    }

    // // Pause for a few milliseconds
    // int marker = millis();
    // while (millis() - marker < 10) {;}
    error = target - encoderPos;
  }
}


void loop()
{
  if (Serial.available() > 0)
  {
    incomingByte = Serial.read();
    prevEnc = encoderPos;
    switch (incomingByte)
    {
    case 100: // d - change rotation direction
      digitalWrite(DIR_PIN, !digitalRead(DIR_PIN));
      break;
    case 101: // e - return encoder value
      Serial.println(encoderPos);
      break;
    case 104: // h - make OCR2A higher (PWM frequency lower)
      OCR2A = OCR2A < 255 ? OCR2A + 1 : 255;
      Serial.print("OCR2A: ");
      Serial.println(OCR2A);
      break;
    case 108: // l - make OCR2A lower (PWM frequency higher)
      OCR2A = OCR2A > 0 ? OCR2A - 1 : 0;
      Serial.print("OCR2A: ");
      Serial.println(OCR2A);
      break;
    case 115: // s - take 200 steps
      stepTo(0);
      Serial.print(encoderPos);
      Serial.print(" ");
      Serial.println(encoderPos - prevEnc);
      break;
    }
  }
}

// ISRs for quadrature decoder readout
void decodeA()
{
  prevB ? encoderPos-- : encoderPos++;
}
void decodeB()
{
  prevB = !prevB;
}

// Pin-change ISR to count pulses to stepper driver
ISR(PCINT0_vect)
{
  steps++;
}
