static const char *string_config= "\
[motion control]\n\
default_feed_rate = 4000 # Default speed (mm/minute) for G1/G2/G3 moves\n\
default_seek_rate = 4000 # Default speed (mm/minute) for G0 moves\n\
mm_per_arc_segment = 0.0 # Fixed length for line segments that divide arcs, 0 to disable\n\
mm_max_arc_error = 0.01 # The maximum error for line segments that divide arcs 0 to disable\n\
arc_correction = 5\n\
default_acceleration = 3000 # Acceleration in mm/second/second.\n\
junction_deviation = 0.05 # See http://smoothieware.org/motion-control#junction-deviation\n\
default_acceleration = 100.0 # default acceleration in mm/sec²\n\
arm_solution = cartesian\n\
\n\
[planner]\n\
junction_deviation = 0.05\n\
#z_junction_deviation = 0.0\n\
minimum_planner_speed = 0\n\
planner_queue_size = 32\n\
\n\
[actuator]\n\
alpha.steps_per_mm = 1600 # Steps per mm for alpha ( X ) stepper\n\
alpha.step_pin = p2_10 # Pin for alpha stepper step signal\n\
alpha.dir_pin = p1_14 # Pin for alpha stepper direction, add '!' to reverse direction\n\
alpha.en_pin = p2_11 # Pin for alpha enable pin\n\
alpha.max_rate = 300 # Maximum rate in mm/min\n\
x.axis_max_speed = 500 # Maximum speed in mm/min\n\
\n\
beta.steps_per_mm = 1600 # Steps per mm for beta ( Y ) stepper\n\
beta.step_pin = p1_7 # Pin for beta stepper step signal\n\
beta.dir_pin = p2_9 # Pin for beta stepper direction, add '!' to reverse direction\n\
beta.en_pin = p2_12 # Pin for beta enable\n\
beta.max_rate = 300 # Maxmimum rate in mm/min\n\
y.axis_max_speed = 500 # Maximum speed in mm/min\n\
\n\
#gamma.steps_per_mm = 1600 # Steps per mm for gamma ( Z ) stepper\n\
#gamma.step_pin = p1_13 # Pin for gamma stepper step signal\n\
#gamma.dir_pin = p6_5 # Pin for gamma stepper direction, add '!' to reverse direction\n\
#gamma.en_pin = p6_1 # Pin for gamma enable\n\
#gamma.max_rate = 300.0 # Maximum rate in mm/min\n\
#z.axis_max_speed = 500 # Maximum speed in mm/min\n\
#gamma.acceleration = 500  # overrides the default acceleration for this axis\n\
\n\
delta.enable = false             # Whether to activate the extruder module at all. All configuration is ignored if false\n\
delta.tool_id = 0               # T0 will select\n\
delta.steps_per_mm = 140        # Steps per mm for extruder stepper\n\
delta.default_feed_rate = 600   # Default rate ( mm/minute ) for moves where only the extruder moves\n\
delta.acceleration = 500        # Acceleration for the stepper motor mm/sec²\n\
delta.max_speed = 50            # Maximum speed in mm/s\n\
delta.step_pin = p6_4            # Pin for extruder step signal\n\
delta.dir_pin = p6_5            # Pin for extruder dir signal ( add '!' to reverse direction )\n\
delta.en_pin = p1_7             # Pin for extruder enable signal\n\
\n\
[switch]\n\
fan.enable = false # Enable this module\n\
fan.input_on_command = M106 # Command that will turn this switch on\n\
fan.input_off_command = M107 # Command that will turn this switch off\n\
fan.output_pin = p4.1 # Pin this module controls\n\
fan.output_type = sigmadeltapwm # PWM output settable with S parameter in the input_on_comand\n\
\n\
[extruder]\n\
hotend.enable = true             # Whether to activate the extruder module at all. All configuration is ignored if false\n\
hotend.tool_id = 0               # T0 will select\n\
\n\
[temperature control]\n\
hotend.enable = true             # Whether to activate this ( 'hotend' ) module at all.\n\
hotend.tool_id = 0               # T0 will select\n\
#hotend.thermistor_pin = ADC0_3     # Pin for the thermistor to read\n\
hotend.heater_pin = nc         # Pin that controls the heater, set to nc if a readonly thermistor is being defined\n\
hotend.thermistor = EPCOS100K   # See http://smoothieware.org/temperaturecontrol#toc5\n\
hotend.set_m_code = 104          # M-code to set the temperature for this module\n\
hotend.set_and_wait_m_code = 109 # M-code to set-and-wait for this module\n\
hotend.designator = T            # Designator letter for this module\n\
hotend.pwm_frequency = 10       # FIXME slow for now when is SPIFI\n\
hotend.sensor = max31855 \n\
hotend.chip_select_pin =   p1_5 \n\
hotend.sclk_pin =   pf_4 \n\
hotend.mosi_pin =   p1_4 \n\
hotend.miso_pin =   p1_3 \n\
hotend.spi_channel = 2    # /SPI channel-0 /SSP0 channel-1 /SSP1 channel-2\n\
hotend.readings_per_second = 9 \n\
\n\
[endstops]\n\
common.debounce_ms = 0         # debounce time in ms (actually 10ms min)\n\
\n\
minx.enable = false             # enable an endstop\n\
minx.pin = p5_3!            # pin\n\
minx.homing_direction = home_to_min      # direction it moves to the endstop\n\
minx.homing_position = 0                # the cartesian coordinate this is set to when it homes\n\
minx.axis = X                # the axis designator\n\
minx.max_travel = 500              # the maximum travel in mm before it times out\n\
minx.fast_rate = 30               # fast homing rate in mm/sec\n\
minx.slow_rate = 5               # slow homing rate in mm/sec\n\
minx.retract = 5                # bounce off endstop in mm\n\
minx.limit_enable = false        # enable hard limit\n\
\n\
miny.enable = false                 # enable an endstop\n\
miny.pin = p9_6!                  # pin\n\
miny.homing_direction = home_to_min # direction it moves to the endstop\n\
miny.homing_position = 0            # the cartesian coordinate this is set to when it homes\n\
miny.axis = Y                       # the axis designator\n\
miny.max_travel = 500               # the maximum travel in mm before it times out\n\
miny.fast_rate = 30                 # fast homing rate in mm/sec\n\
miny.slow_rate = 5                  # slow homing rate in mm/sec\n\
miny.retract = 5                    # bounce off endstop in mm\n\
miny.limit_enable = false            # enable hard limits\n\
\n\
minz.enable = false                  # enable an endstop\n\
minz.pin = p2_13!                  # pin\n\
minz.homing_direction = home_to_min # direction it moves to the endstop\n\
minz.homing_position = 0            # the cartesian coordinate this is set to when it homes\n\
minz.axis = Z                       # the axis designator\n\
minz.max_travel = 500               # the maximum travel in mm before it times out\n\
minz.fast_rate = 30                 # fast homing rate in mm/sec\n\
minz.slow_rate = 5                  # slow homing rate in mm/sec\n\
minz.retract = 5                    # bounce off endstop in mm\n\
minz.limit_enable = false            # enable hard limits\n\
\n";
