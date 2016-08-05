#include "mbed.h"
#include "BNO055.h"
#include "LidarLitev2.h"

// Flag indicating whether the LidarLite sensor is hooked up
const bool lidar_attached = true;

// Most recent lidar distance measurement [cm] and bearing [deg] with respect to
// vehicle (0 degrees at front of car, increasing clockwise).
int lidar_range = 0;
float lidar_bearing = 0;

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
PwmOut steer_pwm(D9);

// Driver class for Bosch BNO055 absolute orientation sensor
BNO055 imu(I2C_SDA, I2C_SCL);

// Driver class for Lidar Lite v2 rangefinder
LidarLitev2 lidar(I2C_SDA, I2C_SCL);

// lidar_mode_pin idles high (2.2k external pullups). Sensor pulls pin low when
// a new measurement is available.
DigitalIn lidar_mode_pin(D7);

// Variables for quadrature odometer encoder
InterruptIn enc_a(D11);
InterruptIn enc_b(D12);
DigitalIn enc_b_in(D12);
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
  pc.printf("Configuring IMU sensor\r\n");
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
  pc.printf("t:%d,AMGS:%d%d%d%d,qw:%d,qx:%d,qy:%d,qz:%d,pot:%d,odo:%d,r:%d,b:%d\r\n",
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
            encoder_pos,
            lidar_range,
            lidar_bearing);
}

int main()
{
  pc.baud(115200);

  // Configure LidarLite for continuous mode, which means that a new measurement
  // will be available each time lidar_mode_pin drops low.

  if (lidar_attached)
  {
    pc.printf("Configuring lidar sensor\r\n");
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


  pc.printf("Beginning loop\r\n");
  while (true)
  {
    pot = alpha*ain.read() + (1 - alpha)*pot;

    if (lidar_attached && !lidar_mode_pin)
    {
      lidar_range = lidar.distanceContinuous();
      lidar_bearing = 0; // TODO
    }

    imu.get_calib();
    imu.get_quat();
    print();
    wait(0.02);
  }
}
