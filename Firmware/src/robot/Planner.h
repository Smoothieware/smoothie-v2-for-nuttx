/*
      This file is part of Smoothie (http://smoothieware.org/). The motion control part is heavily based on Grbl (https://github.com/simen/grbl).
      Smoothie is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
      Smoothie is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
      You should have received a copy of the GNU General Public License along with Smoothie. If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

#include <stdint.h>
#include <cmath>
#include "ActuatorCoordinates.h"

class Block;
class PlannerQueue;
class ConfigReader;

#define N_PRIMARY_AXIS 3
class Robot;

class Planner
{
public:
    Planner();
    bool configure(ConfigReader& cr);
    static Planner *getInstance() { return instance; }

private:
    static Planner *instance;
    float max_exit_speed(Block *);
    float max_allowable_speed( float acceleration, float target_velocity, float distance);

    void calculate_trapezoid(Block *, float entry_speed, float exit_speed );
    float reverse_pass(Block *, float exit_speed);
    float forward_pass(Block *, float next_entry_speed);
    void prepare(Block *, float acceleration_in_steps, float deceleration_in_steps);

    bool append_block(ActuatorCoordinates& target, uint8_t n_motors, float rate_mm_s, float distance, float unit_vec[], float accleration, float s_value, bool g123);
    void recalculate();

    double fp_scale; // optimize to store this as it does not change

    PlannerQueue *queue;
    float previous_unit_vec[N_PRIMARY_AXIS];

    float xy_junction_deviation{0.05F};    // Setting
    float z_junction_deviation{NAN};  // Setting
    float minimum_planner_speed{0.0F}; // Setting
    int planner_queue_size{32}; // setting

    // FIXME should really just make getters and setters or handle the set/get gcode here
    friend Robot;
};
