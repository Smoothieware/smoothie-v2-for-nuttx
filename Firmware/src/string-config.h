static const char *string_config= "\
[general]\
second_usb_serial_enable = false # This enables a second USB serial port\
kill_button_enable = true # Set to true to enable a kill button\
kill_button_pin = 2.12 # Kill button pin. default is same as pause button 2.12 (2.11 is another good choice)\
uart0.baud_rate = 115200 # Baud rate for the default hardware ( UART ) serial port\
leds_disable = true             # Disable using leds after config loaded\
play_led_disable = true             # Disable the play led\
\
[motion control]\
default_feed_rate = 4000 # Default speed (mm/minute) for G1/G2/G3 moves\
default_seek_rate = 4000 # Default speed (mm/minute) for G0 moves\
mm_per_arc_segment = 0.0 # Fixed length for line segments that divide arcs, 0 to disable\
mm_max_arc_error = 0.01 # The maximum error for line segments that divide arcs 0 to disable\
arc_correction = 5\
default_acceleration = 3000 # Acceleration in mm/second/second.\
junction_deviation = 0.05 # See http://smoothieware.org/motion-control#junction-deviation\
default_acceleration = 100.0 # default acceleration in mm/sec²\
arm_solution = delta\
\
[planner]\
junction_deviation = 0.05\
#z_junction_deviation = 0.0\
minimum_planner_speed = 0\
planner_queue_size = 32\
\
[actuator]\
alpha.steps_per_mm = 80 # Steps per mm for alpha ( X ) stepper\
alpha.step_pin = p2_9 # Pin for alpha stepper step signal\
alpha.dir_pin = P3_2 # Pin for alpha stepper direction, add '!' to reverse direction\
alpha.en_pin = nc # Pin for alpha enable pin\
alpha.max_rate = 30000.0 # Maximum rate in mm/min\
x.axis_max_speed = 30000 # Maximum speed in mm/min\
\
beta.steps_per_mm = 80 # Steps per mm for beta ( Y ) stepper\
beta.step_pin = p3_1 # Pin for beta stepper step signal\
beta.dir_pin = p2_12 # Pin for beta stepper direction, add '!' to reverse direction\
beta.en_pin = nc # Pin for beta enable\
beta.max_rate = 30000.0 # Maxmimum rate in mm/min\
y.axis_max_speed = 30000 # Maximum speed in mm/min\
\
gamma.steps_per_mm = 1600 # Steps per mm for gamma ( Z ) stepper\
gamma.step_pin = p2_13 # Pin for gamma stepper step signal\
gamma.dir_pin = p7_1 # Pin for gamma stepper direction, add '!' to reverse direction\
gamma.en_pin = nc # Pin for gamma enable\
gamma.max_rate = 300.0 # Maximum rate in mm/min\
z.axis_max_speed = 300 # Maximum speed in mm/min\
gamma.acceleration = 500  # overrides the default acceleration for this axis\
\
[switch]\
fan.enable = false # Enable this module\
fan.input_on_command = M106 # Command that will turn this switch on\
fan.input_off_command = M107 # Command that will turn this switch off\
fan.output_pin = 2.6 # Pin this module controls\
fan.output_type = pwm # PWM output settable with S parameter in the input_on_comand\
\
misc.enable = false             # Enable this module\
misc.input_on_command = M42              # Command that will turn this switch on\
misc.input_off_command = M43              # Command that will turn this switch off\
misc.output_pin = 2.4              # Pin this module controls\
misc.output_type = digital          # Digital means this is just an on or off pin\
\
led1.enable            = true\
led1.input_on_command  = M1\
led1.input_off_command = M2\
led1.output_pin        = gpio1_0\
led1.output_type       = digital\
\
led2.enable            = true\
led2.input_on_command  = M3\
led2.input_off_command = M4\
led2.output_pin        = gpio3_3\
led2.output_type       = sigmadeltapwm\
\
but1.enable             = true                     # Enable this module\
but1.input_pin          = gpio0_7!                 # button\
but1.output_on_command  = M1                       # command to send\
but1.output_off_command = M2                       # command to send\
but1.input_pin_behavior = toggle\
";
