
#define POT_PIN    A0   // Pin to read voltage from potentiometer

// Digitized reading from potentiometer (running average - see ewma() below)
unsigned int angleADC = 0;

// Time of update step and length of update interval in ms
unsigned long timeMarker = 0;
const unsigned long dt = 20;

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

void setup()
{
  Serial.begin(115200);

  // Wait for serial port to connect
  while (!Serial) {;}
}

void loop()
{

  if (millis() - timeMarker >= dt)
  {
    timeMarker = millis();
    Serial.print("adc: ");
    Serial.println(angleADC >> 4);
  }

  ewma(analogRead(POT_PIN), angleADC);
}
