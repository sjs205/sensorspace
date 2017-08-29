#ifndef READING__H
#define READING__H
/******************************************************************************
 * File: reading.h
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
#include <stdint.h>
#include <time.h>

#include "sensorspace.h"
#include "log.h"

#define READ_MEAS_LEN           32
#define READ_NAME_LEN           128
#define READ_MEAS_COUNT         64

typedef enum {
  READ_NONE,

  READ_RAW,
  READ_INI,
  READ_JSON,
} read_format_t;

/*
 * \brief Enum to hold measurement types supported by sensorspace
 */
typedef enum {
  MEAS_UNKNOWN,
  MEAS_TEMP,
  MEAS_CURRENT,
  MEAS_VOLTAGE,
  MEAS_POWER,
  MEAS_FLOW,
} meas_type_t;

/*
 * \brief Struct to hold reading instance
 * \param reading_id Reading identificaton assigned when inserted into DB
 * \param t The reading time
 * \param device_id The deviceId the reading is linked to
 * \param name The name of the device/reading
 * \param meas Measurements associated with reading
 * \param count Measurement count
 */
struct reading {
  uint32_t reading_id;
  struct tm t;

  uint32_t device_id;

  char name[READ_NAME_LEN];

  struct measurement *meas[READ_MEAS_COUNT];
  uint16_t count;
};

/*
 * \brief Struct to hold measurement instance
 * \param sensor_id The deviceId the reading is linked to
 * \param type The type of measurement
 * \param name The name of the sensor/measurement
 * \param meas The raw measurement value
 */
struct measurement {
  uint32_t sensor_id;
  meas_type_t type;
  char name[READ_NAME_LEN];
  char meas[READ_MEAS_LEN];
};

/* core library functions */
int reading_init(struct reading **r_p);
int measurement_init(struct reading *r);
void free_reading(struct reading *r);
void free_measurements(struct reading *r);

/* helper functions */
int print_reading(struct reading *r);
int validate_reading(struct reading *r);
int convert_tm_db_date(struct tm *date, char *buf);
int convert_db_date_to_tm(const char *time, struct tm *t);
int get_sensor_id_measurement(struct reading *r, uint32_t sensor_id,
    char *buf, size_t len);
int get_sensor_name_measurement(struct reading *r, char *name, char *buf,
    size_t len);

/* reading conversion functions */
int convert_ini_reading(struct reading *r, char *buf, size_t len);
int convert_json_reading(struct reading *r, char *buf, size_t len);
int convert_reading_json(struct reading *r, char *buf, size_t *len);
/* device specific reading conversion functions */
int convert_cc_dev_reading(struct reading *r, char *buf, size_t len);

/* helper functions */
int json_get_key_value(const char *buf, const char *key, char *val);

#endif        /* READING__H */
