/******************************************************************************
 * File: uMQTT_sub.c
 * Description: MicroMQTT (uMQTT) sublish application using linux based sockets
 *              to connect to the broker.
 *              constrained environments.
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
#include <string.h>

#include <getopt.h>

#include "uMQTT.h"
#include "uMQTT_helper.h"
#include "uMQTT_linux_client.h"

#include "sensorspace.h"
#include "reading/reading.h"
#include "log.h"

#ifdef RASPBERRYPI
#include <bcm2835.h>

#define PIN_SCLK    RPI_V2_GPIO_P1_36
#define PIN_RCLK    RPI_V2_GPIO_P1_38
#define PIN_DIO     RPI_V2_GPIO_P1_37
#else
#define PIN_SCLK    1
#define PIN_RCLK    1
#define PIN_DIO     1
#endif

#include "ssdisplay.h"

#define MAX_TOPIC_LEN         1024
#define MAX_MSG_LEN           1024

#define DEFAULT_TOPIC         "sensorspace/reading/temp/"

static int print_usage(void);
/*
 * \brief function to print help
 */
static int print_usage() {

  fprintf(stderr,
      "display_mqtt is an application that connects to an MQTT broker and\n"
      "subscribes to a number of predefined topics. Specified sensor \n"
      "measurements are then sent to the connected displays.\n"
      "\n"
      "Usage: display_mqtt [options]\n"
      "General options:\n"
      " -h [--help]              : Displays this help and exits\n"
      "\n"
      "Display options:\n"
      " -n [--cascaded] <No>     : The number of cascaded displays,\n"
      "                            default: 0.\n"
      "\n"
      "Subscribe options:\n"
      " -t [--topic] <topic>     : Change the default topic. Default: uMQTT_PUB\n"
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
      " -C [--counts]            : Print packet counts\n"
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
      " -e [--error]              : Exit on first error\n"
      "\n");

  return 0;
}

int main(int argc, char **argv) {

  umqtt_ret ret;
  int c, option_index = 0, i;
  uint8_t detail = 0, error = 0, counts = 0;
  char topic[MAX_TOPIC_LEN] = DEFAULT_TOPIC;
  char broker_ip[16] = MQTT_BROKER_IP;
  struct broker_conn *conn;
  int broker_port = MQTT_BROKER_PORT;
  char clientid[UMQTT_CLIENTID_MAX_LEN] = "\0";
  int cas = 1;

  /* initialise new reading */
  struct reading *r = NULL;

  static struct option long_options[] =
  {
    /* These options set a flag. */
    {"help",   no_argument,             0, 'h'},
    {"verbose", required_argument,      0, 'v'},
    {"cascaded", required_argument,     0, 'n'},
    {"detail", no_argument,             0, 'd'},
    {"counts", no_argument,             0, 'C'},
    {"error", no_argument,              0, 'e'},
    {"topic", required_argument,        0, 't'},
    {"broker", required_argument,       0, 'b'},
    {"port", required_argument,         0, 'p'},
    {"clientid", required_argument,     0, 'c'},
    {0, 0, 0, 0}
  };

  struct display *d;

  /* get arguments */
  while (1)
  {
    if ((c = getopt_long(argc, argv, "hCdev:n:t:b:p:c:", long_options,
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

        case 'C':
          /* print packet counts */
          counts = 1;
          break;

        case 'd':
          /* set detailed output */
          detail = 1;
          break;

        case 't':
          /* Set topic */
          if (optarg) {
            strcpy(topic, optarg);
          } else {
            log_stderr(LOG_ERROR,
                "The topic flag should be followed by a topic.");
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

        case 'e':
          /* Break on error */
          error = 1;
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

        case 'n':
          /* Set the number of cascaded displays */
          if (optarg) {
            cas = atoi(optarg);
          } else {
            log_stderr(LOG_ERROR,
                "The cascaded flag should be followed by an integer");
            return print_usage();
          }
          break;
      }
    } else {
      break;
    }
  }

  /* Display initialisation */
  ret = init_display_mem(&d, cas);
  if (ret) {
    log_stderr(LOG_ERROR, "Failed to initialise display memory");
    return ret;
  }

  set_display_pins(d, PIN_SCLK, PIN_RCLK, PIN_DIO);

  if (ret) {
    log_stderr(LOG_ERROR, "Failed to initialise display memory");
    return ret;
  }
  if (!display_init(d)) {
    /* parent */
    //struct display *dc = d;
    for (i = 0; i < cas; i++) {
      /* Set temp output on each display 
      dc->digit[0] = DIGIT_1 | DISP_8 | DISP_DP;
      dc->digit[1] = DIGIT_2 | DISP_E | DISP_DP;
      dc->digit[2] = DIGIT_3 | DISP_E | DISP_DP;
      dc->digit[3] = DIGIT_4 | DISP_A | DISP_DP;
      dc = dc->cascaded;
      */
    }
  } else {
    /* child thread - should only get here once finished */
      return 0;
  }

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

  log_stderr(LOG_INFO, "Subscribing to the following topics:");
  log_stderr(LOG_INFO, "Topic: %s", topic);

  /* Find length of topic and subscribe */
  const char *end = strchr(topic, '\0');
  if (!end || (ret = broker_subscribe(conn, topic, end - topic))) {

    log_stderr(LOG_ERROR, "Subscribing to topic.");
    return UMQTT_ERROR;
  }

  /* Start listening for packets */
  struct mqtt_packet *pkt = NULL;
  if (init_packet(&pkt)) {
    log_stderr(LOG_ERROR, "Initialising packet");
    ret = UMQTT_ERROR;
    goto free;
  }

  while (1) {
    /* packet border */
    log_stderr(LOG_INFO,
        "------------------------------------------------------------");
    if (counts) {
      log_stderr(LOG_INFO,
          "Packet counts: Successful: %d Failed: %d, Publish: %d",
          conn->success_count, conn->fail_count, conn->publish_count);
    }

    ret = conn->receive_method(conn, pkt);
    if (!ret) {
      if (detail) {
        print_packet_detailed_info(pkt);
      } else {
        print_publish_packet_info(pkt);
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

      print_reading(r);

      char measurement[64];
      //get_sensor_id_measurement(r, 0,  measurement, 64);

      get_sensor_name_measurement(r, (char *)"Boil Kettle Temp",
          measurement, 64);
      set_display_str(d, measurement);
      printf("measurement: %s\n", measurement);

      get_sensor_name_measurement(r, (char *)"Mash Tun Temp",
          measurement, 64);
      set_display_str(d->cascaded, measurement);
      printf("measurement: %s\n", measurement);

      get_sensor_name_measurement(r, (char *)"HLT",
          measurement, 64);
      set_display_str(d->cascaded->cascaded, measurement);
      printf("measurement: %s\n", measurement);

      /* cleanup after reading */
      free_reading(r);

    } else if (error) {
      break;
    }
  }

free:
  broker_disconnect(conn);
  free_connection(conn);
  free_packet(pkt);
  free_reading(r);
  return ret;
}
