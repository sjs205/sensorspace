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

#include <stdlib.h>
#include <time.h>
#include <rrd.h>

#include "reading.h"

#define RRD_UPDATE_ARG_COUNT      4
#define RRD_UPDATE_ARG_0          "update"

/*
 * \brief function to initialise a new RRD file.
 */
int rrd_file_init(struct rrdtool *rrd, char *file) {

  log_stdout(LOG_INFO, "New RRD database added: %s", file);
  if (rrd->f_count + 1 <= RRD_MAX_SENSORS) {
    if (!(rrd->file[rrd->f_count++] = calloc(1, sizeof(struct rrd_file)))) {
      log_stderr(LOG_ERROR, "RRDtool: Out of memory");
      goto free;
    }
  } else {
    log_stderr(LOG_ERROR, "RRDtool: Exceded max number of files: %d",
        RRD_MAX_SENSORS);
    return SS_OUT_OF_MEM_ERROR;
  }

  /* Copy filename */
  if (!strncpy(rrd->file[rrd->f_count - 1]->name, file,
        sizeof(struct rrd_file))) {
    log_stderr(LOG_ERROR, "RRDtool: Failed to copy rrd file name");
    return SS_INIT_ERROR;
  }

  return SS_SUCCESS;

free:
  free_rrd_file(rrd->file[--rrd->f_count]);
  return SS_OUT_OF_MEM_ERROR;
}

/*
 * \brief function to add a raw measurement to an RR database.
 */
static int add_measurement_rrd(struct reading *r, unsigned m_idx,
    struct rrd_file *file) {

  int ret = SS_SUCCESS;
  char buf[32];

  sprintf(buf, "%lld:%s", (long long) mktime(&r->t), r->meas[m_idx]->meas);

  const char *updateparams[] = {
    RRD_UPDATE_ARG_0,
    file->name,
    buf,
    NULL
  };

  log_stderr(LOG_DEBUG, "RRD: Sensor ID: %d, Measurement: %s, File: %s",
      r->meas[m_idx]->sensor_id, r->meas[m_idx]->meas, file);
  log_stderr(LOG_DEBUG, "[rrdtool] %s, %s, %s, %s",
      updateparams[0], updateparams[1], updateparams[2], updateparams[3]);

#if 0
  /* Full debug */
  rrd_info_t *rrd_inf = rrd_update_v(3, (char **)updateparams);

  /* need to extract return value */
  rrd_info_print(rrd_inf);

#else
  ret = rrd_update(3, (char **)&updateparams);
#endif
  if (ret) {
    log_stderr(LOG_ERROR, "rrd_error: %s", rrd_get_error());
    rrd_clear_error();
    ret = SS_POST_ERROR;
  } else {
    log_stdout(LOG_INFO, "Measurement added to RRD: %s", buf);
  }

        return ret;
}

/*
 * \brief function to add a reading to an RR database.
 */
int add_reading_rrd(struct reading *r, struct rrdtool *rrd) {

  int ret = SS_SUCCESS;
  unsigned i, j;

  for (i = 0; i < r->count; i++) {
    for (j = 0; j < rrd->f_count; j++) {
      if (r->meas[i]->sensor_id == rrd->sensor_id[j]) {
        log_stdout(LOG_INFO, "Processing measurement from sensor: %d - %s",
            r->meas[i]->sensor_id, r->meas[i]->name);

        ret = add_measurement_rrd(r, i, rrd->file[j]);
        if (ret) {
          log_stderr(LOG_ERROR, "rrdtool message add failed");
        }
      }
    }
  }

  return ret;
}

/*
 * \brief function to free an rrd_file struct.
 */
void free_rrd_file(struct rrd_file *file) {

  if (file)
    free(file);

  return;
}

/*
 * \brief function to free all files in a rrdtool struct.
 */
void free_rrd_files(struct rrdtool *rrd) {

  int i;
  for (i = rrd->f_count; i >= 0; i--) {
    if (rrd->file[i]) {
      free_rrd_file(rrd->file[i]);
    }
  }

  return;
}
