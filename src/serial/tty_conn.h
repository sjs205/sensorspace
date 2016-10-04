#ifndef TTY_CONNECTION__H
#define TTY_CONNECTION__H
/******************************************************************************
 * File: tty_conn.h
 * Description: functions to provide tty connection functionality
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

#define TTY_DEV_STRING_MAX      32

struct tty_conn {
  char *path;
  int baud;

  int fd;

  struct termios *tty_ios;
  struct termios *tty_ios_old;
};

int tty_conn_check_config(struct tty_conn *tty);

int tty_conn_init(struct tty_conn **tty_p);
int tty_conn_open(struct tty_conn *tty);
int tty_conn_read(struct tty_conn *tty, char *buf, size_t *len);
void close_tty_conn(struct tty_conn *tty);
void free_tty_conn(struct tty_conn *tty);
#endif    /* TTY_CONNECTION__H */
