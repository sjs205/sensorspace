/******************************************************************************
 * File: reading_ini.c
 * Description: functions to convert readings to/from ini text format
 * Athor: Steven Swann - swannonline@googlemail.com
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

#include "reading.h"

#define INI_MAX_LINE_LEN        256
#define INI_DEVICEID_SUBSTR     "DID="
#define INI_DATE_SUBSTR         "DATE="
#define INI_MEAS_SUBSTR         "MEAS="
#define INI_MEAS_DELIM_CHAR     ";"
#define INI_STR_EOL             "\n"
#define INI_DELIM_CHAR          "="

/**
 * \brief process reading in ini format
 */
int convert_ini_reading(struct reading *r, char *buf, size_t len) {

  char tmp[INI_MAX_LINE_LEN];
  char *idx = NULL;
  char *t_idx = NULL;
  int ret = SS_SUCCESS;
  size_t t_len = 0;
  int line = 0;
  struct measurement *m = NULL;

  /* Process buf line by line */
  idx = buf;
  do {
    /* copy line to tmp */
    if (strstr(idx, "\n")) {
      t_len = (size_t)strstr(idx, INI_STR_EOL) - (size_t)idx;
      strncpy(tmp, idx, t_len);
      tmp[t_len] = '\0';

      line++;

      /* next line */
      idx += t_len + 1;

    } else {
      /* no new lines => EOF */
      ret = SS_SUCCESS;
      break;
    }

    /* new section => new reading */
    if (tmp[0] == '[') {
      /* not currenty supported. */
      if (line > 1) {
        log_stderr(LOG_ERROR, "Multiple readings not currently supported");
        ret = SS_INI_ERROR;
        free_reading(r);
        break;
      }
      continue;
    }

    /* device_id */
    if (strstr(tmp, INI_DEVICEID_SUBSTR)) {
      t_idx = strstr(tmp, INI_DELIM_CHAR) + 1;

      r->device_id = atoi(t_idx);
    }

    /* reading date */
    if (strstr(tmp, INI_DATE_SUBSTR)) {
      t_idx = strstr(tmp, INI_DELIM_CHAR) + 1;

      convert_db_date_to_tm((char *)t_idx, &r->t);
    }

    /* reading measurement */
    if (strstr(tmp, INI_MEAS_SUBSTR)) {

      if (measurement_init(r)) {
        log_stderr(LOG_ERROR, "Failed to init measurement");
        ret = SS_INI_ERROR;
        free_reading(r);
        break;
      } else {

        m = r->meas[r->count - 1];

        /* sensor_id */
        t_idx = strstr(tmp, INI_DELIM_CHAR) + 1;
        if (!t_idx) {
          log_stderr(LOG_ERROR, "Failed to init measurement");
          ret = SS_INI_ERROR;
          free_reading(r);
          break;
        }
        m->sensor_id = atoi(t_idx);

        /* measurement */
        t_idx = strstr(tmp, INI_MEAS_DELIM_CHAR) + 1;
        if (!t_idx) {
          log_stderr(LOG_ERROR, "Failed to init measurement");
          ret = SS_INI_ERROR;
          free_reading(r);
          break;
        }

        if (strcpy(m->meas, t_idx) <= 0) {
          log_stderr(LOG_ERROR, "Measurement data invalid");
          ret = SS_INI_ERROR;
          free_reading(r);
          break;
        }

        log_stdout(LOG_DEBUG, "New measurement:");
        log_stdout(LOG_DEBUG, "sensor_id: %d", m->sensor_id);
        log_stdout(LOG_DEBUG, "meas: %s", m->meas);
      }
    }
  }  while ((size_t)idx < (size_t)buf + (size_t)len);

  ret = validate_reading(r);
  if (ret) {
    log_stderr(LOG_ERROR, "Reading validation failed");
  }

  return ret;
}


