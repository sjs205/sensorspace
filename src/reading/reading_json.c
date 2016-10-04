/******************************************************************************
 * File: reading_json.c
 * Description: functions to convert readings to/from JSON format
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
#include "reading.h"

/**
 * \brief Crude conversion of reading to JSON (minified)
 */
int convert_reading_json(struct reading *r, char *buf, size_t *len) {

  int i;
  ssize_t l = 0;

  l = snprintf(buf, *len,
      "{\"reading\":{\"date\":\"%02d-%02d-%02d %02d:%02d:%02d\",",
      r->t.tm_year + 1900, r->t.tm_mon + 1, r->t.tm_mday, r->t.tm_hour,
      r->t.tm_min, r->t.tm_sec);
  if (l < 0) {
    goto error;
  }

  /* Set device_id */
  if (r->device_id) {
    l += snprintf(buf + l, *len - l, "\"device\":{\"id\":\"%d\"},",
        r->device_id);
    if (l < 0) {
      goto error;
    }
  }

  /* Set measurements */
  l += snprintf(buf + l, *len - l, "\"sensors\":[");
  if (l < 0) {
    goto error;
  }

  if (r->count) {

    for (i = 0; i < r->count; i++) {
      /* add comma for multiple measurements */
      if (i > 0) {
        *(buf + l++) = ',';
      }

      l += snprintf(buf + l, *len - l, "{\"id\":\"%d\",\"meas\":\"%s\""
          ",\"name\":\"%s\"}",
          r->meas[i]->sensor_id, r->meas[i]->meas, r->meas[i]->name);
      if (l < 0) {
        goto error;
      }
    }
  }

  /* finish and close */
  l += snprintf(buf + l, *len - l, "]}}");
  *len = l + 1;

  return SS_SUCCESS;

error:
  *len = 0;
  log_stderr(LOG_ERROR, "Failed to convert reading to JSON");
  return SS_WRITE_ERROR;
}

/**
 * \brief process reading in JSON format
 */
int convert_json_reading(struct reading *r, char *buf, size_t len) {
  return 0;
}

