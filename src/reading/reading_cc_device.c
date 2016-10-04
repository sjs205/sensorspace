/******************************************************************************
 * File: reading_cc_device.c
 * Description: functions to provide currentcost device support
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
#include "cc_device.h"

#define CC_TEMP_TAG               "tmpr"
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
  int start_tag_len = sprintf(start_tag, "<%s>",tag);
  sprintf(end_tag, "</%s>",tag);

  /* Find start and end tags */
  char *end_tag_index = strstr(msg,end_tag);
  char *start_tag_index = strstr(msg,start_tag);
  if(start_tag_index != NULL && end_tag_index != NULL) {
    /* Start position and text length */
    char *start_pos = start_tag_index+start_tag_len;

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
 * \brief Process incoming data from a cc_device and convert to reading
 */
int convert_cc_dev_input(struct reading **r_p, char *buf, size_t len) {

  int ret = SS_SUCCESS;
  int i;
  char tmp[32];

  /* init reading */
  struct reading *r;
  if (reading_init(&r)) {
    log_stderr(LOG_ERROR, "Failed to init reading");
    goto error;
  }

  if (strstr(buf, CC_DEV_MSG_STRING)) {
    /* currentcost device message */

    /*
    char sIDc[4] = "\0";
    if (!get_tag_text(buf, "sensor", sIDc)) {
      int sID = atoi(sIDc);
    }
    */
    /* set reading time to NOW */
    time_t t = time(NULL);
    r->t = *localtime((const time_t *)&t);

    struct measurement *m = NULL;

    /* Temperature measurement */
    ret = get_tag_text(buf, CC_TEMP_TAG, m->meas);
    if (!ret) {
      if (measurement_init(r)) {
        log_stderr(LOG_ERROR, "Failed to init measurement");
        goto error;
      }

      m = r->meas[r->count - 1];
      /* remove leading zeros */
      sprintf(m->meas, "%.01f", atof(m->meas));
      log_stdout(LOG_DEBUG,
          "New temperature measurement: %s degC", m->meas);
    }

    /* Power measurements */
    char ch_tag[4] = CC_CHANNEL_TAG;
    /* for each possible channel */
    for (i = 1; i < CC_MAX_NO_CHANNELS; i++) {
      /* set channel tag */
      ch_tag[CC_CHANNEL_CHAR] = (uint8_t)ch_tag[CC_CHANNEL_CHAR] + i;
      ret = get_tag_text(buf, ch_tag, tmp);
      if (!ret) {
        if (measurement_init(r)) {
          log_stderr(LOG_ERROR, "Failed to init measurement");
          goto error;
        }
        m = r->meas[r->count - 1];

        ret = get_tag_text(tmp, "watts", m->meas);
        if (!ret) {
          /* remove leading zeros */
          sprintf(m->meas, "%d", atoi(m->meas));
          log_stdout(LOG_DEBUG,
              "New power measurement: %s watts", m->meas);
        }
      }
    }

    r_p = &r;

  } else {
    log_stderr(LOG_ERROR, "Current cost message not found\n");
    free_reading(r);
    ret = SS_NO_MATCH;
  }

  return ret;

error:
  free_reading(r);
  return ret;
}

