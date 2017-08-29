/******************************************************************************
 * File: actuator.h
 * Description: Actuator control functions
 * Author: Steven Swann - swannonline@googlemail.com
 *
 * Copyright (c) swannonline, 2013-2014
 *
 * This file is part of sensorspace.
 *
 * sensorspace is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as sublished by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * sensorspace is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with sensorspace.  If not, see <http://www.gnu.org/licenses/>.
 *
 *****************************************************************************/
#include <stdint.h>
#include <stdbool.h>

#define NOMSG_TIMEOUT_S   360

/*
 * \brief Struct to store actuator state and settings
 * \param pin The pin the actuator is connected to
 * \param name The human readable name of the actuator
 * \param pv_topic The MQTT topic that contains the process variable
 * \param pv_name The name of the measurement contains the process variable
 * \param state The expected state of the controller
 * \param invert Is the actuator pin inerted?
 * \param duty Maximum duty cycle of the actuator
 * \param max_on The maximim on period for an actuator
 * \param last_set The time the state was last set
 */
struct actuator {
  uint8_t pin;
  char name[128];
  char pv_topic[128];
  char pv_name[128];
  bool state;
  bool invert;
  uint8_t duty;
  uint8_t max_on;
  time_t last_set;
};

void set_actuator_state(struct actuator *p);
int init_actuator(struct actuator *p, bool init_state);
