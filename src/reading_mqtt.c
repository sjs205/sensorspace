/******************************************************************************
 * File: reading_mqtt.c
 * Description: An application that publishes sensor readings to a broker
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
#include "uMQTT_linux_client.h"

#include "sensorspace.h"
#include "reading.h"
#include "log.h"

#define MQTT_DEFAULT_TOPIC    "sensorspace/reading"

#define MAX_TOPIC_LEN 1024
#define MAX_MSG_LEN 2048

static int print_usage(void);

/*
 * \brief function to print help
 */
static int print_usage() {

  fprintf(stderr,
      "reading_mqtt is an application that connects to an MQTT broker and \n"
      "sends a reading in either raw, ini or json format as a publish packet\n"
      "before disconnecting\n"
      "Usage: reading_mqtt options] -d <id> -m <measurement> -s <id>;'\n"
      "General options:\n"
      " -h [--help]              : Displays this help and exits\n"
      "\n"
      "Reading options:\n"
      " -D [--date] <date>       : The reading date if not NOW.\n"
      "                            Format: '%%Y-%%m-%%d %%H:%%M:%%S'\n"
      " -d [--device-id] <id>    : The device_id the reading is attached to.\n"
      " -N [--device-name] <name>: The device_id the reading is attached to.\n"
      " -m [--meas] <measurement>: Measurement to sent as part of the reading.\n"
      "                            Can be used multiple times.\n"
      " -s [--sensor-id] <id>    : The sensor_id of the previous measurement.\n"
      " -n [--name] <name>       : The sensor/measurement name.\n"
      " -j [--json]              : Embed the reading in JSON format (DEFAULT)\n"
      " -i [--ini]               : Embed the reading in INI format\n"
      "                            NOTE: Not currently supported\n"
      "\n"
      "Publish options:\n"
      " -l [--location] <loc>    : Add a location topic. \n"
      "                            Can be used multiple times.\n"
      " -t [--topic] <topic>     : Change the default topic. \n"
      "                            Default: \n"
      "                            'sensorspace/reading/[location]/'\n"
      "                             [device-id]/[device-name]"
      " -r [--retain]            : Set the retain flag\n"
      "\n"
      "Broker options:\n"
      " -b [--broker] <broker-IP>: Change the default broker IP\n"
      "                             - only IP addresses are\n"
      "                            currently supported. Default: localhost\n"
      " -p [--port] <port>       : Change the default port. Default: 1883\n"
      " -c [--clientid] <id>     : Change the default clientid\n"
      "\n"
      "\nDebug options:\n"
      " -v [--verbose] <LEVEL>   : set verbose level to LEVEL\n"
      "                               Levels are:\n"
      "                                 SILENT\n"
      "                                 ERROR\n"
      "                                 WARN\n"
      "                                 INFO (default)\n"
      "                                 DEBUG\n"
      "                                 DEBUG_THREAD\n"
      "\n");

  return 0;
}

int main(int argc, char **argv) {

  int ret;
  int c, option_index = 0;
  char topic[MAX_TOPIC_LEN] = MQTT_DEFAULT_TOPIC;
  char broker_ip[16] = MQTT_BROKER_IP;
  int broker_port = MQTT_BROKER_PORT;
  size_t len = MAX_MSG_LEN;
  char msg[MAX_MSG_LEN] = "\0";
  char clientid[UMQTT_CLIENTID_MAX_LEN] = "\0";
  uint8_t retain = 0;
  uint32_t repeat = 1;
  bool topic_set = false;

  struct reading *r = NULL;
  ret = reading_init(&r);
  if (ret) {
    return -1;
  }

  /* set reading date to now */
  time_t t = time(0);
  localtime_r(&t, &r->t);

  static struct option long_options[] =
  {
    /* These options set a flag. */
    {"help",   no_argument,             0, 'h'},
    {"measure", required_argument,      0, 'm'},
    {"sensor-id", required_argument,    0, 's'},
    {"device-id", required_argument,    0, 'd'},
    {"device-name", required_argument,  0, 'N'},
    {"name", required_argument,         0, 'n'},
    {"date", required_argument,         0, 'D'},
    {"json", no_argument,               0, 'j'},
    {"ini", no_argument,                0, 'i'},
    {"retain",  no_argument,            0, 'r'},
    {"verbose", required_argument,      0, 'v'},
    {"location", required_argument,     0, 'l'},
    {"topic", required_argument,        0, 't'},
    {"broker", required_argument,       0, 'b'},
    {"port", required_argument,         0, 'p'},
    {"clientid", required_argument,     0, 'c'},
    {0, 0, 0, 0}
  };

  /* get arguments */
  while (1)
  {
    if ((c = getopt_long(argc, argv, "hv:s:n:N:d:D:jirt:l:m:b:p:c:", long_options,
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
          /* set retain flag */
          retain = 1;
          break;

        case 't':
          /* Set topic */
          if (optarg) {
            topic_set = true;
            strcpy(topic, optarg);
          } else {
            log_stderr(LOG_ERROR,
                "The topic flag should be followed by a topic");
            return print_usage();
          }
          break;

        case 'l':
          /* Set location */
          if (optarg) {
            if (topic_set) {
              log_stderr(LOG_ERROR,
                  "Location flag are invalid when the topic flag is used");
              return print_usage();
            }
            sprintf(topic, "%s/%s", topic, optarg);
          } else {
            log_stderr(LOG_ERROR,
                "The topic flag should be followed by a topic");
            return print_usage();
          }
          break;


        case 'D':
          /* set the reading date */
          if (optarg) {
            convert_db_date_to_tm(optarg, &r->t);
          } else {
            log_stderr(LOG_ERROR,
                "The date flag should be followed by a date");
            return print_usage();
          }
          break;

        case 'd':
          /* set a device_id */
          if (optarg) {
            r->device_id = atoi(optarg);
          } else {
            log_stderr(LOG_ERROR,
                "The device_id flag should be followed by a device_id");
            return print_usage();
          }
          break;

        case 'N':
          /* set device name */
          if (optarg) {
            strcpy(r->name, optarg);
          } else {
            log_stderr(LOG_ERROR,
                "The device name flag should be followed by a string");
            return print_usage();
          }
          break;


        case 'm':
          /* set a measurement */
          if (optarg) {

            ret = measurement_init(r);
            if (ret) {
              log_stderr(LOG_ERROR, "Failed to initialise measurement");
              return print_usage();
            }

            strcpy(r->meas[r->count - 1]->meas, optarg);

          } else {
            log_stderr(LOG_ERROR,
                "The measurement flag should be followed by a measurement");
            return print_usage();
          }
          break;

        case 'n':
          /* set sensor/measurement name */
          if (optarg) {
            strcpy(r->meas[r->count - 1]->name, optarg);
          } else {
            log_stderr(LOG_ERROR,
                "The name flag should be followed by a string");
            return print_usage();
          }
          break;

        case 's':
          /* set a sensor_id */
          if (optarg && r->count) {
            r->meas[r->count - 1]->sensor_id = atoi(optarg);
          } else {
            log_stderr(LOG_ERROR,
                "The sensor_id flag should follow a measurement flag, and"
                " should be followed by a sensor_id");
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
    log_stdout(LOG_ERROR, "Initialising socket connection");
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

  struct mqtt_packet *pkt = construct_packet_headers(PUBLISH);

  /* build topic string */
  if (!topic_set) {
    if (r->device_id) {
      sprintf(topic, "%s/%d", topic, r->device_id);
    }
    if (r->name) {
      sprintf(topic, "%s/%s", topic, r->name);
    }
  }

  if (!pkt || (ret = set_publish_variable_header(pkt, topic, strlen(topic)))) {
    log_stderr(LOG_ERROR, "Setting up packet");
    ret = UMQTT_ERROR;
    goto free;
  }

  if ((ret = set_publish_fixed_flags(pkt, retain, 0, 0))) {
    log_stderr(LOG_ERROR, "Setting publish flags");
    ret = UMQTT_ERROR;
    goto free;
  }

  /* convert reading ready for mqtt tx */
  convert_reading_json(r, msg, &len);

  log_stdout(LOG_INFO, "Constructed MQTT PUBLISH packet with:");
  log_stdout(LOG_INFO, "Topic: %s", topic);
  log_stdout(LOG_INFO, "Message: %s", msg);

  if ((ret = init_packet_payload(pkt, PUBLISH, (uint8_t *)msg, strlen(msg)))) {
    log_stderr(LOG_ERROR, "Attaching payload");
    ret = UMQTT_ERROR;
    goto free;
  }

  finalise_packet(pkt);

  log_stdout(LOG_INFO, "Sending packet to broker");

  /* Send packets */
  do {
    ret = broker_send_packet(conn, pkt);
    if (ret) {
      log_stderr(LOG_ERROR, "Sending packet failed");
    } else {
      log_stdout(LOG_INFO, "Successfully sent packet");
    }
  } while (--repeat);

free:
  log_stdout(LOG_INFO, "Disconnecting from broker");
  broker_disconnect(conn);
  free_reading(r);
  free_connection(conn);
  free_packet(pkt);
  return ret;
}
