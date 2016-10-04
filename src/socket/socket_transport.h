#ifndef SOCKET_TRANSPORT__H
#define SOCKET_TRANSPORT__H
/******************************************************************************
 * File: socket_transports.h
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
#include <pthread.h>
#include <semaphore.h>

#define SOCKET_SS_CFG_HANDLE          (const char *)"TRANSPORT_SOCKET"

#define SOCKET_DEV_STRING_MAX      32

/**
 * \brief Struct to store socket connection config.
 * \param ip The ip address of the broker.
 * \param port The port with which to bind to.
 * \param sockfd The socket file descriptor of a connection instance.
 * \param serv_addr struct holding the address of the broker.
 * \param conn_state Current connection state.
 */
struct socket_transport {
  char ip[16];
  int port;
  struct sockaddr_in serv_addr;

  int sockfd;

  pthread_mutex_t socket_mutex;
  sem_t read_sem;

};

void socket_tran_config(void *ctx, const char *ip, uint8_t ip_len, int port);
int socket_tran_init(void **ctx_p);
int socket_tran_connect(void *ctx);
int socket_tran_open(void *ctx);
int socket_tran_read(void *ctx, trn_payload_t payload_t, void *pl_ctx,
    int *size);
void close_socket_tran(void *ctx);
void free_socket_tran(struct socket_transport *socket_tran);
void socket_free_ctx(void *ctx);
#endif    /* SOCKET_TRANSPORT__H */
