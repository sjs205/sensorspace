/******************************************************************************
 * File: pid.c
 * Description: PID controller functions
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
#include "log.h"
#include "controller.h"

/*
 * \brief Function to calculate the controller error
 */
static void set_controller_error(struct pid_ctrl *pid) {
  pid->e = pid->sp - pid->pv;
  return;
}

/*
 * \brief Function to calculate the proportional error
 */
static float proportional_control(struct pid_ctrl *pid) {
  pid->pe = pid->kp * pid->e;
  return (pid->pe);
}

/*
 * \brief Function to calculate the integral error
 */
static float intergral_control(struct pid_ctrl *pid) {
  /* The following only works when sampling inteval = 1 second */
  pid->ie += pid->e * pid->ki;

  if (pid->ie_max && pid->ie > pid->ie_max) {
    pid->ie = pid->ie_max;
  } else if (pid->ie_min && pid->ie < pid->ie_min) {
    pid->ie = pid->ie_min;
  }
  return (pid->ie);
}

/*
 * \brief Function to calculate the derivative error
 */
static float derivative_control(struct pid_ctrl *pid) {
#if 0
  /* Error based derivative */
  pid->de = pid->kd * (pid->prev_e - pid->e);
#else
  /* PV based derivative - with elimination of derivetive kick */
  pid->de = pid->kd * -(pid->pv - pid->prev_pv);
#endif

  pid->prev_e = pid->e;
  pid->prev_pv = pid->pv;
  return (pid->de);
}

/*
 * \brief Function to set the controller state - on/off - based on the
 *        current error and limits
 */
static void set_control_state(struct pid_ctrl *pid) {

  /* ToDo:
     * Stablise output
     * Rationalise pid output
     */
  if ((pid->idle_ulimit && pid->pv <= pid->sp + pid->idle_ulimit)
      && (pid->idle_llimit && pid->pv >= pid->sp + pid->idle_llimit)) {
      /* pv within idle range, do noting */
      pid->pv_down = false;
      pid->pv_up = false;
  } else {
    /* set state based on controller output */
    if (pid->out < pid->down_on) {
      pid->pv_up = false;
      pid->pv_down = true;
    } else if (pid->out > pid->up_on) {
      pid->pv_down = false;
      pid->pv_up = true;
    } else {
      /* steady state, do nothing */
      pid->pv_down = false;
      pid->pv_up = false;
    }
  }
  return;
}

/*
 * \brief Function to perfrom the PID controller action
 */
extern float pid_controller(struct pid_ctrl *pid) {
    set_controller_error(pid);
    pid->out = proportional_control(pid);
    pid->out += intergral_control(pid);
    pid->out += derivative_control(pid);
    set_control_state(pid);
    return pid->out;
}
