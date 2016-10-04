/******************************************************************************
 * File: socket_transports.c
 * Description: functions to provide socket transport functionality
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
#include <errno.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "log.h"
#include "sensorspace.h"
#include "uMQTT.h"
#include "uMQTT_client.h"
#include "socket_transport.h"
    
int socket_tran_init(void **ctx_p) 
{

  struct socket_transport *socket_tran;

  if (!(socket_tran = calloc(1, sizeof(struct socket_transport)))) {
    goto free;
  }

  /* init semaphore */
  sem_init(&socket_tran->read_sem, 0, 0);

  /* statically init mutex */
  socket_tran->socket_mutex = (pthread_mutex_t)PTHREAD_MUTEX_INITIALIZER;

  *ctx_p = (void *)socket_tran;
  return SS_SUCCESS;

free:
  free_socket_tran(socket_tran);
  return SS_OUT_OF_MEM_ERROR;
}

void socket_tran_config(void *ctx, const char *ip, uint8_t ip_len, int port) {
    struct socket_transport *skt_tran = (struct socket_transport *)ctx;

  log_stdout(LOG_INFO, "Configuring socket connection to: %s:%d", ip, port);

  skt_tran->serv_addr.sin_family = AF_INET;
  skt_tran->port = port;
  skt_tran->serv_addr.sin_port = htons(skt_tran->port);
  memcpy(skt_tran->ip, ip, ip_len);

  return;
}

int socket_tran_connect(void *ctx) {
    struct socket_transport *skt_tran = (struct socket_transport *)ctx;

  /* start of CRITICAL SECTION */
  while (pthread_mutex_lock(&skt_tran->socket_mutex)) {
    log_stdout(LOG_DEBUG_THREAD, "Socket thread Block");
  }

  log_stdout(LOG_INFO, "Attempting to connect to: %s", skt_tran->ip);

  
  if ((skt_tran->sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
  {
    log_stdout(LOG_ERROR, "Error: Could not create socket\n");
    goto error;
  }

  /* convert ip address to binary */
  if (inet_pton(skt_tran->serv_addr.sin_family, skt_tran->ip,
        &skt_tran->serv_addr.sin_addr) <= 0)
  {
    log_stdout(LOG_DEBUG, "ERROR: inet_pton error occured\n");
    goto error;
  }

  if (connect(skt_tran->sockfd, (struct sockaddr *)&skt_tran->serv_addr,
        sizeof(skt_tran->serv_addr)) < 0)
  {
    log_stdout(LOG_DEBUG,"Error: Connect Failed\n");
    goto error;
  }

  log_stdout(LOG_INFO, "Socket connected, success");

  /* end of CRITICAL SECTION */
  pthread_mutex_unlock(&skt_tran->socket_mutex);
  return SS_SUCCESS;
  
  return UMQTT_SUCCESS;
error:
  /* end of CRITICAL SECTION */
  pthread_mutex_unlock(&skt_tran->socket_mutex);
  return SS_INIT_ERROR;
}

/**
 * \brief read() socket transport - assumes data is available, else may block
 * \param pl_ctx pointer to a multiple character buffer
 * \param size pointer to the size of the buffer, if successful, size is
 *        updated with the number of bytes read
 */
int socket_tran_read(void *ctx, trn_payload_t payload_t, void *pl_ctx,
    int *size) {
  char *buf = pl_ctx;

  int ret = SS_SUCCESS;

  struct socket_transport *tran = (struct socket_transport *)ctx;

  /* start of CRITICAL SECTION */

  log_stdout(LOG_DEBUG_THREAD, "socket_mutex locking");
  while (pthread_mutex_lock(&tran->socket_mutex)) {
    log_stdout(LOG_DEBUG_THREAD, "SOCKET thread Block");
  }

  log_stdout(LOG_DEBUG_THREAD, "socket_mutex locked");


  *size = read(tran->sockfd, buf, *size);
  if (*size == -1 || buf[0] == '\n') {
    if (errno == EINTR) {
      ret = SS_CONTINUE;
    } else {
      ret = SS_READ_ERROR;
    }
  } else {
    buf[*size] = '\0';
    log_stdout(LOG_DEBUG, "read returned %d bytes:\n%s", *size, buf);
  }

  /* end of CRITICAL SECTION */
  log_stdout(LOG_DEBUG_THREAD, "socket_mutex unlocking");
  pthread_mutex_unlock(&tran->socket_mutex);

  log_stdout(LOG_DEBUG_THREAD, "socket_mutex unlocked");

  return ret;
}

void  close_socket_tran(void *ctx) {
  struct socket_transport *socket_tran = (struct socket_transport *)ctx;
  if (socket_tran) {
    log_stdout(LOG_INFO, "Closing socket device: %s", socket_tran->ip);

    if (socket_tran->sockfd) {
      /* Restore old settings */
      close(socket_tran->sockfd);
      socket_tran->sockfd = 0;
    }
  }
  return;
}

void  free_socket_tran(struct socket_transport *socket_tran) 
{
  free(socket_tran);
  return;
}

void socket_free_ctx(void *ctx) {
  free_socket_tran((struct socket_transport *)ctx);
  return;
}
