/******************************************************************************
 * File: mqtt_rrdtool.c
 * Description: An application that subscribes to MQTT topis and updates an
 *   RR database with the resultant reading.
 * Author: Steven Swann - swannonline@googlemail.com
 *
 * Copyright (c) swannonline, 2013-2014
 *
 * This file is part of sensorspace.
 *
 * sensorspace is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as sublished by
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
#include <stdbool.h>
#include <string.h>
#include <signal.h>

#include <getopt.h>

#include "uMQTT.h"
#include "uMQTT_helper.h"
#include "uMQTT_linux_client.h"

#include "sensorspace.h"
#include "reading.h"
#include "log.h"

#define MQTT_DEFAULT_TOPIC    "sensorspace/reading"

#define MAX_TOPIC_LEN 1024
#define MAX_NAME_LEN 128
#define MAX_MSG_LEN 2048

static int print_usage(void);

/*
 * \brief function to print help
 */
static int print_usage() {

  fprintf(stderr,
      "mqtt_rrdtool is an application that connects to an MQTT broker,\n"
      "subscribes to a number of topics and updates a round-robin databas\n"
      "with any readings received that match predefined sensor IDs\n"
      "Usage: mqtt_rrdtool [options];'\n"
      "General options:\n"
      " -h [--help]              : Displays this help and exits\n"
      "\n"
      "RRDtool options:\n"
      " -r [--rrd_file] <file>   : RRD file to update, can be used multiple\n"
      "                             times.\n"
      " -s [--sensor-id] <id>    : The sensor_id of the sensor to update.\n"
      " -n [--name] <name>       : The name of the sensor to update.\n"
      "\n"
      "Broker options:\n"
      " -b [--broker] <broker-IP>: Change the default broker IP\n"
      "                             - only IP addresses are\n"
      "                            currently supported. Default: localhost\n"
      " -p [--port] <port>       : Change the default port. Default: 1883\n"
      " -c [--clientid] <id>     : Change the default clientid\n"
      " -t [--topic] <topic>     : Topic, from which, the readings should\n"
      "                             arrive on. Can be used multiple times\n"
      "\n"
      "\nDebug options:\n"
      " -v [--verbose] <LEVEL>   : set verbose level to LEVEL\n"
      "                               Levels are:\n"
      "                                 SILENT\n"
      "                                 ERROR\n"
      "                                 WARN\n"
      "                                 INFO (default)\n"
      "                                 DEBUG\n"
      "\n");

  return 0;
}

int main(int argc, char **argv) {

  int ret;
  int i;
  int c, option_index = 0;
  char broker_ip[16] = MQTT_BROKER_IP;
  int broker_port = MQTT_BROKER_PORT;
  size_t len = MAX_MSG_LEN;
  char json[MAX_MSG_LEN] = "\0";
  char clientid[UMQTT_CLIENTID_MAX_LEN] = "\0";
  time_t t;

  struct reading *r = NULL;
  struct mqtt_packet *rx_pkt = NULL;

  /* Topic variables */
  char *topic[16] = { 0 };
  uint8_t topic_idx = 0;

  struct rrdtool rrd;
  memset(&rrd, 0, sizeof(struct rrdtool));

  topic[topic_idx] = calloc(MAX_TOPIC_LEN, sizeof(char));
  if (!topic[topic_idx]) {
    log_stderr(LOG_ERROR, "Failed to allocate memory for topics");
    return -1;
  }

  strncpy(topic[topic_idx], MQTT_DEFAULT_TOPIC, MAX_TOPIC_LEN);

  static struct option long_options[] =
  {
    /* These options set a flag. */
    {"help",   no_argument,             0, 'h'},
    {"verbose", required_argument,      0, 'v'},
    {"rrd_file", required_argument,     0, 'r'},
    {"sensor-id", required_argument,    0, 's'},
    {"name", required_argument,         0, 'n'},
    {"broker", required_argument,       0, 'b'},
    {"port", required_argument,         0, 'p'},
    {"clientid", required_argument,     0, 'c'},
    {"topic", required_argument,        0, 't'},
    {0, 0, 0, 0}
  };

  /* get arguments */
  while (1)
  {
    if ((c = getopt_long(argc, argv, "hv:s:n:t:r:b:p:c:", long_options,
            &option_index)) != -1) {

      switch (c) {
        case 'h':
          return print_usage();

        case 'v':
          /* set log level */
          if (optarg) {
            set_log_level_str(optarg);
          }
          break;

        case 'r':
          /* new rrd file */
          if (optarg) {
            if (rrd_file_init(&rrd, optarg)) {
              log_stderr(LOG_ERROR, "Failed to initialise rrd file");
              return -1;
            }
          } else {
            log_stderr(LOG_ERROR,
                "The RRD file flag should be followed by a file");
            return print_usage();
          }
          break;

        case 't':
          /* Set topic */
          if (optarg) {
            if (topic_idx) {
              topic[topic_idx] = calloc(MAX_TOPIC_LEN, sizeof(char));
              if (!topic[topic_idx]) {
                log_stderr(LOG_ERROR,
                    "Failed to allocate memory for topic");
                return -1;
              }
            }
            strcpy(topic[topic_idx++], optarg);
          } else {
            log_stderr(LOG_ERROR,
                "The topic flag should be followed by a topic");
            return print_usage();
          }
          break;

        case 's':
          /* set a sensor_id */
          if (optarg) {
            if (rrd.f_count) {
              rrd.sensor_id[rrd.f_count - 1] = atoi(optarg);
            }
          } else {
            log_stderr(LOG_ERROR,
                "The sensor_id flag should follow an rrd file flag, and"
                " should be followed by a sensor_id");
            return print_usage();
          }
          break;

        case 'n':
          /* set a name */
          if (optarg && rrd.f_count) {
            strncpy(rrd.name[rrd.f_count - 1], optarg, READ_NAME_LEN);
          } else {
            log_stderr(LOG_ERROR,
                "The sensor name flag should follow an rrd file flag, and"
                " should be followed by a sensor name");
            return print_usage();
          }
          break;

        case 'b':
          /* change the default broker ip */
          if (optarg) {
            strcpy(broker_ip, optarg);
          } else {
            log_stderr(LOG_ERROR,
                "The broker flag should be followed by an IP address");
            return print_usage();
          }
          break;

        case 'p':
          /* change the default port */
          if (optarg) {
            broker_port = *optarg;
          } else {
            log_stderr(LOG_ERROR,
                "The port flag should be followed by a port");
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

      }
    } else {
      /* Final arguement */
      break;
    }
  }

  struct broker_conn *conn;

  log_stdout(LOG_INFO, "Initialisig socket connection");

  init_linux_socket_connection(&conn, broker_ip, sizeof(broker_ip), broker_port);
  if (!conn) {
    log_stderr(LOG_ERROR, "Initialising socket connection");
    return -1;
  }

  if (clientid[0]) {
    broker_set_clientid(conn, clientid, sizeof(clientid));
  }

  log_stdout(LOG_INFO, "Connecting to broker");

  struct linux_broker_socket *skt = '\0';
  if ((ret = broker_connect(conn))) {
    log_stderr(LOG_ERROR, "Connecting to broker");
    free_connection(conn);
    return ret;
  } else {
    skt = (struct linux_broker_socket *)conn->context;
    log_stdout(LOG_INFO,
        "Connected to broker:\nip: %s port: %d", skt->ip, skt->port);
  }

  struct mqtt_packet *pkt = construct_packet_headers(SUBSCRIBE);


  log_stdout(LOG_INFO, "Subscribing to the following topic:");

  for (i = 0; i < topic_idx; i++) {
    log_stdout(LOG_INFO, "%s", topic[i]);
    /* Find length of topic and subscribe */
    const char *end = strchr(topic[i], '\0');
    if (!end || (ret = broker_subscribe(conn, (char *)topic[i],
            end - topic[i]))) {
      log_stderr(LOG_ERROR, "Subscribing to topic %s.", topic[i]);
      return ret;
    }
  }

  /* Start listening for packets */
  while (1) {

    /* packet border */
    log_stdout(LOG_INFO,
        "------------------------------------------------------------");

    ret = init_packet(&rx_pkt);
    if (ret) {
      log_stderr(LOG_ERROR, "Failed to initialie packet");
      break;
    }

    ret = reading_init(&r);
    if (ret) {
      log_stderr(LOG_ERROR, "Failed to initialie reading");
      break;
    }

    /* set reading date/time to now - fallback */
    t = time(0);
    localtime_r(&t, &r->t);

    ret = conn->receive_method(conn, rx_pkt);
    if (ret) {
      log_stderr(LOG_ERROR, "Receiving packet");
      break;
    }

    log_stdout(LOG_INFO, "Received packet - Attempting to convert to a reading");

    /* copy payload data - which should be JSON */
    /* Currently, we assume readings is json, but it could equally be ini? */
    strncpy(json, (const char *)&rx_pkt->payload->data, rx_pkt->pay_len);
    json[rx_pkt->pay_len + 1] = '\0';
    log_stderr(LOG_DEBUG, "PKT: %s", json);

    /* convert reading ready for rrd_update */
    ret = convert_json_reading(r, json, len);
    if (ret) {
      log_stderr(LOG_ERROR, "Converting reading from JSON");
      continue;
    }

    print_reading(r);

    ret = add_reading_rrd(r, &rrd);
    if (ret) {
      log_stderr(LOG_ERROR, "Failed to add reading to RR database");
    }

    free_reading(r);
    free_packet(rx_pkt);

  }

  log_stdout(LOG_INFO, "Disconnecting from broker");
  broker_disconnect(conn);
  free_reading(r);
  free_connection(conn);
  for (i = 0; i < topic_idx; i++) {
    free(topic[i]);
  }
  free_packet(pkt);
  free_packet(rx_pkt);
  free_rrd_files(&rrd);
  return ret;
}
