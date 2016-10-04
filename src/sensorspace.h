#ifndef SENSORSPACE__H
#define SENSORSPACE__H
/******************************************************************************
 * File: sensorspace.h
 * Description: Main sensorspace header file
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

#define SS_SUCCESS                 0
#define SS_INIT_ERROR              1
#define SS_OUT_OF_MEM_ERROR        2
#define SS_SELECT_ERROR            3
#define SS_READ_ERROR              4
#define SS_WRITE_ERROR             5
#define SS_NO_SELECT_AVAIL         6
#define SS_READY                   7
#define SS_FILE_NOT_REG            8
#define SS_CFG_NO_MATCH            9
#define SS_CFG_NO_DEV              10
#define SS_CFG_NO_SENSOR           11
#define SS_CFG_NEW_TRAN            12
#define SS_CFG_NEW_DEV             13
#define SS_CFG_NEW_SENSOR          14
#define SS_CFG_FAILED              15
#define SS_NOT_AVAILABLE           16
#define SS_NO_MATCH                17
#define SS_BUF_FULL                18
#define SS_BUF_EMPTY               19
#define SS_POST_ERROR              20
#define SS_GET_ERROR               21
#define SS_GET_EMPTY               22
#define SS_CONN_ERROR              23
#define SS_CONN_RECONNECT          24
#define SS_INI_ERROR               25
#define SS_CFG_NO_DB               26
#define SS_READING_ERROR           27
#define SS_TTY_ERROR               28
#define SS_CONTINUE                99

/* needed? */
typedef enum {
  JSON,
  INI,
  RAW,
  DEV_CONFIG,
  SENSE_CONFIG,
  TRN_CFG,
  READING,
  READING_CALC,
  CALC,
} payload_t;

#endif                    /* SENSORSPACE__H */
