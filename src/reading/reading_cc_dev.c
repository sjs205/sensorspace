/******************************************************************************
 * File: reading_cc_dev.c
 * Description: functions to provide currentcost device support devices
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

#include "log.h"
#include "reading.h"

#define CC_DEV_MSG_STRING         "<msg><src>CC128-v"
#define CC_DEV_MSG_END_STRING     "</msg>"
#define CC_DEV_HIST_PKT_STRING    "hist"
#define CC_TEMP_TAG               "tmpr"
#define CC_WATTS_TAG              "watts"
#define CC_SENSORID_TAG           "id"
#define CC_SENSOR_TAG             "sensor"
#define CC_CHANNEL_TAG            "ch1"
#define CC_CHANNEL_CHAR           2
#define CC_MAX_NO_CHANNELS        10

/**
 * \brief Get currentcost XML tag values from input message
 */
static int get_tag_text(char *msg, const char *tag, char *text) {

  int index;
  int ret = SS_SUCCESS;
  char start_tag[10], end_tag[10];

  /* Build Tags */
  int start_tag_len = sprintf(start_tag, "<%s>", tag);
  sprintf(end_tag, "</%s>", tag);

  /* Find start and end tags */
  char *end_tag_index = strstr(msg, end_tag);
  char *start_tag_index = strstr(msg, start_tag);
  if (start_tag_index != NULL && end_tag_index != NULL) {
    /* Start position and text length */
    char *start_pos = start_tag_index + start_tag_len;

    for(index = 0; index <= (((long)end_tag_index - (long)start_pos) - 1);
        index++) {
      text[index] = *(start_pos+index);
    }
    text[index+1] = '\0';

  } else {
    ret = SS_NO_MATCH;
  }

  return ret;
}

/**
 * \brief process incoming data from the device or attached sensors
 */
int convert_cc_dev_reading(struct reading *r, char *buf, size_t len) {

  int ret = SS_SUCCESS;
  char tmp[32] = {0};

  if (strstr(buf, CC_DEV_MSG_STRING)) {
    /* currentcost device message */

    /* discard historic packets - not currently supported */
    if (strstr(buf, CC_DEV_HIST_PKT_STRING)) {
      log_stderr(LOG_ERROR, "Discarding historic packet");
      ret = SS_NO_MATCH;
      goto end;
    }

    /* set reading time to NOW */
    time_t t = time(NULL);
    r->t = *localtime((const time_t *)&t);

    struct measurement *m = NULL;

    /* Temperature measurement */
    ret = get_tag_text(buf, CC_TEMP_TAG, tmp);
    if (!ret) {
      if (measurement_init(r)) {
        log_stderr(LOG_ERROR, "Failed to init measurement");
        goto end;
      }
      m = r->meas[r->count - 1];
      /* remove leading zeros */
      sprintf(m->meas, "%.01f", atof(tmp));
      sprintf(m->name, "Temperature (ºC)");
      log_stdout(LOG_DEBUG, "New temperature measurement: %s degC", m->meas);
    }

    /* Power measurements */
    if (measurement_init(r)) {
      log_stderr(LOG_ERROR, "Failed to init measurement");
      goto end;
    }
    m = r->meas[r->count - 1];

    char sid_c[4] = {0};
    ret = get_tag_text(buf, CC_SENSORID_TAG, sid_c);
    if (!ret) {
      m->sensor_id = atoi(sid_c);
    }

    ret = get_tag_text(buf, CC_WATTS_TAG, m->meas);
    if (!ret) {
      /* remove leading zeros */
      sprintf(m->meas, "%d", atoi(m->meas));
      log_stdout(LOG_DEBUG, "New power measurement: %s watts", m->meas);
    }

    ret = get_tag_text(buf, CC_SENSOR_TAG, tmp);
    if (!ret) {
      tmp[1] = '\0';
      sprintf(m->name, "Power Sensor %d (Watts)", atoi(tmp));
    }

  } else {
    log_stderr(LOG_ERROR, "Current cost message not found");
    ret = SS_NO_MATCH;
  }

end:
  return ret;
}
