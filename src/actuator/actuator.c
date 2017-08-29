/******************************************************************************
 * File: actuator.c
 * Description: Test application to control two actuators, a cooler and a
 *              heater.
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
#include <unistd.h>

#include <bcm2835.h>

#include "actuator.h"

#define PIN_HEATER    RPI_V2_GPIO_P1_11
#define PIN_COOLER    RPI_V2_GPIO_P1_13

int main(int argc, char **argv)
{
  struct actuator cooler, heater;
  cooler.pin = PIN_COOLER;
  cooler.invert = 1;
  heater.pin = PIN_HEATER;
  heater.invert = 1;

  /* initialisation */
  init_actuator(&heater, false);
  init_actuator(&cooler, false);

  return 0;
}

