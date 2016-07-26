// Arduino firmware for Rolling Blackout AVC rover quad steering unit
// Author: Andrew Adare
// Primary functions:
// 1. Read a potentiometer voltage to angleADC, which maps to a steering angle
//    (the mapping is not fully linear, due to geometry of steering linkage)
// 2. Control steering actuator motor through Pololu 18v15 high-current driver:
//    https://www.pololu.com/product/755
// 3. Read encoder on rear axle for odometry.
// 4. Communicate with a master CPU via simple commands over UART USB serial line

#define POT_PIN A0   // Pin to read voltage from potentiometer
#define ENC_PIN_A 2  // Odometer encoder channel A (INT0/PD2)
#define ENC_PIN_B 3  // Odometer encoder channel B (INT1/PD3)
#define DIR_PIN 7    // Controls direction pin on motor driver
#define RST_PIN 8    // Controls reset pin on driver
#define PWM_PIN 9    // Same output also available on 10
#define LEFT 0       // Assignments based on this wiring on driver board:
#define RIGHT 1      //   red: OUTA; black: OUTB
#define DISABLE 0    // Motor driver reset pin asserted low:
#define ENABLE 1     //   DISABLE = !RESET (low); ENABLE = !RESET (high)

// Threshold used in trapezoidal motor ramp schedule [ADC units].
// Defines maximum distance from target position at which to begin rampdown.
// Large values begin rampdown sooner (risking undershoot); small values begin
// later (risking overshoot). Optimal value is found empirically.
// #define RAMPDOWN_DISTANCE 100

// Byte received from serial input
byte rxByte = 0;

// 8 bit PWM value
byte dutyCycle = 0;

// LEFT or RIGHT
byte direction = 0;

// Digitized reading from potentiometer (running average - see ewma() below)
unsigned int adc16 = 0;    // 16 * running average
unsigned int angleADC = 0; // Latest measurement (adc16/16)

// Time step size controlling motor ramp rate [ms]
const unsigned int dt = 2;

String cmd = "";

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

void updateADC()
{
  static unsigned int prevADC = 0;

  ewma(analogRead(POT_PIN), adc16);
  angleADC = adc16 >> 4;

  // Print if ADC value changed (TODO: remove after prototyping phase)
  if (angleADC != prevADC)
  {
    Serial.print("adc: ");
    Serial.println(adc16 >> 4);
  }

  prevADC = angleADC;
}

// Delay by dt, using the time to keep the angleADC value updated.
void wait(const unsigned int dt)
{
  static unsigned long timeMarker = 0;

  timeMarker = millis();
  while (millis() - timeMarker < dt)
  {
    updateADC();
  }
}

// Ramp up to max PWM value, quitting early if halfway position is reached.
// target [ADC units] is used to compute the remaining distance.
// Returns the number of ADC units traversed during the ramp (< 0 if ramping left).
int rampUp(const int target, const int halfDistance)
{
  int initialADC = angleADC;

  for (dutyCycle = 0; dutyCycle < 256; dutyCycle++)
  {
    // Increase speed by one step
    analogWrite(PWM_PIN, dutyCycle);

    // Update angleADC during delay
    wait(dt);

    // Stop ramping up if the remaining distance is half (or less) of the
    // initial move distance
    if (abs(target - angleADC) <= abs(halfDistance))
    {
      return angleADC - initialADC;
    }
  }
  return angleADC - initialADC;
}

// Plateau phase for long moves. Move distance at current dutyCycle. Intended
// to be called after rampUp().
// Returns number of ADC units traversed during this phase (< 0 if moving left).
int flatTop(const int distance)
{
  if (distance <= 0)
    return 0;

  int initialADC = angleADC;
  while (abs(angleADC - initialADC) < distance)
  {
    wait(dt); // updates angleADC
  }
  return angleADC - initialADC;
}

int rampDown(const unsigned int waitTime)
{
  int initialADC = angleADC;
  while (dutyCycle > 0)
  {
    dutyCycle--;
    analogWrite(PWM_PIN, dutyCycle);
    wait(waitTime);
  }
  return angleADC - initialADC;
}

// Make ramped move to targetADC +/- tolerance (tolerance should be nonnegative)
void steerTo(const int targetADC, const int tolerance)
{
  // Total distance to traverse [ADC units]
  int tripLength = targetADC - angleADC;

  if (abs(tripLength) < tolerance)
    return;

  // Set motion direction
  digitalWrite(DIR_PIN, tripLength > 0 ? RIGHT : LEFT);

  // Follow a trapezoid or pyramid ramp plan

  // 1. Ramp up and check remaining distance
  int rampUpLength = rampUp(targetADC, tripLength/2);
  int remainingDistance = targetADC - angleADC;

  // 2. Run at plateau speed if move is long enough (returns immediately if not)
  int flatDistance = abs(remainingDistance) - abs(rampUpLength);
  int flatTopLength = flatTop(flatDistance);

  int rampDownLength = rampDown(1); // Wait only 1ms between PWM steps
  int total = rampUpLength + flatTopLength + rampDownLength;

  Serial.print("tripLength: ");
  Serial.print(tripLength);
  Serial.print(", traveled: ");
  Serial.print(total);
  Serial.println();
}


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
  updateADC();

  if (Serial.available() > 0)
  {
    rxByte = Serial.read();
    if (rxByte == 'd') // change rotation direction
    {
      direction = digitalRead(DIR_PIN);
      direction = !direction;
      digitalWrite(DIR_PIN, direction);
      Serial.print("direction: ");
      Serial.println(direction);
    }
    if (rxByte == 'g') // go in current direction for 500 ms
    {
      Serial.println("motor on");
      for (int i=0; i<256; i++)
      {
        wait(dt);
        analogWrite(PWM_PIN, i);
      }
      for (int i=255; i>=0; i--)
      {
        wait(dt);
        analogWrite(PWM_PIN, i);
      }

      Serial.println("motor off");
      analogWrite(PWM_PIN, 0); // Disable motor
    }
    if (isDigit(rxByte))
    {
      byte digit = rxByte - 48;
      Serial.print(digit);
      cmd += digit;
    }
    if (rxByte == '\r' || rxByte == '\n')
    {
      int target = cmd.toInt();
      Serial.println();
      Serial.print("cmd: ");
      Serial.println(cmd);
      Serial.print("target: ");
      Serial.println(target);

      if (target < 370 || target > 900)
      {
        Serial.print("Invalid target value: ");
        Serial.println(target);
      }
      else
      {
        Serial.print("steerTo ");
        Serial.println(target);
        steerTo(target, 10);
      }
      cmd = "";
    }

  }
}
