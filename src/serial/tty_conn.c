/******************************************************************************
 * File: tty_conn.c
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#include "sensorspace.h"
#include "log.h"
#include "tty_conn.h"

/**
 * \brief Check to ensure minimal configuration settings are in place.
 * \param tty The tty connection to check
 */
int tty_conn_check_config(struct tty_conn *tty) {

  if (access(tty->path, F_OK)) {
    log_stderr(LOG_ERROR, "tty connection device access: %s\n\t%s\n",
        tty->path, strerror(errno));
    return SS_CFG_FAILED;
  }

  return SS_SUCCESS;
}

/**
 * \brief Initialise the tty_conn struct
 */
int tty_conn_init(struct tty_conn **tty_p)
{
  struct tty_conn *tty;

  if (!(tty = calloc(1, sizeof(struct tty_conn)))) {
    goto free;
  }

  if (!(tty->path = calloc(TTY_DEV_STRING_MAX, sizeof(char)))) {
    log_stderr(LOG_ERROR, "tty connection: Out of memory");
    goto free;
  }

  if (!(tty->tty_ios = calloc(1, sizeof(struct termios)))) {
    log_stderr(LOG_ERROR, "tty connection: Out of memory");
    goto free;
  }

  if (!(tty->tty_ios_old = calloc(1, sizeof(struct termios)))) {
    log_stderr(LOG_ERROR, "tty connection: Out of memory");
    goto free;
  }

  *tty_p = (void *)tty;
  return SS_SUCCESS;

free:
  free_tty_conn(tty);
  return SS_OUT_OF_MEM_ERROR;
}

/**
 * \brief Open a tty connection
 * \param tty The tty connection to open
 */
int tty_conn_open(struct tty_conn *tty) {

  log_stdout(LOG_INFO, "Attempting to open device: %s", tty->path);
  /*
     Open modem device for reading and writing and not as controlling tty
     because we don't want to get killed if linenoise sends CTRL-C.
     */
  tty->fd = open(tty->path, O_RDWR | O_NOCTTY);// | O_NONBLOCK);
  if (tty->fd < 0) {
    log_stderr(LOG_ERROR, "Open failed: %s: %s", tty->path, strerror(errno));
    goto error;
  }

  /* save current serial port settings */
  tcgetattr(tty->fd, tty->tty_ios_old);

  /* Configure device */
  int baud;
  switch(tty->baud) {
    case B0: baud = B0; break;
    case B50: baud = B50; break;
    case B75: baud = B75; break;
    case B110: baud = B110; break;
    case B134: baud = B134; break;
    case B150: baud = B150; break;
    case B200: baud = B200; break;
    case B300: baud = B300; break;
    case B600: baud = B600; break;
    case B1200: baud = B1200; break;
    case B1800: baud = B1800; break;
    case B2400: baud = B2400; break;
    case B4800: baud = B4800; break;
    case B9600: baud = B9600; break;
    case B19200: baud = B19200; break;
    case B38400: baud = B38400; break;
    case B57600: baud = B57600; break;
    case B115200: baud = B115200; break;
    case B230400: baud = B230400; break;
    default: baud = B57600;
  }

  tty->tty_ios->c_cflag = baud | CS8 | CLOCAL | CREAD;
  /* ignore bytes with parity error, map CR to NL */
  tty->tty_ios->c_iflag = ICRNL;
  /* Raw output */
  tty->tty_ios->c_oflag = 0;
  /* Disable all echo */
  tty->tty_ios->c_lflag = ICANON;

  if (tcflush(tty->fd, TCIFLUSH)) {
    log_stderr(LOG_ERROR, "%s: %s", tty->path, strerror(errno));
    goto error;
  }

  /* Activate new settings */
  if (tcsetattr(tty->fd, TCSANOW, tty->tty_ios)) {
    log_stderr(LOG_ERROR, "%s: %s", tty->path, strerror(errno));
    goto error;
  }

  log_stdout(LOG_INFO, "Device open, success");

  return SS_SUCCESS;

error:
  return SS_INIT_ERROR;
}

/**
 * \brief read() tty connection - assumes data is available, else may block
 * \param tty The tty connection to attempt to read
 * \param buf pointer to a multiple character buffer
 * \param len pointer to the size of the buffer, if successful, size is
 *        updated with the number of bytes read
 */
int tty_conn_read(struct tty_conn *tty, char *buf, size_t *len) {
  int ret = SS_SUCCESS;

  *len = read(tty->fd, buf, *len);
  if ((ssize_t)*len == -1) {// || buf[0] == '\n') {
    if (errno == EINTR || errno == EAGAIN) {
      ret = SS_CONTINUE;
    } else {
      log_stderr(LOG_ERROR, "Read error: %d:%s", errno, strerror(errno));
      ret = SS_READ_ERROR;
    }

  } else {
    buf[*len] = '\0';
    log_stdout(LOG_DEBUG, "read returned %d bytes:", *len);
    if (*len > 1 && buf[0] != '\n') {
      log_stdout(LOG_DEBUG, "%s", buf);
    }
  }

  return ret;
}

void close_tty_conn(struct tty_conn *tty) {
  if (tty) {
    log_stdout(LOG_INFO, "Closing tty device: %s", tty->path);

    if (tty->fd) {
      /* Restore old settings */
      tcflush(tty->fd, TCIFLUSH);
      tcsetattr(tty->fd, TCSANOW, tty->tty_ios_old);
      close(tty->fd);
      tty->fd = 0;
    }
  }
  return;
}

void free_tty_conn(struct tty_conn *tty)
{
  log_stdout(LOG_DEBUG, "Freeing tty connection: %s", tty->path);
  free(tty->path);
  free(tty->tty_ios);
  free(tty->tty_ios_old);
  free(tty);
  return;
}
