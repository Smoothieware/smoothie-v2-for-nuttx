#include "Laser.h"

#include <algorithm>

#define enable_key "enable"
#define pwm_pin_key "pwm_pin"
#define inverted_key "inverted_pwm"
#define ttl_pin_key "ttl_pin"
#define pwm_period_key "pwm_period"
#define maximum_power_key "maximum_power"
#define minimum_power_key "minimum_power"
#define maximum_s_value_key "maximum_s_value"


Laser::Laser() : Module("laser")
{
    laser_on = false;
    scale= 1;
    manual_fire= false;
}

bool configure(ConfigReader& cr)
{

    ConfigReader::section_map_t m;
    if(!cr.get_section("laser", m)) return false;

    if( !cr.get_bool(m,  enable_key , false) ) {
        // as not needed free up resource
        return false;
    }

    pwm_pin= new Pwm();
    pwm_pin->from_string(cr.get_string(m, pwm_pin_key, "nc"))->as_output();

    if(!pwm_pin.is_valid()) {
        printf("Error: laser-config: Specified pin is not a valid PWM pin.\n");
        delete pwm_pin;
        return false;
    }

    this->pwm_inverting = cr.get_bool(m, inverted_key, false);

    // TTL settings
    this->ttl_pin = new Pin();
    ttl_pin->from_string( cr.get_string(m, ttl_pin_key, "nc" ))->as_output();
    this->ttl_used = ttl_pin->connected();
    this->ttl_inverting = ttl_pin->is_inverting();
    if (ttl_used) {
        ttl_pin->set(0);
    } else {
        delete ttl_pin;
        ttl_pin = NULL;
    }

    // this is set globally in Pwm
    //uint32_t period= cr.get_float(m, pwm_period_key, 20);
    //this->pwm_pin->period_us(period);

    this->pwm_pin->set(this->pwm_inverting ? 1 : 0);
    this->laser_maximum_power = cr.get_float(m, maximum_power_key, 1.0f) ;
    this->laser_minimum_power = cr.get_float(m, minimum_power_key, 0) ;

    // S value that represents maximum (default 1)
    this->laser_maximum_s_value = cr.get_float(m, maximum_s_value_key, 1.0f) ;

    set_laser_power(0);

    // register command handlers
    using std::placeholders::_1;
    using std::placeholders::_2;

    THEDISPATCHER->add_handler( "fire", std::bind( &Laser::handle_fire_cmd, this, _1, _2) );
    THEDISPATCHER->add_handler(Dispatcher::MCODE_HANDLER, 221, std::bind(&Laser::handle_gcodes, this, _1, _2));

    //register for events
    // this->register_for_event(ON_HALT);
    // this->register_for_event(ON_GCODE_RECEIVED);
    // this->register_for_event(ON_CONSOLE_LINE_RECEIVED);
    // this->register_for_event(ON_GET_PUBLIC_DATA);

    // no point in updating the power more than the PWM frequency, but no more than 1KHz

    // THEKERNEL->slow_ticker->attach(std::min(1000UL, 1000000/period), this, &Laser::set_proportional_power);
}

bool Laser::handle_fire_cmd( std::string& params, OutputStream& os )
{
    HELP("fire laser: 0-100 | off");

    if(THEROBOT->is_halted()) return true; // if in halted state ignore any commands

    std::string power = stringutils::shift_parameter( params );
    if(power.empty()) {
        os.printf("Usage: fire power%%|off\n");
        return true;
    }

    float p;
    if(power == "off" || power == "0") {
        p= 0;
        os.printf("turning laser off and returning to auto mode\n");

    }else{
        p= strtof(power.c_str(), NULL);
        p= confine(p, 0.0F, 100.0F);
        os.printf("WARNING: Firing laser at %1.2f%% power, entering manual mode use fire off to return to auto mode\n", p);
    }

    p= p/100.0F;
    manual_fire= set_laser_power(p);
}

// returns instance
bool Laser::request(const char *key, void *value)
{
    if(strcmp(key, "get_instance") == 0) {
        *(Laser*)value= this;
        return true;
    }

    return false;
}

bool Laser::handle_gcodes(GCode& gcode, OutputStream& os)
{
    if (gcode.get_code() == 221) { // M221 S100 change laser power by percentage S
        if(gcode.has_arg('S')) {
            this->scale= gcode.get_arg('S') / 100.0F;

        } else {
            os.printf("Laser power scale at %6.2f %%\n", this->scale * 100.0F);
        }
    }
}

// calculates the current speed ratio from the currently executing block
float Laser::current_speed_ratio(const Block *block) const
{
    // find the primary moving actuator (the one with the most steps)
    size_t pm= 0;
    uint32_t max_steps= 0;
    for (size_t i = 0; i < THEROBOT->get_number_registered_motors(); i++) {
        // find the motor with the most steps
        if(block->steps[i] > max_steps) {
            max_steps= block->steps[i];
            pm= i;
        }
    }

    // figure out the ratio of its speed, from 0 to 1 based on where it is on the trapezoid,
    // this is based on the fraction it is of the requested rate (nominal rate)
    float ratio= block->get_trapezoid_rate(pm) / block->nominal_rate;

    return ratio;
}

// get laser power for the currently executing block, returns false if nothing running or a G0
bool Laser::get_laser_power(float& power) const
{
    const Block *block = StepTicker::getInstance()->get_current_block();

    // Note to avoid a race condition where the block is being cleared we check the is_ready flag which gets cleared first,
    // as this is an interrupt if that flag is not clear then it cannot be cleared while this is running and the block will still be valid (albeit it may have finished)
    if(block != nullptr && block->is_ready && block->is_g123) {
        float requested_power = ((float)block->s_value/(1<<11)) / this->laser_maximum_s_value; // s_value is 1.11 Fixed point
        float ratio = current_speed_ratio(block);
        power = requested_power * ratio * scale;

        return true;
    }

    return false;
}

// called every millisecond from timer ISR
uint32_t Laser::set_proportional_power(uint32_t dummy)
{
    if(manual_fire) return 0;

    float power;
    if(get_laser_power(power)) {
        // adjust power to maximum power and actual velocity
        float proportional_power = ( (this->laser_maximum_power - this->laser_minimum_power) * power ) + this->laser_minimum_power;
        set_laser_power(proportional_power);

    } else if(laser_on) {
        // turn laser off
        set_laser_power(0);
    }
    return 0;
}

bool Laser::set_laser_power(float power)
{
    // Ensure power is >=0 and <= 1
    power= confine(power, 0.0F, 1.0F);

    if(power > 0.00001F) {
        this->pwm_pin->write(this->pwm_inverting ? 1 - power : power);
        if(!laser_on && this->ttl_used) this->ttl_pin->set(true);
        laser_on = true;

    }else{
        this->pwm_pin->write(this->pwm_inverting ? 1 : 0);
        if (this->ttl_used) this->ttl_pin->set(false);
        laser_on = false;
    }

    return laser_on;
}

void Laser::on_halt(bool flg)
{
    if(flg) {
        set_laser_power(0);
        manual_fire= false;
    }
}

float Laser::get_current_power() const
{
	float p= pwm_pin->read();
    return (this->pwm_inverting ? 1 - p : p) * 100;
}
