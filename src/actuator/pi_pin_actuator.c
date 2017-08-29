/******************************************************************************
 * File: pi_pin_actuator.c
 * Description: Pin actuator functions specific to the raspberry pi
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

#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/mman.h>
#include <time.h>

#include <bcm2835.h>

#include "actuator.h"

void set_actuator_state(struct actuator *a) {

  a->last_set = time(NULL);
  bcm2835_gpio_write(a->pin, a->invert ? !a->state : a->state);

  return;
}

int init_actuator(struct actuator *a, bool init_state) {

  if (!bcm2835_init()) {
    return 1;
  }

  /* set initial states */
  bcm2835_gpio_fsel(a->pin, BCM2835_GPIO_FSEL_OUTP);
  a->state = init_state;
  set_actuator_state(a);

  return 0;
}
