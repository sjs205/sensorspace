#ifndef READING_CC_DEV__H
#define READING_CC_DEV__H
/******************************************************************************
 * File: reading_cc_device.h
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
#define CC_DEV_MSG_STRING         "<msg><src>CC128-v"
#define CC_DEV_TEST_MSG           "<msg><src>CC128-v0.12</src>"           \
                                  "<dsb>00327</dsb><time>03:06:50</time>" \
                                  "<tmpr>24.9</tmpr><sensor>0</sensor>"   \
                                  "<id>00983</id><type>1</type>"          \
                                  "<ch1><watts>00822</watts></ch1></msg>"

#define CC_TEMP_TAG               "tmpr"
#define CC_CHANNEL_TAG            "ch1"
#define CC_CHANNEL_CHAR           2
#define CC_MAX_NO_CHANNELS        10


/* Function declariations here */
int process_cc_dev_input(struct reading **r_p, char *buf, size_t len);

#endif        /* READING_CC_DEV__H */
