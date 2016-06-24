// Arduino code for steering control unit

#define STEP_PIN   3    // INT1/PD3/OC2B
#define DIR_PIN    4    // Pin controlling rotation direction
#define ENABLE_PIN 5    // Asserted low (connected to stepper driver)
#define POT_PIN    A0   // Pin to read voltage from potentiometer

#define PI                 3.14159265358979
#define RAD_PER_DEG        PI/180
#define MAX_STEERING_ANGLE PI/6  // Built-in steering limit of front wheels
#define MAX_HEADING_CHANGE 0.281 // [rad] at max steering angle

// Input command, formatted like "goto 123\n".
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

// Exponentially weighted moving averages for integer data. Used for over-
// sampling noisy measurements in a time series.
// When using the result, it must be divided by 8: x8 >> 3.
void ewma(unsigned int x, unsigned int &x8)
{
  // Compute weights like 1/8*(current x) + 7/8*(prev running avg x), except
  // multiplied through by 8 to avoid precision loss from int division:
  // 8*xavg = x + 8*xavg - (8*xavg - 8/2)/8
  x8 = x + x8 - ((x8 - 4) >> 3);
}

// Compute steering angle from target vehicle heading, given the current heading.
// Angles should be in radians, increasing CW from north = 0.
float targetSteerAngle(float targetHeading, float vehicleHeading)
{
  // Angle of velocity vector from vehicle centerline at CM. L < 0; R > 0.
  float a = targetHeading - vehicleHeading;

  // Define target velocity angle inside a range of -pi to pi
  a = (a < -PI) ? a + 2*PI : (a > PI) ? a - 2*PI : a;

  if (a > MAX_HEADING_CHANGE)
  {
    return MAX_STEERING_ANGLE;
  }

  if (a < -MAX_HEADING_CHANGE)
  {
    return -MAX_STEERING_ANGLE;
  }

  // Steering angle delta from steering model: delta = atan(2*tan(a)).
  // Taylor expansion to cubic order used instead (good to 1% in worst case).
  return 2*a*(1 - a*a);
}

// Configure PWM output so that front wheels steer towards steerAngleSetpoint.
// Intended to be called inside a loop. Args in radians.
void steer(float steerAngleSetpoint, float tolerance = 0.01)
{
  float delta = steerAngleSetpoint - steerAngle;

  // Validate inputs
  if (abs(steerAngle + delta) > MAX_STEERING_ANGLE)
  {
    pinMode(STEP_PIN, INPUT);
    return;
  }

  if (delta > tolerance)
  {
    // Steer right of current angle
    pinMode(STEP_PIN, OUTPUT);
    digitalWrite(DIR_PIN, LOW);
  }

  else if (delta < -tolerance)
  {
    // Steer left of current angle
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

void setup()
{
  pinMode(STEP_PIN, INPUT); // Don't enable steering motor yet
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
  command.reserve(128); // Allocate 128 bytes for input string

  // Set up count() as an interrupt handler
  attachInterrupt(digitalPinToInterrupt(STEP_PIN), count, FALLING);
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
        String s = String("adc:") + (angleADC >> 3) + String(",steps:") + steps;
        Serial.println(s);
      }
      else if (op.equalsIgnoreCase("steer"))
      {
        Serial.print("Received: steer ");
        Serial.println(arg);
        // TODO
        // steer(targetSteerAngle(0.0), 0.04); // Head north, 2.3 degree deadband
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

void printStatus()
{
  // Digitized voltage drop on turnpot
  Serial.print("adc:");
  Serial.print(angleADC >> 3);

  // Current steering angle of front wheels [rad]
  // steerAngle = 1e-4 * map(angleADC >> 3, maxLeftADC, maxRightADC, leftSteerMax, rightSteerMax);
  // Serial.print(",sa:");
  // Serial.print(steerAngle);

  // Target steering angle for a vehicle heading of due north
  // TODO uncomment when vehicleHeading is being read in
  // Serial.print(" ST: ");
  // Serial.print(targetSteerAngle(targetHeading));

  // Output pulse count. Only corresponds to pulses sent to driver if STEP_PIN
  // is in output mode.
  Serial.print(",steps:");
  Serial.print(steps);

  Serial.println();
}

void loop()
{
  handleCommands();

  ewma(analogRead(POT_PIN), angleADC);
}
