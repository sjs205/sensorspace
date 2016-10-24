/******************************************************************************
 * File: rrd.c
 * Description: Functions to implement rrdtoop integration
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

#include <time.h>
#include <rrd.h>

#define RRD_MAX_FILENAME_LEN      1024
#define RRD_MEASUREMENT_LEN       32
#define RRD_MAX_SENSORS           32

struct rrd_file {
  char f[RRD_MAX_FILENAME_LEN];
};

struct rrdtool {
  unsigned sensor_id[RRD_MAX_SENSORS];
  struct rrd_file *file[RRD_MAX_SENSORS];
  unsigned f_count;
};

int add_reading_rrd(struct readings *r, struct rrdtool *rrd) {

  int i;
  int ret = SS_SUCCESS;
  char meas[RRD_MEASUREMENT_LEN];

  for (i = 0; i < r->count; i++) {
    if (r->meas[i]->sensor_id == rrd->sensor_id) {
      ret = add_measurement_rrd(r->meas[i], rrd);

    }
  }

  return SS_SUCCESS;
}

/*
 * \brief function to add a raw measurement to an RR database.
 */
static int add_measurement_rrd(uint32_t sensor_id, time_t date, char *meas,
    struct rrd_file *f) {

  int ret = SS_SUCCESS;

  sprintf(buf, "%lld:%s", (long long) mktime(r->t), r->meas);

  char *updateparams[] = {
    "update",
    f->file,
    buf,
    NULL
  };

  ret = rrd_update(3, updateparams);

  return SS_SUCCESS;
}
