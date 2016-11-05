/******************************************************************************
 * File: reading.c
 * Description: functions to provide a common reading mechanism
 * Author: Steven Swann - swannonline@googlemail.com
 *
 * Copyright (c) swannonline, 2013-2014
 *
 * This file is part of sensorspace.
 *
 * sensorspace is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
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
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "reading.h"

/**
 * \brief initialise new reading struct
 */
int reading_init(struct reading **r_p) {

  struct reading *r;
  if (!(r = calloc(1, sizeof(struct reading)))) {
    log_stderr(LOG_ERROR, "Readings: Out of memory");
    goto free;
  }

  r->count = 0;

  *r_p = r;
  return SS_SUCCESS;

free:
  free_reading(r);
  return SS_OUT_OF_MEM_ERROR;
}

/**
 * \brief validate new reading
 * \param r reading to validate
 */
int validate_reading(struct reading *r) {
  int ret = SS_SUCCESS;

  /* confirm that date is valid */
  time_t t = time(0);
  struct tm *now = localtime(&t);
  if (r->t.tm_mon != now->tm_mon && r->t.tm_year != now->tm_year) {
    /* reading either too old or not set */
    log_stderr(LOG_WARN, "Invalid date, setting date to NOW!");
    memcpy((void *)&r->t, (void *)now, sizeof(struct tm));
  }

  if (r->device_id == 0) {
    log_stderr(LOG_ERROR, "Invalid device_id");
    ret = SS_READING_ERROR;
  }

  int i;
  for (i = 0; i < r->count; i++) {
    if (r->meas[i]->sensor_id == 0) {
      log_stderr(LOG_ERROR, "Invalid measurement sensor_id");
      ret = SS_READING_ERROR;
    }
    if (r->meas[i]->meas[0] == '\0') {
      log_stderr(LOG_ERROR, "Invalid measurement");
      ret = SS_READING_ERROR;
    }
  }

  return ret;
}

/**
 * \brief initialise new measurement in a reading struct
 */
int measurement_init(struct reading *r) {

  if (!(r->meas[r->count++] = calloc(1, sizeof(struct measurement)))) {
    log_stderr(LOG_ERROR, "Measurement: Out of memory");
    goto free;
  }
  return SS_SUCCESS;

free:
  free(r->meas[--r->count]);
  return SS_OUT_OF_MEM_ERROR;
}

/**
 * \brief converts struct tm to db date
 */
int convert_tm_db_date(struct tm *date, char *buf) {

  sprintf(buf, "%02d-%02d-%02d %02d:%02d:%02d",
      date->tm_year + 1900, date->tm_mon + 1, date->tm_mday,
      date->tm_hour, date->tm_min, date->tm_sec);

  return SS_SUCCESS;
}

/**
 * \brief converts string date in db format to struct tm date
 */
int convert_db_date_to_tm(const char *time, struct tm *t) {
  if(time) {
    char *delim_char;

    t->tm_year = atoi(time) - 1900;
    delim_char = strstr(time, "-");

    /* tm_mon in range 0-11 */
    t->tm_mon = atoi(delim_char + 1) - 1;
    delim_char = strstr(delim_char + 1, "-");

    t->tm_mday = atoi(delim_char + 1);
    delim_char = strstr(delim_char + 1, " ");

    t->tm_hour = atoi(delim_char + 1);
    delim_char = strstr(delim_char + 1, ":");

    t->tm_min = atoi(delim_char + 1);
    delim_char = strstr(delim_char + 1, ":");

    t->tm_sec = atoi(delim_char + 1);
  }

    return SS_SUCCESS;
}

/**
 * \brief print reading struct
 */
int print_reading(struct reading *r) {

  int i;

  printf("\n**** New Reading ****\n");
  printf("Device:\n Device_id: %d\n", r->device_id);
  printf("Reading Date: %02d-%02d-%d %02d:%02d:%02d\n", r->t.tm_mday,
      r->t.tm_mon + 1, r->t.tm_year + 1900, r->t.tm_hour, r->t.tm_min,
      r->t.tm_sec);
  printf("\tMeasurements:\n");
  for (i = 0; i < r->count; i++) {
    printf("\tSensor_id: %d\n", r->meas[i]->sensor_id);
    printf("\tName: %s\n", r->meas[i]->name);
    printf("\tMeasurement: %s\n", r->meas[i]->meas);
  }

  return SS_SUCCESS;
}

void free_reading(struct reading *r) {
  if (r) {
    free_measurements(r);
    free(r);
  }
  return;
}

void free_measurements(struct reading *r) {
  int i;
  if (r) {
    for (i = 0; i < r->count; i++) {
      if (r->meas[i]) {
        free(r->meas[i]);
      }
    }
    r->count = 0;
  }
  return;
}
