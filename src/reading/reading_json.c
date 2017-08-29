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
#include <stdlib.h>
#include <string.h>

#include "reading.h"

#define JSON_DELIMITER            ':'
#define JSON_BLOCK_CONTAINER      '{'
#define JSON_BLOCK_END_CONTAINER  '}'
#define JSON_ARRAY_CONTAINER      '['
#define JSON_ARRAY_END_CONTAINER  ']'
#define JSON_STR_CONTAINER        '\"'
#define JSON_DATE_KEY             "\"date\""
#define JSON_DEVICE_KEY           "\"device\""
#define JSON_ID_KEY               "\"id\""
#define JSON_NAME_KEY             "\"name\""
#define JSON_SENSORS_KEY          "\"sensors\""
#define JSON_MEAS_KEY             "\"meas\""
#define JSON_TYPE_KEY             "\"type\""
#define JSON_MIN_BUF_SIZE         sizeof("\"\":\"\"")

/**
 * \brief Crude conversion of reading to JSON (minified)
 */
int convert_reading_json(struct reading *r, char *buf, size_t *len) {

  int i;
  ssize_t l = 0;

  l = snprintf(buf, *len,
      "{\"date\":\"%02d-%02d-%02d %02d:%02d:%02d\",",
      r->t.tm_year + 1900, r->t.tm_mon + 1, r->t.tm_mday, r->t.tm_hour,
      r->t.tm_min, r->t.tm_sec);
  if (l < 0) goto error;

  /* Set device_id */
  if (r->device_id) {
    l += snprintf(buf + l, *len - l, "\"device\":{\"id\":\"%d\"},",
        r->device_id);
    if (l < 0) goto error;
  }

  /* Set measurements */
  l += snprintf(buf + l, *len - l, "\"sensors\":[");
  if (l < 0) goto error;

  if (r->count) {

    for (i = 0; i < r->count; i++) {
      /* add comma for multiple measurements */
      if (i > 0) {
        *(buf + l++) = ',';
      }

      /* open */
      l += sprintf(buf + l, "{");

      if (r->meas[i]->sensor_id) {
        /* assumes measurment always present, hence comma */
        l += snprintf(buf + l, *len - l, "\"id\":\"%d\",",
            r->meas[i]->sensor_id);
        if (l < 0) goto error;
      }

      if (r->meas[i]->name[0]) {
        l += snprintf(buf + l, *len - l, "\"name\":\"%s\"", r->meas[i]->name);
        if (l < 0) goto error;
      }

      if (r->meas[i]->meas) {
        /* assumes name always present, hence comma */
        l += snprintf(buf + l, *len - l, ",\"meas\":\"%s\"", r->meas[i]->meas);
        if (l < 0) goto error;
      }

      /* close */
      l += sprintf(buf + l, "}");

    }
  }

  /* finish and close */
  l += sprintf(buf + l, "]}");
  *len = l + 1;

  return SS_SUCCESS;

error:
  *len = 0;
  log_stderr(LOG_ERROR, "Failed to convert reading to JSON");
  return SS_WRITE_ERROR;
}

/**
 * \brief Function to get find the end of a container within a json buffer
 * \param start The start location
 * \param end The end location to be returned
 * \param open The container opening character
 * \param close The container closing character to search for
 */
static int get_json_end_container(const char *start, char **end_p,
    const char open, const char close) {
  int ret = SS_SUCCESS;
  char *end = (char *)start;
  unsigned indent = 1;

  log_stderr(LOG_DEBUG, "Finding container contents: %c*%c", open, close);

  /* find opening tag */
  while (*end != open && *end != '\0' ) end++;
  end++;

  /* Get end of array */
  while (*end != '\0') {

    if (*end == close) {
      /* nested array end */
      indent--;
    } else if (*end == open) {
      /* nested array start*/
      indent++;
    }

    log_stderr(LOG_DEBUG, "JSON current: %c, searching: %c Current indent %zd",
      *end, close, indent);

    if (indent == 0) {
      /* found close container */
      log_stderr(LOG_DEBUG, "End container found");
      break;

    } else {
      end++;
    }
  }

  if (end == '\0') {
    /* reached end of string - without finding end */
    log_stderr(LOG_ERROR, "JSON incomplete");
    ret = SS_GET_ERROR;
  }

  /* Ensure we capure end container too */
  *end_p = end + 1;

  return ret;
}

/**
 * \brief Function to find the "value" represented by a "key"
 * \param buf Buffer to hold the JSON string
 * \param key The key to search for
 * \param val The value to be returned
 */
int json_get_key_value(const char *buf, const char *key, char *val) {
  int ret = SS_SUCCESS;
  char *idx = NULL, *idx_end = NULL;
  size_t len = 0;

  log_stderr(LOG_DEBUG, "Searching for JSON key: %s", key);

  /* find key */
  idx = strstr(buf, key);
  if (!idx) {
    log_stderr(LOG_DEBUG, "JSON key: %s not present", key);
    return SS_GET_EMPTY;
  }

  log_stderr(LOG_DEBUG, "JSON key found: %s", key);

  /* find next none whitespace/return value */
  while (*idx == ' ' || *idx == '\r' || *idx == '\n') idx++;

  while (*idx != JSON_DELIMITER && *idx != '\0') {
    idx++;
  }

  if (*idx != JSON_DELIMITER) {
    log_stderr(LOG_ERROR, "JSON extraction failed: no delimiter");
    return SS_GET_ERROR;
  }

  /* first char after delimiter */
  idx++;

  /* find next none whitespace/return value */
  while (*idx == ' ' || *idx == '\r' || *idx == '\n') idx++;

  /* extract value */
  switch (*idx) {
    case JSON_STR_CONTAINER:
      ret = get_json_end_container(idx, &idx_end, JSON_STR_CONTAINER,
          JSON_STR_CONTAINER);
      break;

    case JSON_ARRAY_CONTAINER:
      ret = get_json_end_container(idx, &idx_end, JSON_ARRAY_CONTAINER,
          JSON_ARRAY_END_CONTAINER);
      break;

    case JSON_BLOCK_CONTAINER:
      ret = get_json_end_container(idx, &idx_end, JSON_BLOCK_CONTAINER,
          JSON_BLOCK_END_CONTAINER);
      break;

    default:
      ret = SS_GET_ERROR;
      break;
  }

  if (ret) {
    return ret;
  }

  /* Remove start and end tags */
  len = (size_t)(--idx_end - ++idx);
  log_stderr(LOG_DEBUG, "Attempting to copy %zu bytes", len);

  /* copy value string */
  if (!strncpy(val, idx, len)) {
    log_stderr(LOG_ERROR, "JSON extraction failed, could not copy value");
    ret = SS_GET_ERROR;
  }

  /* ensure terminated */
  val[len] = '\0';

  log_stderr(LOG_DEBUG, "JSON value extracted: %s", val);

  return ret;
}

/**
 * \brief Function to get a JSON array element
 * \param buf Buffer to hold the JSON array
 * \param i The array element's index to get
 * \param block The array block returned.
 */
static int get_array_block(const char *buf, unsigned i, char *block) {
  int ret = SS_SUCCESS;
  unsigned count = 0;
  char *idx = (char *)buf;
  char *idx_end = NULL;

  while (1) {
    ret = get_json_end_container(idx,
        (char **)&idx_end, JSON_BLOCK_CONTAINER,JSON_BLOCK_END_CONTAINER);
    if (ret || !idx) break;

    if (i == count) {

      if (*idx == ',') {
        idx++;
      }

      /* replace buffer */
      if (!strncpy(block, idx, (idx_end - idx))) {
        log_stderr(LOG_ERROR, "Failed to copy JSON block to buffer");
        ret = SS_GET_ERROR;
      }
      block[(idx_end - idx) + 1] = '\0';
      break;
    }
    count++;

    idx = idx_end;
    /* find next none whitespace/return value */
    while (*idx == ' ' || *idx == '\r' || *idx == '\n') idx++;

    if (*idx != ',') {
      log_stderr(LOG_ERROR, "JSON array index %d not available", i);
      ret = SS_GET_ERROR;
      break;
    }
  }

  return ret;
}

/**
 * \brief Returns the number of elements within a JSON array
 * \param buf Buffer to hold the JSON array
 */
static int get_array_block_count(const char *buf) {
  char *idx = (char *)buf;
  int i = 0;

  while (1) {
    int ret = get_json_end_container(buf + (idx - buf),
        (char **)&idx, JSON_BLOCK_CONTAINER,JSON_BLOCK_END_CONTAINER);
    if (ret || !idx) break;

    /* find next none whitespace/return value */
    while (*idx == ' ' || *idx == '\r' || *idx == '\n') idx++;

    if (*idx == ',') {
      i++;
    } else {
      break;
    }
  }

  return i;
}

/**
 * \brief Convert JSON into a reading struct
 * \param r Pointer to output reading struct
 * \param buf The JSON buffer
 * \param len The length of the JSON buffer
 */
int convert_json_reading(struct reading *r, char *buf, size_t len) {
  int ret = SS_SUCCESS;
  int i;
  int blk_cnt = 0;
  char val[1024], val1[1024], val2[1024];

  /* date conversion */
  ret = json_get_key_value(buf, JSON_DATE_KEY, val);
  if (ret) {
    /* set date to NOW */
  } else {
    convert_db_date_to_tm(val, &r->t);
  }

  /* device conversion */
  ret = json_get_key_value(buf, JSON_DEVICE_KEY, val);
  if (!ret) {

    /* we should have a device object */
    ret = json_get_key_value(val, JSON_ID_KEY, val1);
    if (!ret) {
      r->device_id = atoi(val1);
    }

    ret = json_get_key_value(val, JSON_NAME_KEY, r->name);
    if (!ret) {
      /* need proper error checking */
    }
  }

  /* measurement conversion */
  ret = json_get_key_value(buf, JSON_SENSORS_KEY, val);
  if (!ret) {

    blk_cnt = get_array_block_count(val);
    log_stderr(LOG_DEBUG, "JSON measurement count: %d\n", blk_cnt + 1);

    /* for each sensors array element */
    for (i = 0; i <= blk_cnt; i++) {

      /* get array element */
      ret = get_array_block(val, i, val1);
      if (ret) {
        log_stderr(LOG_ERROR, "Failed to copy JSON block to buffer");
      }
      /* how do we return from a single measurement - i.e - no array */

      log_stderr(LOG_DEBUG, "Processing array block: %s", val1);

      /* measurement */
      ret = measurement_init(r);
      if (ret) {
        break;
      }

      /* measurement id */
      ret = json_get_key_value(val1, JSON_MEAS_KEY, r->meas[r->count - 1]->meas);

      /* name */
      ret = json_get_key_value(val1, JSON_NAME_KEY, r->meas[r->count - 1]->name);

      /* sensor id */
      ret = json_get_key_value(val1, JSON_ID_KEY, val2);
      if (!ret) {
        r->meas[r->count - 1]->sensor_id = atoi(val2);
      }


      /* type */
      ret = json_get_key_value(val1, JSON_TYPE_KEY, val2);
      if (!ret) {
        /* ToDo: type conversion */
      }

    }
    log_stderr(LOG_DEBUG, "JSON conversion complete");
  }

  if (!ret || ret == SS_GET_EMPTY) {
    return SS_SUCCESS;
  } else {
    return ret;
  }
}
