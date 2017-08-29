/******************************************************************************
 * File: ds18b20_mqtt.c
 * Description: A program that reads data from DB18B20 temperature
 *              sensors and sends this to an MQTT broker
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
#include <dirent.h>
#include <unistd.h>

#include <sys/stat.h>
#include <fcntl.h>

#include <getopt.h>

#include "uMQTT.h"
#include "uMQTT_linux_client.h"

#include "sensorspace.h"
#include "reading.h"
#include "log.h"

#define MQTT_DEFAULT_TOPIC    "sensorspace/reading/temp"

#define MAX_TOPIC_LEN 1024
#define MAX_MSG_LEN 2048

#define DS18B20_ID          "28-"
#define MAX_NO_DS18B20      16
#define DS18B20_ID_LEN      16

#define W1_DEVICE_DIR       "/sys/bus/w1/devices/"
#define W1_TEMP_DELIM       "t="
#define W1_READING_FILE     "w1_slave"
#define W1_CRC_FAIL_STR     "NO"
#define W1_CRC_SUCCESS_STR  "YES"

struct ds18b20_measurement {
  char sensor[DS18B20_ID_LEN];
  struct measurement meas;
};

int ds18b20_reading(struct reading *r, struct ds18b20_measurement *ds_meas);

static int print_usage(void);

/*
 * \brief function to print help
 */
static int print_usage() {

  fprintf(stderr,
      "ds18b20_mqtt is an application that connects to an MQTT broker and \n"
      "sends a temperature reading in either raw, ini or json format as\n"
      "a publish packet\n"
      "before disconnecting\n"
      "Usage: reading_mqtt options] -d <id> -m <measurement> -s <id>;'\n"
      "General options:\n"
      " -h [--help]              : Displays this help and exits\n"
      "\n"
      "Reading options:\n"
      " -d [--device-id] <id>    : The device_id the reading is attached to.\n"
      " -N [--device-name] <name>: The name of the device  the reading is\n"
      "                            attached to.\n"
      " -I [--id] <id>           : The DS13B20 onewire sensors to read, can\n"
      "                          : be used multiple times.\n"
      " -s [--sensor-id] <id>    : Set the sensor_id of the previously\n"
      "                          : defined sensor.\n"
      " -n [--name] <name>       : Set the sensor`name of the previously\n"
      "                          : defined sensor. Default is the\n"
      "                          : onewire sensor ID.\n"
      " -j [--json]              : Embed the reading in JSON format (DEFAULT)\n"
      " -i [--ini]               : Embed the reading in INI format\n"
      "                            NOTE: Not currently supported\n"
      " -S [--seconds] <S>       : Number of secsonds between readings\n"
      "                            Default is perform a single reading then\n"
      "                            exit.\n"
      "\n"
      "Publish options:\n"
      " -l [--location] <loc>    : Add a location topic. \n"
      "                            Can be used multiple times.\n"
      " -t [--topic] <topic>     : Change the default topic. \n"
      "                            Default: \n"
      "                            'sensorspace/reading/[location]/'\n"
      "                             [device-id]/[device-name]\n"
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

int ds18b20_reading(struct reading *r, struct ds18b20_measurement *ds_meas) {

  /* for each file in directory */
  int ret = SS_SUCCESS;
  DIR *d;
  struct dirent *dir;
  char filename[1024] = "\0";
  char buf[128] = "\0";
  char *temp_str = NULL;

  d = opendir(W1_DEVICE_DIR);
  if (d)
  {
    while ((dir = readdir(d)) != NULL)
    {
      if (strstr(dir->d_name, DS18B20_ID)) {

        /* Are we looking for a specific device? */
        if (ds_meas && ds_meas->sensor &&
            strcmp(dir->d_name, ds_meas->sensor)) {
          /* no match */
          continue;
        }

        log_stdout(LOG_INFO, "DS18B20 Found: %s", dir->d_name);

        /* Process sensor data */
        sprintf(filename, "%s/%s/%s", W1_DEVICE_DIR, dir->d_name,
            W1_READING_FILE);
        FILE *file = fopen(filename, "r");

        fread(buf, sizeof(uint8_t), sizeof(buf), file);

        if (strstr(buf, W1_CRC_SUCCESS_STR) &&
            strstr(buf, W1_TEMP_DELIM)) {
          temp_str = strstr(buf, W1_TEMP_DELIM) + sizeof(W1_TEMP_DELIM) - 1;

          /* new measurement */
          ret = measurement_init(r);
          if (ret) {
            log_stderr(LOG_ERROR, "Failed to initialise measurement");
            return ret;
          }

          sprintf(r->meas[r->count - 1]->meas, "%3.3f",
              atof(temp_str) / 1000);

          if (ds_meas && ds_meas->meas.name[0]) {
            sprintf(r->meas[r->count - 1]->name, "%s", ds_meas->meas.name);
          } else {
            sprintf(r->meas[r->count - 1]->name, "%s", dir->d_name);
          }

          if (ds_meas && ds_meas->meas.sensor_id) {
            r->meas[r->count - 1]->sensor_id = ds_meas->meas.sensor_id;
          }
        } else {
          log_stderr(LOG_ERROR, "Failed to retrieve temp data");
        }
      }

    }
    closedir(d);
  }
  return ret;
}

int main(int argc, char **argv) {

  int ret;
  int i;
  int c, option_index = 0;
  char topic[MAX_TOPIC_LEN] = MQTT_DEFAULT_TOPIC;
  char broker_ip[16] = MQTT_BROKER_IP;
  int broker_port = MQTT_BROKER_PORT;
  size_t len = MAX_MSG_LEN;
  char msg[MAX_MSG_LEN] = "\0";
  char clientid[UMQTT_CLIENTID_MAX_LEN] = "\0";
  uint8_t retain = 0;
  uint32_t secs = 0;
  bool topic_set = false;
  bool all_devices = true;
  time_t t;

  struct reading *r = NULL;
  ret = reading_init(&r);
  if (ret) {
    return -1;
  }

  struct ds18b20_measurement ds_set[MAX_NO_DS18B20];
  bzero(&ds_set, sizeof(ds_set));
  uint8_t ds_cnt = 0;

  static struct option long_options[] =
  {
    /* These options set a flag. */
    {"help",   no_argument,             0, 'h'},
    {"device-id", required_argument,    0, 'd'},
    {"sensor-id", required_argument,    0, 's'},
    {"id", required_argument,           0, 'I'},
    {"device-name", required_argument,  0, 'N'},
    {"name", required_argument,         0, 'n'},
    {"json", no_argument,               0, 'j'},
    {"ini", no_argument,                0, 'i'},
    {"retain",  no_argument,            0, 'r'},
    {"verbose", required_argument,      0, 'v'},
    {"location", required_argument,     0, 'l'},
    {"topic", required_argument,        0, 't'},
    {"broker", required_argument,       0, 'b'},
    {"port", required_argument,         0, 'p'},
    {"clientid", required_argument,     0, 'c'},
    {"seconds", required_argument,      0, 'S'},
    {0, 0, 0, 0}
  };

  /* get arguments */
  while (1)
  {
    if ((c = getopt_long(argc, argv, "hv:s:I:n:N:d:jirt:l:b:p:c:S:",
            long_options, &option_index)) != -1) {

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

        case 'I':
          /* set a ds18b20 to look for */
          all_devices = false;
          if (optarg) {
            /* only increment if sensor is set, i.e. only after second */
            if (*ds_set[ds_cnt].sensor) {
              ds_cnt++;
            }
            strcpy(ds_set[ds_cnt].sensor, optarg);
          } else {
            log_stderr(LOG_ERROR,
                "The id flag should be followed by a DS18B20 ID");
            return print_usage();
          }
          break;

        case 's':
          /* set sensor_id of previously defined sensor */
          if (optarg && ds_set[ds_cnt].sensor) {
            ds_set[ds_cnt].meas.sensor_id = atoi(optarg);
          } else {
            log_stderr(LOG_ERROR,
                "The sensor_id flag should be followed by a sensor_id"
                "and follow an --ID flag");
            return print_usage();
          }
          break;

        case 'n':
          /* set name of previous sensor */
          if (optarg && ds_set[ds_cnt].sensor) {
            strcpy(ds_set[ds_cnt].meas.name, optarg);
          } else {
            log_stderr(LOG_ERROR,
                "The name flag should be followed by a string and should "
                "follow an --ID flag");
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

        case 'S':
          /* Set number if seconds between readings */
          if (optarg) {
            secs = atoi(optarg);
          } else {
            log_stderr(LOG_ERROR,
                "The seconds flag should be followed by an integer");
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

  log_stdout(LOG_INFO, "Initialising socket connection");

  init_linux_socket_connection(&conn, broker_ip, sizeof(broker_ip),
      broker_port);
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

  do { /* Main process loop */

    /* set reading date to now */
    t = time(0);
    localtime_r(&t, &r->t);

    if (!pkt ||
        (ret = set_publish_variable_header(pkt, topic, strlen(topic)))) {
      log_stderr(LOG_ERROR, "Setting up packet");
      ret = UMQTT_ERROR;
      goto free;
    }

    if ((ret = set_publish_fixed_flags(pkt, retain, 0, 0))) {
      log_stderr(LOG_ERROR, "Setting publish flags");
      ret = UMQTT_ERROR;
      goto free;
    }

    if (all_devices) {
      ds18b20_reading(r, NULL);

    } else {
      /* Only process defined sensors */
      for (i = 0; i < MAX_NO_DS18B20; i++) {
        if (*ds_set[i].sensor) {
          ds18b20_reading(r, &ds_set[i]);
        } else {
          /* Must be at the end of the set */
          break;
        }
      }
    }

    /* convert reading ready for mqtt tx if we have valid reading */
    if (r->count > 0) {
      convert_reading_json(r, msg, &len);

      log_stderr(LOG_DEBUG, "Constructed MQTT PUBLISH packet with:");
      log_stderr(LOG_DEBUG, "Topic: %s", topic);
      log_stderr(LOG_DEBUG, "Message: %s", msg);

      ret = init_packet_payload(pkt, PUBLISH, (uint8_t *)msg, strlen(msg));
      if (ret) {
        log_stderr(LOG_ERROR, "Attaching payload");
        goto free;
      }

      finalise_packet(pkt);

      log_stdout(LOG_INFO, "Sending temperature packet to broker");

      /* Send packets */
      ret = broker_send_packet(conn, pkt);
      if (ret) {
        log_stderr(LOG_ERROR, "Sending packet failed");
      } else {
        log_stdout(LOG_INFO, "Successfully sent packet");
      }

    } else {
      log_stderr(LOG_ERROR, "Refusing to send an empty reading");
      /* if we are here, it is likely that we have disconnected
         from the broker. We need to:
         * Check connection
         * attempt to reconnect
       */
    }

    if (secs) {
      /* reset reading */
      free_measurements(r);

      sleep(secs);
    }

  } while (secs);



free:
  log_stdout(LOG_INFO, "Disconnecting from broker");
  broker_disconnect(conn);
  free_reading(r);
  free_connection(conn);
  free_packet(pkt);
  return ret;
}
