#include "mbed.h"
#include "BNO055.h"
#include "PIDControl.h"

// #define TUNE_PID 1 // Uncomment to use PIDTuner
#ifdef TUNE_PID
#include "PIDTuner.h"
#endif

// Sample interval in milliseconds
const int timestep = 25;


// Smoothing parameter for exponentially-weighted moving average s[t] of
// time series measurements y[t]:
//     s[t] = alpha*y[t-1] + (1-alpha)*s[t-1], 0 < alpha <= 1
// Small alpha: strong smoothing, with slower response to trends
// Large alpha: less noise damping but faster response
const float alpha = 0.3;

// Steering angle potentiometer reading in the range [0,1]
float pot;

// PID gain parameters
float kp = 0.2, ki = 50, kd = 0;

// Initial PID setpoint
const float initial_setpoint = 0.5;

// Serial connection to PC over USB
Serial pc(USBTX, USBRX);
char cmd[100];
char steer_cmd[100];

// ADC for steering angle potentiometer
AnalogIn ain(A0);

// PWM output to motor drivers
PwmOut steer_pwm(D3);
PwmOut throttle_pwm(D10);

// Driver class for Bosch BNO055 absolute orientation sensor
BNO055 imu(I2C_SDA, I2C_SCL);

// PID controller class for steering (eventually also throttle?)
PIDControl pid(kp, ki, kd, initial_setpoint, timestep);

#ifdef TUNE_PID
PIDTuner tuner(pid);
#endif

// Lidar angle sensor pins and bearing angle [deg] with respect to vehicle
// centerline (0 degrees at front of car, increasing clockwise).
InterruptIn lidar_enc_pin(D0);
InterruptIn lidar_rev_pin(D1);
volatile int lidar_angle_counter = 0;
volatile int lidar_angle_counter_max = 0; // tmp
const int lidar_encoder_period = 1346; // Counts per rotation (empirical)

// Read LidarLite v2 sensor in PWM mode (I2C interface was flaky!)
// Measured pulse width in microseconds = distance measurement in mm.
InterruptIn lidar_pulse_pin(D9);
volatile int lidar_pulse_start = 0; // us
volatile int lidar_pulse_width = 0; // us

// Variables for quadrature odometer encoder
InterruptIn enc_a(D11);
InterruptIn enc_b(D12);
DigitalIn enc_b_in(D12);
volatile int32_t encoder_pos = 0;
volatile uint8_t prev_b = 0;

// Status indicator
DigitalOut led(LED1);

// Timers
Timer main_timer;  // Records overall time and ms intervals - don't reset
Timer pulse_timer; // Measures pulse widths - frequently reset

// Interrupt handlers for channels A and B of quadrature rotary encoder
void decode_a()
{
  prev_b ? encoder_pos-- : encoder_pos++;
  led = !led;
}
void decode_b()
{
  prev_b = !prev_b;
}

// Interrupt handlers to measure PWM pulse width
void on_lidar_pulse_rise()
{
  lidar_pulse_start = pulse_timer.read_us();
}
void on_lidar_pulse_fall()
{
  lidar_pulse_width = pulse_timer.read_us() - lidar_pulse_start;
  pulse_timer.reset(); // to avoid rollover (resumes from 0)
}

void on_new_revolution()
{
  lidar_angle_counter_max = lidar_angle_counter; // tmp
  lidar_angle_counter = 0;
}
void on_lidar_encoder_rise()
{
  lidar_angle_counter++;
}

void setup_imu()
{
  pc.printf("Configuring IMU sensor\r\n");
  imu.reset();
  int timeout_counter = 0;
  while (!imu.check())
  {
    led = !led;
    wait(0.1);
    timeout_counter++;
    if (timeout_counter > 100)
    {
      pc.printf("ERROR: Problem connecting to orientation sensor\r\n");
    }
  }
  led.write(0);
  imu.setmode(OPERATION_MODE_NDOF);
}

void print()
{
  static int prev_time = 0;

  int now = main_timer.read_ms();

  if (now - prev_time < timestep)
  {
    return;
  }

  // Compute lidar bearing angle in degrees from encoder pulse count
  int lidar_bearing = float(lidar_angle_counter)/lidar_encoder_period * 360;
  if (lidar_bearing > 359) lidar_bearing = 359;

  pc.printf("t:%d,AMGS:%d%d%d%d,qw:%d,qx:%d,qy:%d,qz:%d,sa:%d,odo:%d,r:%d,b:%d\r\n",
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
            lidar_pulse_width, // = range in mm
            lidar_bearing);

  pc.printf("%d", lidar_angle_counter_max); // tmp

  prev_time = now;
}

int main()
{
  pc.baud(115200);
  main_timer.start();
  pulse_timer.start();

  pid.minOutput = 0.0;
  pid.maxOutput = 1.0;
  pid.setPID(kp, ki, kd);

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

  // Attach interrupt handlers for LidarLite PWM (mode) pin.
  lidar_pulse_pin.rise(&on_lidar_pulse_rise);
  lidar_pulse_pin.fall(&on_lidar_pulse_fall);

  // Interrupt handlers for lidar angle counters
  lidar_enc_pin.rise(&on_lidar_encoder_rise);
  lidar_rev_pin.rise(&on_new_revolution);

  pc.printf("Beginning loop\r\n");

  while (true)
  {
    pot = alpha*ain.read() + (1 - alpha)*pot;

    // pid.setpoint = atof(cmd)/1000; // TODO: read from master computer
#ifdef TUNE_PID
    if (pc.readable())
    {
      tuner.handleByte(pid, pc.getc());
      if (strlen(tuner.message) > 0)
      {
        pc.printf(tuner.message);
      }
    }
#endif

    pid.update(pot);

    steer_pwm.write(pid.output);

    imu.get_calib();
    imu.get_quat();

    print();
  }
}
