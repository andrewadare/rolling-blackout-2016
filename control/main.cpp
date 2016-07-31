#include "mbed.h"
#include "BNO055.h"
#include "LidarLitev2.h"

// Flag indicating whether the LidarLite sensor is hooked up
const bool lidar_attached = false;

// Smoothing parameter for exponentially-weighted moving average s[t] of
// time series measurements y[t]:
//     s[t] = alpha*y[t-1] + (1-alpha)*s[t-1], 0 < alpha <= 1
// Small alpha: strong smoothing, with slower response to trends;
// Large alpha: less noise damping but faster response.
const float alpha = 0.3;

// Steering angle potentiometer reading in the range [0,1]
float pot;

// Serial connection to PC over USB
Serial pc(USBTX, USBRX);

// ADC for steering angle potentiometer
AnalogIn ain(A0);

// PWM output to driver for steering motor
PwmOut steer_pwm(PC_7);

// Driver class for Bosch BNO055 absolute orientation sensor
BNO055 imu(I2C_SDA, I2C_SCL);

// Driver class for Lidar Lite v2 rangefinder
LidarLitev2 lidar(I2C_SDA, I2C_SCL);

// lidar_mode_pin idles high (2.2k external pullups). Sensor pulls pin low when
// a new measurement is available.
DigitalIn lidar_mode_pin(PA_5);

// Variables for quadrature odometer encoder
InterruptIn enc_a(PB_3); // D3
InterruptIn enc_b(PB_4); // D5
DigitalIn enc_b_in(PB_4);
int32_t encoder_pos = 0;
uint8_t prev_b = 0;

// Status indicator
DigitalOut led(LED1);

Timer timer;

// Interrupt handler for quadrature decoder readout (channel a)
void decode_a()
{
  prev_b ? encoder_pos-- : encoder_pos++;
  led = !led;
}

// Interrupt handler for quadrature decoder readout (channel b)
void decode_b()
{
  prev_b = !prev_b;
}

void setup_imu()
{
  imu.reset();
  while (!imu.check())
  {
    led = !led;
    wait(0.1);
  }
  led.write(0);
  imu.setmode(OPERATION_MODE_NDOF);
}

void print()
{
  pc.printf("t:%d,AMGS:%d%d%d%d,qw:%d,qx:%d,qy:%d,qz:%d,pot:%d,odo:%d",
            timer.read_ms(),
            imu.cal.accel,
            imu.cal.mag,
            imu.cal.gyro,
            imu.cal.system,
            imu.quat.raww,
            imu.quat.rawx,
            imu.quat.rawy,
            imu.quat.rawz,
            (int)(1000*pot),
            encoder_pos);
  if (lidar_attached && !lidar_mode_pin)
  {
    pc.printf(",d=%d", lidar.distanceContinuous()); // cm
  }
  pc.printf("\r\n");
}

int main()
{
  pc.baud(115200);

  // Configure LidarLite for continuous mode, which means that a new measurement
  // will be available each time lidar_mode_pin drops low.
  if (lidar_attached)
  {
    lidar.configure();
    lidar.beginContinuous();
  }
  setup_imu();
  timer.start();
  steer_pwm.period_us(250);

  // First reading in smoothed time-series average
  pot = ain.read();

  // Initialize encoder channel b
  prev_b = enc_b_in.read();

  // Attach interrupt handlers for quadrature decoder channels a,b
  enc_a.rise(&decode_a);
  enc_b.rise(&decode_b);
  enc_b.fall(&decode_b);

  while (true)
  {
    pot = alpha*ain.read() + (1 - alpha)*pot;

    imu.get_calib();
    imu.get_quat();
    print();
    wait(0.02);
  }
}
