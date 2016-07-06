// Arduino code for steering control unit

#define STEP_PIN   3    // INT1/PD3/OC2B
#define DIR_PIN    4    // Pin controlling rotation direction
#define ENABLE_PIN 5    // Asserted low (connected to stepper driver)
#define POT_PIN    A0   // Pin to read voltage from potentiometer

#define PI                 3.14159265358979
#define RAD_PER_DEG        PI/180
#define MAX_STEERING_ANGLE PI/6  // Built-in steering limit of front wheels
#define MAX_HEADING_CHANGE 0.281 // [rad] at max steering angle

// Input command string
String command;

// Number of steps taken by motor after a command is issued.
volatile unsigned long steps = 0;

// Digitized reading from potentiometer (running average - see ewma() below)
unsigned int angleADC = 0;

// Steering angle (positive clockwise) and limits.
// Since map() uses integer math, the limits (+/-30 deg = pi/6) are
// represented as integral units of 1e-4 rad to limit loss of precision.
int leftSteerMax = -MAX_STEERING_ANGLE * 1e4;
int rightSteerMax = MAX_STEERING_ANGLE * 1e4;
float steerAngle = 0;

// Steering angle limits as ADC readings. Empirically determined.
int maxLeftADC = 577;
int maxRightADC = 254;

// Pin-change interrupt callback to increment step counter (only called when
// PWM output is enabled).
void count()
{
  steps++;
}

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
  if (Serial.available())
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

      // Steer the car - expecting that arg is a steering angle in ADC units.
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
}

// Current steering angle of front wheels [rad]
// steerAngle = 1e-4 * map(angleADC >> 4, maxLeftADC, maxRightADC, leftSteerMax, rightSteerMax);

void setup()
{
  pinMode(STEP_PIN, INPUT); // Don't enable steering motor yet
  pinMode(DIR_PIN, OUTPUT);
  // pinMode(11, OUTPUT); // Outputs 250 Hz square wave if enabled

  // Configure Timer 2 to generate a 500 Hz square wave on pin 3 to pulse the
  // stepper motor driver. See comments in test_pwm.ino for details.
  TCCR2A = (1 << WGM20) | (1 << COM2A0) | (1 << COM2B1);
  TCCR2B = (1 << CS22) | (1 << WGM22);
  OCR2A = 220;
  OCR2B = 110;

  // Set rotation direction
  digitalWrite(DIR_PIN, LOW);

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
  handleCommands();

  ewma(analogRead(POT_PIN), angleADC);
}
