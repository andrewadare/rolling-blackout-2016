#include "mbed.h"
#include "BNO055.h"
#include "LidarLitev2.h"
#include "PIDControl.h"

// Flag indicating whether the LidarLite sensor is hooked up
const bool lidar_attached = true;

// Sample interval in milliseconds
const int timestep = 25;

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

// PID gain parameters
float kp = 0.2, ki = 50, kd = 0;

// Initial PID setpoint
const float initial_setpoint = 0.5;

// Serial connection to PC over USB
Serial pc(USBTX, USBRX);

// ADC for steering angle potentiometer
AnalogIn ain(A0);

// PWM output to motor drivers
PwmOut steer_pwm(PWM_OUT); // D3 on Nucleo L432KC
PwmOut throttle_pwm(D6);

// Driver class for Bosch BNO055 absolute orientation sensor
BNO055 imu(I2C_SDA, I2C_SCL);

// Driver class for Lidar Lite v2 rangefinder
LidarLitev2 lidar(I2C_SDA, I2C_SCL);

// PID controller class for steering (eventually also throttle?)
PIDControl pid(kp, ki, kd, initial_setpoint, timestep);

// lidar_mode_pin idles high, dropping low when new data is ready to be read.
DigitalIn lidar_mode_pin(D9);

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
  static int prev_time = 0;

  int now = timer.read_ms();

  if (now - prev_time < timestep)
  {
    return;
  }

  pc.printf("t:%d,AMGS:%d%d%d%d,qw:%d,qx:%d,qy:%d,qz:%d,pot:%d,odo:%d,r:%d,b:%d\r\n",
            now,
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

  lidar_range = 0; // reset

  prev_time = now;
}

int main()
{
  pc.baud(115200);
  timer.start();

  pid.minOutput = 0.0;
  pid.maxOutput = 1.0;
  pid.setPID(kp, ki, kd);

  if (lidar_attached)
  {
    // Configure LidarLite for continuous mode, which means that a new measurement
    // will be available each time lidar_mode_pin drops low.
    pc.printf("\r\nConfiguring lidar sensor\r\n");
    lidar.configure();
    lidar.beginContinuous();
  }

  setup_imu();

  steer_pwm.period_us(250);
  throttle_pwm.period_us(250);

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

    // pid.setpoint = atof(cmd)/1000; // TODO: read from master computer

    pid.update(pot);
    steer_pwm.write(pid.output);

    if (lidar_attached && !lidar_mode_pin)
    {
      lidar_range = lidar.distanceContinuous();
      lidar_bearing = 0; // TODO
    }

    imu.get_calib();
    imu.get_quat();
    print();
  }
}
