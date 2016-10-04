/******************************************************************************
 * File: tty_mqtt.c
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
#include <string.h>
#include <stdbool.h>
#include <errno.h>

#include <getopt.h>

#include "uMQTT.h"
#include "uMQTT_linux_client.h"

#include "sensorspace.h"
#include "reading/reading.h"
#include "serial/tty_conn.h"
#include "log.h"

#define MQTT_DEFAULT_TOPIC    "sensorspace/readings/"

#define TTY_FILE_NAME_LEN     128
#define DEFAULT_TTY_DEV       "/dev/ttyUSB0"
#define DEFAULT_TTY_BAUD      115200

#define RX_BUF_LEN            2048
#define MAX_TOPIC_LEN         1024
#define MAX_MSG_LEN           2048

#define CC_DEVICE_STR         "CC_DEV"
#define FLOW_DEVICE_STR       "FLOW_DEV"
#define RAW_DEVICE_STR        "RAW_DEV"

#define CC_DEV_MSG_END_STRING     "</msg>"

typedef enum {
  CURRENT_COST_DEV,
  FLOW_DEV,
  RAW_DEV
} device_type_t;

static int print_usage(void);
static int process_cc_buffer(char *buf, size_t *len);

/*
 * \brief function to print help
 */
static int print_usage() {

  fprintf(stderr,
      "tty_mqtt is an application that connects to a tty device and processes\n"
      "incoming data into readings, before sending as an MQTT PUBLISH packet\n"
      "\n"
      "Usage: tty_mqtt [options] -D <device-type> -d <id> -s <id>'\n"
      "General options:\n"
      " -h [--help]              : Displays this help and exits\n"
      "\n"
      "Device/Reading options:\n"
      " -T [--type] <type>       : The device type, supported options are:\n"
      "                            CC_DEV      Current cost device\n"
      "                            FLOW_DEV    Flow meter device\n"
      "                            RAW_DEV     Raw device\n"
      "                            This should be the first option provided\n"
      " -d [--device_id] <id>    : The device_id the reading is attached to.\n"
      " -s [--sensor_id] <id>    : The sensor_ids of the measurements\n"
      "                            Can be used multiple times.\n"
      " -j [--json]              : Embed the reading in JSON format (DEFAULT)\n"
      " -i [--ini]               : Embed the reading in INI format\n"
      "                            NOTE: Not currently supported\n"
      "\n"
      "TTY Device options:\n"
      " -D [--tty-dev] <tty-dev> : The TTY device to connect to.\n"
      " -B [--baud] <baud rate>         : The baud rate of the connection.\n"
      "\n"
      "Publish options:\n"
      " -t [--topic] <topic>     : Change the default topic. \n"
      "                            Default: 'sensorspace/readings/'\n"
      " -r [--retain]            : Set the retain flag\n"
      "\n"
      "Broker options:\n"
      " -b [--broker] <broker-IP>: Change the default broker IP - only IP\n"
      "                            addresses are currently supported.\n"
      "                            Default: localhost\n"
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
      "\n");

  return 0;
}

/*
 * \brief Fucntion to process incoming currentcost data.
 * \param buf data buffer holding raw TTY data
 * \param len length of buffer
 */
static int process_cc_buffer(char *buf, size_t *len) {
  static char cc_buf[RX_BUF_LEN];
  static size_t cc_buf_len = 0;
  static bool oversized = false;

  /* backup bytes received until we receive newline */
  strncpy((char *)&cc_buf + cc_buf_len, buf, *len);
  cc_buf_len += *len;

  if (0) {
    /* test conversion */
    strncpy((char *)&cc_buf,
        "<msg><src>CC128-v0.12</src><dsb>00396</dsb><time>02:49:31</time>"
        "<tmpr>21.9</tmpr><sensor>0</sensor><id>00983</id><type>1</type>"
        "<ch1><watts>00836</watts></ch1></msg>",
        strlen(
          "<msg><src>CC128-v0.12</src><dsb>00396</dsb><time>02:49:31</time>"
          "<tmpr>21.9</tmpr><sensor>0</sensor><id>00983</id><type>1</type>"
          "<ch1><watts>00836</watts></ch1></msg>"));
  }


  /* ensure complete cc packet */
  if (!strstr(cc_buf, "\n") && !strstr(cc_buf, CC_DEV_MSG_END_STRING)) {
    /* discard oversized packets */
    if (cc_buf_len == RX_BUF_LEN) {
      log_stderr(LOG_ERROR, "TTY packet oversized, discarding data");
      oversized = true;
      cc_buf_len = 0;
      return SS_CONTINUE;
    }

    log_stdout(LOG_DEBUG, "awaiting more data");
    return SS_CONTINUE;
  }

  if (oversized) {
    oversized = false;
    cc_buf_len = 0;
    log_stderr(LOG_DEBUG, "discarding remains of oversized packet");
    return SS_CONTINUE;
  }

  if (cc_buf_len <= 1) {
    /* Probably just a black line, ignore */
    log_stdout(LOG_DEBUG, "ignoring empty line");
    cc_buf_len = 0;
    return SS_CONTINUE;
  }


  cc_buf_len = 0;
  strncpy((char *)buf, (char *)&cc_buf, cc_buf_len);
  *len = cc_buf_len;

  return SS_SUCCESS;
}

int main(int argc, char **argv) {

  int ret;
  int c, option_index = 0;

  /* select variables */
  fd_set read_fds;
  int nfds = 0;

  /* mqtt variables */
  char topic[MAX_TOPIC_LEN] = MQTT_DEFAULT_TOPIC;
  char broker_ip[16] = MQTT_BROKER_IP;
  int broker_port = MQTT_BROKER_PORT;
  char clientid[UMQTT_CLIENTID_MAX_LEN] = "\0";
  uint8_t retain = 0;
  char msg[MAX_MSG_LEN] = "\0";
  size_t len = MAX_MSG_LEN;
  struct broker_conn *conn;
  struct mqtt_packet *pkt = NULL;

  /* reading variables */
  struct reading *r = NULL;
  ret = reading_init(&r);
  if (ret) {
    return -1;
  }

  /* set reading date to now */
  time_t t = time(0);

  /* tty variables */
  char buf[RX_BUF_LEN];
  size_t buf_len = RX_BUF_LEN;
  device_type_t type = RAW_DEV;

  struct tty_conn *tty;
  ret = tty_conn_init(&tty);
  if (ret) {
    free_reading(r);
    return -1;
  }

  /* set TTY defaults */
  strcpy(tty->path, DEFAULT_TTY_DEV);
  tty->baud = DEFAULT_TTY_BAUD;

  static struct option long_options[] =
  {
    /* These options set a flag. */
    {"help",   no_argument,             0, 'h'},
    {"verbose", required_argument,      0, 'v'},
    {"type", required_argument,         0, 'T'},
    {"device_id", required_argument,     0, 'd'},
    {"sensor_id", required_argument,     0, 's'},
    {"json", no_argument,               0, 'j'},
    {"ini", no_argument,                0, 'i'},
    {"retain",  no_argument,            0, 'r'},
    {"tty-dev", required_argument,      0, 'D'},
    {"baud", required_argument,         0, 'B'},
    {"topic", required_argument,        0, 't'},
    {"broker", required_argument,       0, 'b'},
    {"port", required_argument,         0, 'p'},
    {"clientid", required_argument,     0, 'c'},
    {0, 0, 0, 0}
  };

  /* get arguments */
  while (1)
  {
    if ((c = getopt_long(argc, argv, "hv:s:d:T:D:B:jirt:b:p:c:", long_options,
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
            strcpy(topic, optarg);
          } else {
            log_stderr(LOG_ERROR,
                "The topic flag should be followed by a topic");
            return print_usage();
          }
          break;

        case 'T':
          /* Set the device type */
          if (optarg) {

            if (strstr(optarg, CC_DEVICE_STR)) {
              type = CURRENT_COST_DEV;

            } else if (strstr(optarg, FLOW_DEVICE_STR)) {
              type = FLOW_DEV;

            } else {
              /* default */
              type = RAW_DEV;
            }

          } else {
            log_stderr(LOG_ERROR,
                "The device type flag should be followed by a device type");
            return print_usage();
          }
          break;

        case 'D':
          /* Set the TTY device file */
          if (optarg) {
            strcpy(tty->path, optarg);

          } else {
            log_stderr(LOG_ERROR,
                "The device flag should be followed by a tty device");
            return print_usage();
          }
          break;

        case 'B':
          /* Set the device file */
          if (optarg) {
            tty->baud = atoi(optarg);
          } else {
            log_stderr(LOG_ERROR,
                "The baud rate flag should be followed by a baud rate");
            return print_usage();
          }
          break;

        case 'd':
          /* Set a device_id */
          if (optarg) {
            r->device_id = atoi(optarg);
          } else {
            log_stderr(LOG_ERROR,
                "The device_id flag should be followed by a device_id");
            return print_usage();
          }
          break;

        case 's':
          /* Set a sensor_id */
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
          /* Change the default broker ip */
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

  /* init connections */
  log_stdout(LOG_INFO, "Initialising broker socket connection");

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

  /* test tty config */
  log_stdout(LOG_INFO, "TTY device: %s, baud: %d", tty->path, tty->baud);
  if (tty_conn_check_config(tty)) {
    log_stdout(LOG_ERROR, "Testing TTY device config");
    return -1;
  }

  /* Open tty device */
  ret = tty_conn_open(tty);
  if (ret) {
    log_stderr(LOG_ERROR, "Opening TTY device");
    free_connection(conn);
    free_tty_conn(tty);
    return ret;
  }

  /* wait for data - main program loop */
  while (1) {

    /* select init */
    FD_ZERO(&read_fds);
    nfds = ((tty->fd > skt->sockfd) ? tty->fd : skt->sockfd) + 1;
    FD_SET(tty->fd, &read_fds);
    FD_SET(skt->sockfd, &read_fds);

    ret = select(nfds, &read_fds, NULL, NULL, NULL);
    if (ret == -1) {
      if (errno == EINTR) {
        log_stderr(LOG_ERROR, "Select: %s: %s", tty->path, strerror(errno));
        continue;
      }
      log_stderr(LOG_ERROR, "Select failed: %s: %s", tty->path, strerror(errno));
      continue;

    } else if (!ret) {
      /* should never get here since disabled timeout */
      log_stdout(LOG_DEBUG, "select timed out");
    }

    /* Determine if any fds are ready */
    if (FD_ISSET(tty->fd, &read_fds)) {

      /* set reading time to now */
      t = time(0);
      localtime_r(&t, &r->t);

      buf_len = RX_BUF_LEN;
      ret = tty_conn_read(tty, (char *)&buf, &buf_len);
      if (ret && ret != SS_CONTINUE) {
        log_stderr(LOG_ERROR, "Read: %d:%s", errno, strerror(errno));
        break;

      } else if (ret == SS_CONTINUE) {
        continue;

      } else if (buf_len) {
        /* process tty data */
        if (CURRENT_COST_DEV == type) {

          ret = process_cc_buffer(buf, &buf_len);
          if (ret == SS_CONTINUE) {
            continue;
          }

          /* process reading */
          free_measurements(r);
          ret = convert_cc_dev_reading(r, buf, buf_len);
          if (ret) {
            log_stderr(LOG_ERROR, "failed to decode output");
            continue;
          }

        } else if (FLOW_DEV == type) {
          /* not currently supported */

        } else if (RAW_DEV == type) {
          /* Simply copy buffer to message payload */
          memcpy((void *)msg, (void *)buf, (buf_len < len ? buf_len : len));
        }

        if (type != RAW_DEV) {
          /* apply message processing */
          ret = convert_reading_json(r, msg, &len);
          if (ret) {
            log_stderr(LOG_ERROR, "failed to encode reading into JSON");
            continue;
          }
        }

      } else if (FD_ISSET(skt->sockfd, &read_fds)) {
        /* process MQTT input */
        /* need to test this to ensure packets are processed upon recipt */
        ret = read_socket_packet(conn, pkt);
        if (ret) {
          log_stderr(LOG_ERROR, "failed to process packet input");
          continue;
        }

      }

      /* Process output MQTT packet is fall through case. continue to avoid */
      /* Create publish packet on new data */
      pkt = construct_packet_headers(PUBLISH);

      if (!pkt ||
          (ret = set_publish_variable_header(pkt, topic, strlen(topic)))) {
        log_stderr(LOG_ERROR, "Setting up packet");
        ret = UMQTT_ERROR;
        goto free;
      }

      ret = set_publish_fixed_flags(pkt, retain, 0, 0);
      if (ret) {
        log_stderr(LOG_ERROR, "Setting publish flags");
        ret = UMQTT_ERROR;
        goto free;
      }

      ret = init_packet_payload(pkt, PUBLISH, (uint8_t *)msg, strlen(msg));
      if (ret) {
        log_stderr(LOG_ERROR, "Attaching payload");
        ret = UMQTT_ERROR;
        goto free;
      }

      finalise_packet(pkt);

      log_stdout(LOG_INFO, "Constructed MQTT PUBLISH packet with:");
      log_stdout(LOG_INFO, "Topic: %s", topic);
      log_stdout(LOG_INFO, "Message: %s", msg);

      /* Send packet */
      ret = broker_send_packet(conn, pkt);
      if (ret) {
        log_stderr(LOG_ERROR, "Sending packet failed");
      } else {
        log_stdout(LOG_INFO, "Successfully sent packet to broker");
      }

      free_packet(pkt);
      buf_len = RX_BUF_LEN;
    }
  }


free:
  log_stdout(LOG_INFO, "Disconnecting from broker");
  broker_disconnect(conn);
  free_reading(r);
  close_tty_conn(tty);
  free_tty_conn(tty);
  free_connection(conn);
  free_packet(pkt);
  return ret;
}
