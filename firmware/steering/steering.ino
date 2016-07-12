// Arduino code for steering control unit

#define ENABLE_PIN  9   // Asserted low on stepper driver
#define DIR_PIN    10   // Pin controlling rotation direction
#define STEP_PIN   11   // INT1/PB3/OC2
#define POT_PIN    A0   // Pin to read voltage from potentiometer

// Input command string
String command;

// Number of steps taken by motor after a command is issued.
volatile unsigned long steps = 0;

// Digitized reading from potentiometer (running average - see ewma() below)
unsigned int angleADC = 0;

// Steering angle limits as ADC readings. Empirically determined.
int maxLeftADC = 577;
int maxRightADC = 254;

// Exponentially weighted moving average for integer data. Used for over-
// sampling noisy measurements in a time series.
// When using the result, it must be divided by 16: x16 >> 4.
void ewma(unsigned int x, unsigned int &x16)
{
  // Compute weights like 1/16*(current x) + 15/16*(prev running avg x), except
  // multiplied through by 16 to avoid precision loss from int division:
  // 16*xavg = x + 16*xavg - (16*xavg - 16/2)/16
  x16 = x + x16 - ((x16 - 8) >> 4);
}

void steerTo(int targetADC, int tolerance)
{
  int currentADC = (angleADC >> 4);

  // Validate inputs
  // if (targetADC >= maxLeftADC || targetADC <= maxRightADC)
  // {
  //   pinMode(STEP_PIN, INPUT);
  //   return;
  // }

  if (currentADC > targetADC + tolerance)
  {
    // Steer right
    pinMode(STEP_PIN, OUTPUT);
    digitalWrite(DIR_PIN, LOW);
  }

  else if (currentADC < targetADC - tolerance)
  {
    // Steer left
    pinMode(STEP_PIN, OUTPUT);
    digitalWrite(DIR_PIN, HIGH);
  }

  else
  {
    // Hold the current steering angle.
    // Disable timer output and reset stepper pulse counter
    pinMode(STEP_PIN, INPUT);
    steps = 0;
  }
}

void handleCommands()
{
  char c = Serial.read();
  if (c == '\n')
  {
    String op = command;
    int arg = 0;

    // If command has a space, assign separate operation and argument parts
    int spaceIndex = command.indexOf(" ");
    if (spaceIndex >= 0)
    {
      op = command.substring(0, spaceIndex);
      arg = command.substring(spaceIndex + 1).toInt();
    }

    // Please reply to request for latest steering angle reading
    if (op.equalsIgnoreCase("u"))
    {
      String s = String("adc:") + (angleADC >> 4) + String(",steps:") + steps;
      Serial.println(s);
    }

    // Steer the car - expecting that arg is a target position in ADC units.
    else if (op.equalsIgnoreCase("s"))
    {
      // Currently the motor runs at a fixed speed (stepper pulse rate).
      steerTo(arg, 10);
    }

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

void setup()
{
  pinMode(DIR_PIN, OUTPUT);

  // Configure Timer 2 for phase-correct (count-up-count-down) PWM
  // mode, with variable PWM frequency (controlled by OCR2A).
  TCCR2A = (1 << WGM20) | (1 << COM2A0) | (1 << COM2B1);
  TCCR2B |= (1 << CS22);   // Prescale to F_CPU/64 (datasheet table 18-9)
  TCCR2B |= (1 << WGM22);  // Toggle OC2A on compare match (table 18-4)

  // With the WGM22 and COM2A0 bits set, OC2A toggles based on the OCR2A value.
  // Pin 11 frequency:
  // f = 16MHz / 64 / (2*OCR2A) / 2 = 62500 Hz / OCR2A
  // duty cycle = 50% (fixed)
  OCR2A = 110;

  // Enable motor
  digitalWrite(ENABLE_PIN, LOW);

  Serial.begin(115200);
  command.reserve(128); // Allocate 128 bytes for input string

  // Set up count() as an interrupt handler
  attachInterrupt(digitalPinToInterrupt(STEP_PIN), count, FALLING);

  // Wait for serial port to connect
  while (!Serial) {;}
}

void loop()
{
  if (Serial.available())
  {
    handleCommands();
  }

  ewma(analogRead(POT_PIN), angleADC);
}

// Pin-change interrupt callback to increment step counter (only called when
// PWM output is enabled).
void count()
{
  steps++;
}
