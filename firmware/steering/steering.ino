
// [stepper_and_lidar]$ arduino --board arduino:avr:uno --port /dev/ttyUSB0 --upload steering.ino -v

#define STEP_PIN   3  // INT1/PD3/OC2B
#define DIR_PIN    4  // Pin controlling rotation direction
#define ENABLE_PIN 5  // Asserted low
#define POT_PIN A0

volatile unsigned long steps = 0;
int incomingByte = 0; // for incoming serial data
int angleADC = 0;

// Pin-change interrupt callback - increment step counter
void count()
{
  steps++;
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

  attachInterrupt(digitalPinToInterrupt(STEP_PIN), count, FALLING);
}

void loop()
{

  steps = 0;
  digitalWrite(DIR_PIN, LOW);
  do {}
  while (steps < 100);

  delay(500);

  steps = 0;
  digitalWrite(DIR_PIN, HIGH);
  do {}
  while (steps < 100);

  delay(500);

  angleADC = analogRead(POT_PIN);
  Serial.println(angleADC);

}
