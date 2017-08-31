/******************************************************************************
 * File: controller.h
 * Description: Controller functions
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

/*
 * \brief Struct to store current state and settings of a PID controller
 * \param sp Set Point
 * \param kp Proportional gain
 * \param ki Integral gain
 * \param kd Derivitive gain
 * \param pv Current process variable
 * \param prev_pv Previous process variable
 * \param pe Proportional error
 * \param ie Integral error
 * \param de Derivitive error
 * \param e Process variable error
 * \param prev_e Previous process variable error
 * \param ie_min The minimum value allowable for the integral error
 * \param ie_max The maximum value allowable for the integral error
 * \param down_on The lower value of the PID output that will drive the PV reducer
 * \param up The upper value of the PID output that will drive the PV increaser
 * \param idle_ulimit The allowable upper band in which no action will be applied
 * \param idle_llimit The allowable lower band in which no action will be applied
 * \param update_count Count of the number of PV updates that have occured
 * \param out The output from the controller
 * \param pv_up State of the PV increaser
 * \param pv_down State of the PV reducer
 * \param pv_sid Process variable sensor_id
 * \param pv_topic The MQTT topic that carries the process variable
 * \param pv_name The name of the measurement that contains the process variable
 */
struct pid_ctrl {
  int sinterval;
  /* input variables */
  float sp;
  float kp;
  float ki;
  float kd;
  float pv;
  float prev_pv;
  float pe;
  float ie;
  float de;
  float e;
  float prev_e;
  float ie_min;
  float ie_max;
  float down_on;
  float up_on;

  float idle_ulimit;
  float idle_llimit;

  /* count variables */
  uint32_t update_count;

  /* output variables */
  float out;
  bool pv_up;
  bool pv_down;

  /* MQTT variables */
  uint32_t pv_sid;
  char pv_topic[128];
  char pv_name[128];
};

extern float pid_controller(struct pid_ctrl *pid);
