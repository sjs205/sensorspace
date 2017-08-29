/******************************************************************************
 * File: actuator_mqtt.c
 * Description: Program to allow actuators to be controlled bia MQTT
 * Author: Steven Swann - swannonline@googlemail.com
 *
 * Copyright (c) swannonline, 2013-2014
 *
 * This file is part of uMQTT.
 *
 * uMQTT is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as sublished by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * uMQTT is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with uMQTT.  If not, see <http://www.gnu.org/licenses/>.
 *
 *****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <sys/select.h>

#include <getopt.h>

#include "uMQTT.h"
#include "uMQTT_helper.h"
#include "uMQTT_linux_client.h"

#include "sensorspace.h"
#include "reading/reading.h"
#include "log.h"

#include "actuator.h"

#define MAX_TOPIC_LEN         1024
#define MAX_MSG_LEN           1024
#define NOMSG_OFF_DELAY       300

#define DEFAULT_TOPIC         "sensorspace/reading/temp/"

static int print_usage(void);

/*
 * \brief function to print help
 */
static int print_usage() {

  fprintf(stderr,
      "actuator_mqtt connects actuators to to an MQTT broker and\n"
      "allows defined actuators to be controlled.\n"
      "\n"
      "Usage: actuator_mqtt [options]\n"
      "General options:\n"
      " -h [--help]              : Displays this help and exits\n"
      "\n"
      "Control options:\n"
      " -P [---pin] <pin>        : The pin number the actuator is conected\n"
      "                            to. Must be specfied, can be used multiple\n"
      "                            times.\n"
      " -n [--name] <name>       : The human readable name of the actuator\n"
      " -N [--pv-name] <name>    : The controller variable responsible for\n"
      "                          : for the actuator control commands.\n"
      " -t [--pv-topic] <topic>  : The topic containing the actuator control\n"
      " -D [--duty-cycle] <cyc>  : The duty cycle to apply to the actuator \n"
      "                            when on.\n"
      " -m [--max-on] <max-on>   : The maximum on time for the actuator\n"
      " -i [--invert]            : Invert the actuator pin\n"
      " -S [--init-state] <bool> : Initial state of the actuator\n"
      " -T [--timeout] <sec>     : Max time between message before actuator\n"
      "                            changes state. Default: 360 seconds\n"
      "\n"
      "Broker options:\n"
      " -b [--broker] <broker-IP>: Change the default broker IP - only IP\n"
      "                            addresses are currently supported.\n"
      "                            Default ip: localhost\n"
      " -p [--port] <port>       : Change the default port. Default: 1883\n"
      " -c [--clientid] <id>     : Change the default clientid\n"
      "\n"
      "Output options:\n"
      " -d [--detail]            : Output detailed packet information\n"
      "                            Default: output publish packet summary\n"
      "\n"
      "Debug options:\n"
      " -v [--verbose] <LEVEL>   : set verbose level to LEVEL\n"
      "                               Levels are:\n"
      "                                 SILENT\n"
      "                                 ERROR\n"
      "                                 WARN\n"
      "                                 INFO (default)\n"
      "                                 DEBUG\n"
      "                                 DEBUG_FN\n"
      "\n");

  return 0;
}

int main(int argc, char **argv) {

  int ret;
  int c, option_index = 0, i;
  uint8_t detail = 0;
  char broker_ip[16] = MQTT_BROKER_IP;
  struct broker_conn *conn;
  int broker_port = MQTT_BROKER_PORT;
  char clientid[UMQTT_CLIENTID_MAX_LEN] = "\0";
  struct mqtt_packet *pkt = NULL;

  /* initialise new reading */
  struct reading *r = NULL;

  /* create actuators */
  struct actuator a[10];
  uint8_t a_index = 0;
  memset(&a, 0, sizeof(a));
  int timeout = 360;

  /* select variables */
  fd_set read_fds;
  int nfds = 0;
  struct timeval s_timeout;


  static struct option long_options[] =
  {
    /* These options set a flag. */
    {"help",   no_argument,             0, 'h'},
    {"verbose", required_argument,      0, 'v'},
    {"pin-number", required_argument,   0, 'P'},
    {"name", required_argument,         0, 'n'},
    {"duty", required_argument,         0, 'D'},
    {"invert", no_argument,             0, 'i'},
    {"max-on", required_argument,       0, 'm'},
    {"timeout", required_argument,      0, 'T'},
    {"detail", no_argument,             0, 'd'},
    {"pv-topic", required_argument,     0, 't'},
    {"pv-name", required_argument,      0, 'N'},
    {"init-state", required_argument,   0, 'S'},
    {"broker", required_argument,       0, 'b'},
    {"port", required_argument,         0, 'p'},
    {"clientid", required_argument,     0, 'c'},
    {0, 0, 0, 0}
  };

  /* get arguments */
  while (1)
  {
    if ((c = getopt_long(argc, argv, "hdv:P:t:T:N:S:b:p:c:m:iD:n:P:", long_options,
            &option_index)) != -1) {

      switch (c) {
        case 'h':
          return print_usage();
          break;

        case 'v':
          /* set log level */
          if (optarg) {
            set_log_level_str(optarg);
          }
          break;

        case 'd':
          /* set detailed output */
          detail = 1;
          break;

        case 't':
          /* Set topic */
          if (optarg) {
            strcpy(a[a_index].pv_topic, optarg);
          } else {
            log_stderr(LOG_ERROR,
                "The pv-topic flag should be followed by a topic.");
            return print_usage();
          }
          break;

        case 'n':
          /* Set name */
          if (optarg) {
            strcpy(a[a_index].name, optarg);
          } else {
            log_stderr(LOG_ERROR,
                "The name flag should be followed by a human readable name.");
            return print_usage();
          }
          break;

        case 'N':
          /* Set pv-name */
          if (optarg) {
            strcpy(a[a_index].pv_name, optarg);
          } else {
            log_stderr(LOG_ERROR,
                "The pv-name flag should be followed by a name.");
            return print_usage();
          }
          break;

        case 'S':
          /* Set initial state */
          if (optarg) {
            if (atoi(optarg)) {
              log_stdout(LOG_INFO, "Setting initial state to on");
              a[a_index].state = true;
            } else {
              log_stdout(LOG_INFO, "Setting initial state to off");
              a[a_index].state = false;
            }
          } else {
            log_stderr(LOG_ERROR,
                "The initial state flag should be followed by a binary state.");
            return print_usage();
          }
          break;

        case 'b':
          /* change the default broker ip */
          if (optarg) {
            strcpy(broker_ip, optarg);
          } else {
            log_stderr(LOG_ERROR,
                "The broker flag should be followed by an IP address.");
            return print_usage();
          }
          break;

        case 'p':
          /* change the default port */
          if (optarg) {
            broker_port = *optarg;
          } else {
            log_stderr(LOG_ERROR,
                "The port flag should be followed by a port.");
            return print_usage();
          }
          break;

        case 'c':
          /* Set clientid */
          if (optarg) {
            strcpy(clientid, optarg);
          } else {
            log_stderr(LOG_ERROR,
                "The clientid flag should be followed by a clientid");
            return print_usage();
          }
          break;

        case 'P':
          /* Set the pin number the actuator is connected to */
          if (optarg) {
            if (a[a_index].pin) {
              a_index++;
            }
            a[a_index].pin = atoi(optarg);
          } else {
            log_stderr(LOG_ERROR,
                "The pin flag should be followed by an integer");
            return print_usage();
          }
          break;

        case 'm':
          /* Set the maximum amount of time the actuator can remain on */
          if (optarg) {
            a[a_index].max_on = atoi(optarg);
          } else {
            log_stderr(LOG_ERROR,
                "The max-on flag should be followed by an integer");
            return print_usage();
          }
          break;

        case 'D':
          /* Set the maximum amount of time the actuator can remain on */
          if (optarg) {
            a[a_index].duty = atoi(optarg);
          } else {
            log_stderr(LOG_ERROR,
                "The duty cycle flag should be followed by an integer");
            return print_usage();
          }
          break;

        case 'i':
          /* Invert the actuator pin */
          a[a_index].invert = true;
          break;

        case 'T':
          /* Set timeout */
          if (optarg) {
            timeout = atoi(optarg);
          } else {
            log_stderr(LOG_ERROR,
                "The timeout flag should be followed by a timeout, in secs.");
            return print_usage();
          }
          break;

      }
    } else {
      break;
    }
  }

  /* socket initialisation */
  log_stderr(LOG_INFO, "Initialising socket connection");

  init_linux_socket_connection(&conn, broker_ip, sizeof(broker_ip), broker_port);
  if (!conn) {
    log_stderr(LOG_ERROR, "Initialising socket connection");
    return -1;
  }

  if (clientid[0]) {
    broker_set_clientid(conn, clientid, sizeof(clientid));
  }

  log_stderr(LOG_INFO, "Connecting to broker");

  struct linux_broker_socket *skt = '\0';
  if ((ret = broker_connect(conn))) {
    log_stderr(LOG_ERROR, "Connecting to broker");
    free_connection(conn);
    return ret;
  } else {
    skt = (struct linux_broker_socket *)conn->context;
    log_stderr(LOG_INFO,
        "Connected to broker:\nip: %s port: %d", skt->ip, skt->port);
  }

  /* Actuator initialisation */
  for (i = 0; i <= a_index; i++) {
    if (!a[i].pin) {
      break;
    }
    ret = init_actuator(&a[i], a[i].state);
    if (ret) {
      log_stderr(LOG_ERROR, "Failed to initialise actuators");
      return ret;
    }

    /* subscribe to actuator packets */
    log_stderr(LOG_INFO, "Subscribing to the following topics:");

    if (a[i].pv_topic[0]) {
      /* Find length of topic and subscribe */
      const char *end = strchr(a[i].pv_topic, '\0');
      log_stderr(LOG_INFO, "Topic: %s", a[i].pv_topic);
      if (!end || (ret = broker_subscribe(conn, a[i].pv_topic,
              end - a[i].pv_topic))) {
        log_stderr(LOG_ERROR, "Subscribing to topic.");
        return UMQTT_ERROR;
      }
    }
  }

  /* Start listening for packets */
  while (1) {

    if (init_packet(&pkt)) {
      log_stderr(LOG_ERROR, "Initialising packet");
      ret = UMQTT_ERROR;
      goto free;
    }

    /* select init */
    FD_ZERO(&read_fds);
    nfds = skt->sockfd + 1;
    FD_SET(skt->sockfd, &read_fds);

    s_timeout.tv_usec = 250;
    s_timeout.tv_sec = 0;
    ret = select(nfds, &read_fds, NULL, NULL, &s_timeout);
    if (ret == -1) {
      if (errno == EINTR) {
        log_stderr(LOG_ERROR, "Select: %s", strerror(errno));
        goto next;
      }
      log_stderr(LOG_ERROR, "Select failed: %s", strerror(errno));
      goto next;

    } else if (!ret) {
      /* should never get here since disabled timeout */
      log_stdout(LOG_DEBUG, "select timed out");
    }

    /* Determine if any fds are ready */
    if (FD_ISSET(skt->sockfd, &read_fds)) {
      /* process MQTT input */

      ret = conn->receive_method(conn, pkt);
      if (ret) {
        log_stderr(LOG_ERROR, "failed to process packet input");
        goto next;
      } else {
        if (detail) {
          print_packet_detailed_info(pkt);
        } else {
          print_publish_packet_info(pkt);
        }
      }

      /* process incoming packet */
      ret = reading_init(&r);
      if (ret) {
        return -1;
      }

      ret = convert_json_reading(r, (char *)pkt->payload, pkt->pay_len);
      if (ret) {
        log_stderr(LOG_ERROR, "Failed to convert payload to reading.");
      }

      for (i = 0; i <= a_index; i++) {
        if (!a[i].pin) {
          break;
        }

        /* ToDo: We should be checking the packet topic here */
        char state[64];
        if (!get_sensor_name_measurement(r, a[i].pv_name, state, 64)) {
          log_stdout(LOG_INFO, "Updating state of %s actuator", a[i].name);
          if (!strcmp(state, "true")) {
            a[i].state = true;
          } else if (!strcmp(state, "false")) {
            a[i].state = false;
          } else {
            a[i].state = (bool)atoi(state);
          }
          set_actuator_state(&a[i]);
        }
      }
    }

    /* Disable actuators that have exceeded their timeout */
    for (i = 0; i <= a_index; i++) {
      if (!a[i].pin) {
        break;
      }
      if (a[i].last_set + timeout <= time(NULL)) {
        log_stderr(LOG_ERROR, "Disabling %s since msg timeout exceeded",
            a[i].name);
        a[i].state = 0;
        set_actuator_state(&a[i]);
      }
    }

next:
    /* cleanup after reading */
    free_reading(r);
    r = NULL;
    free_packet(pkt);
    pkt = NULL;
  }

free:
broker_disconnect(conn);
  free_connection(conn);
  free_packet(pkt);
  free_reading(r);
  return ret;
}
