/******************************************************************************
 * File: pid_mqtt.c
 * Description: A PID controller over MQTT
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
#include <errno.h>
#include <sys/time.h>

#include <getopt.h>

#include "uMQTT.h"
#include "uMQTT_linux_client.h"

#include "sensorspace.h"
#include "reading.h"
#include "controller.h"
#include "log.h"

#define MQTT_DEFAULT_TOPIC    "sensorspace/reading"

#define MAX_TOPIC_LEN 1024
#define MAX_MSG_LEN 2048

/* Required for test mode */
struct controller_data {
  uint32_t second;
  float val;
};

static int print_usage(void);
int file_read_contents(const char *filename, uint8_t *buf, size_t *len);
int file_read_controller_data(const char *filename,
    struct controller_data **pdata);
size_t file_get_size(const char *filename);
void test_mode(struct pid_ctrl *pid, char *test_file);
int convert_pid_reading(struct pid_ctrl *p, struct reading *r);

/*
 * \brief function to print help
 */
static int print_usage() {

  fprintf(stderr,
      "pid_mqtt is an application that provides controller functionality\n"
      "on an MQTT network by allowing reading variables to be controlled.\n"
      "Usage: pid_mqtt [options]\n\n"
      "General options:\n"
      " -h [--help]              : Displays this help and exits\n"
      "\n"
      "Publish options:\n"
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
      " -c [--clientid] <id>     : Change the default clientid Default: PID\n"
      "\n"
      "Controller options:\n"
      " -i [--sample-int] <secs> : sampling interval, seconds, default 1S.\n"
      " -S [--set-point]         : Set the set point.\n"
      " -P [--Kp] <prop gain>    : Set the proportional gain\n"
      " -I [--Ki] <prop gain>    : Set the integral gain\n"
      " -D [--Kd] <prop gain>    : Set the derivitive gain\n"
      " -T [--pv-topic] <topic>  : Set the topic.for the reading message \n"
      "                          : that includes the process variable.\n"
      " -I [--pv-sid] <sensorid> : Set the sensorID for the process variable.\n"
      " -n [--pv-name] <name>    : Set the sensor name for the process variable.\n"
      " -E [--ie-max] <val>      : The maximum value of the integral error.\n"
      "                          : Default: 2.0\n"
      " -e [--ie-min] <val>      : The minumim value of the integral error.\n"
      "                          : Default: -2.0\n"
      " -U [--idle-upper] <val>  : The allowable upper band in which no control\n"
      "                          : action will be applied. Default: 1.0\n"
      " -u [--idle-lower] <val>  : The allowable lower band in which no control\n"
      "                          : action will be applied. Default: -1.0\n"
      " -O [--up-on] <val>       : The upper value of the PID output that will\n"
      "                          : drive the PV increase on. Default 1.0\n"
      " -o [--down-on] <val>     : The lower value of the PID output that will\n"
      "                          : drive the PV reducer on. Default -1.0\n"
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
      " -D [--debug-mqtt]        : Send PID debug data as part of MQTT message\n"
      " -M [--test-mode] <file>  : Start the controller in test mode using the\n"
      "                            data found in <file> as input data.\n"
      "\n");

  return 0;
}

/*
 * \brief Function to convert elements of struct pid_ctrl to a reading
 */
int convert_pid_reading(struct pid_ctrl *p, struct reading *r) {

  int ret;
  sprintf(r->name, "PID Controller");
  /* take each pid varibale and create a measurement */
  ret = measurement_init(r);
  if (ret) {
    return ret;
  }
  sprintf(r->meas[r->count - 1]->name, "Set Point");
  sprintf(r->meas[r->count - 1]->meas, "%f", p->sp);

  ret = measurement_init(r);
  if (ret) {
    return ret;
  }
  sprintf(r->meas[r->count - 1]->name, "Process Variable");
  sprintf(r->meas[r->count - 1]->meas, "%f", p->pv);

  ret = measurement_init(r);
  if (ret) {
    return ret;
  }
  sprintf(r->meas[r->count - 1]->name, "Error");
  sprintf(r->meas[r->count - 1]->meas, "%f", p->e);

  ret = measurement_init(r);
  if (ret) {
    return ret;
  }
  sprintf(r->meas[r->count - 1]->name, "Output");
  sprintf(r->meas[r->count - 1]->meas, "%f", p->out);

  ret = measurement_init(r);
  if (ret) {
    return ret;
  }
  sprintf(r->meas[r->count - 1]->name, "PV_up");
  sprintf(r->meas[r->count - 1]->meas, "%s", p->pv_up ? "true" : "false");

  ret = measurement_init(r);
  if (ret) {
    return ret;
  }
  sprintf(r->meas[r->count - 1]->name, "PV_down");
  sprintf(r->meas[r->count - 1]->meas, "%s", p->pv_down ? "true" : "false");
  return 0;
}

/*
 * \brief Function to convert a csv file into an array of values
 * \param filename The file to read.
 * \param pdata pointer to resultant data structure.
 * \return on sucess, the number of data entries, -1 on failure.
 */
int file_read_controller_data(const char *filename,
    struct controller_data **pdata) {

  int ret = 0;
  size_t lines;
  ssize_t line_len = 0;
  size_t buf_len = 128;
  char *buf;
  char *val_p = NULL;
  struct controller_data *data;

  FILE *f = fopen(filename, "rb");

  /* allocate memory for buffer */
  buf = calloc(sizeof(char), buf_len);
  if (!buf) {
    log_stderr(LOG_ERROR, "Failed to allocate memory for buffer");
    ret = -1;
    goto free_file;
  }

  /* count lines in file - no of records */
  for (lines = 0; getline(&buf, &buf_len, f) != -1; lines++);
  log_stdout(LOG_INFO, "Found %zu lines in file %s", lines, filename);
  fseek(f, 0, SEEK_SET);

  /* allocate memory for data */
  data = calloc(sizeof(struct controller_data), lines);
  if (!data) {
    log_stderr(LOG_ERROR, "Failed to allocate memory for data array");
    ret = -1;
    goto free_file;
  }

  while ((line_len = getline(&buf, &buf_len, f)) != -1) {
    val_p = strstr(buf, ",") + sizeof(char);
    if (!val_p) {
      /* Either eof or an error has occured */
      continue;
    }
    data[ret].second = atoi(buf);
    data[ret].val = atof(val_p);
    /* ToDo: validate the data */
    log_stderr(LOG_DEBUG, "New data value @ %d seconds: %f",
        data[ret].second, data[ret].val);
    ret++;
  }

  *pdata = data;
free_file:
  free(buf);
  fclose(f);

  return ret;
}

/*
 * \brief Function to read the contents of a file into buffer.
 * \param filename The file to read.
 * \param buf The buffer for the file.
 * \param len The len of the buffer.
 */
int file_read_contents(const char *filename, uint8_t *buf, size_t *len) {

  int ret = 0;

  FILE *f = fopen(filename, "rb");
  fseek(f, 0, SEEK_END);
  size_t fsize = ftell(f);
  fseek(f, 0, SEEK_SET);

  if (fsize > *len) {
    log_stderr(LOG_ERROR, "The file (%zu bytes) is larger than buffer (%zu bytes)",
        fsize, *len);
    ret = -1;
  } else {
    *len = fread(buf, 1, fsize, f);
  }

  fclose(f);

  return ret;
}

/*
 * \brief Function to return the size of a file.
 * \param filename The file to read.
 */
size_t file_get_size(const char *filename) {

  FILE *f = fopen(filename, "rb");
  fseek(f, 0, SEEK_END);
  size_t fsize = ftell(f);
  fclose(f);

  return fsize;
}


void test_mode(struct pid_ctrl *pid, char *test_file) {
  uint32_t time_idx;
  size_t len = 0;
  size_t data_idx = 0;
  struct controller_data *data = NULL;

  len = file_read_controller_data(test_file, &data);

  log_stdout(LOG_INFO, "Loaded %d data points.", len);

  log_stdout(LOG_INFO, "time, sp, PV, Pe, De, Ie, e, out, heat state, cool_state");
  /* initial conditions */
  log_stdout(LOG_INFO, "0, %f, %f, %f, %f, %f, %f, %f, %d, %d", pid->sp,
      pid->pv, pid->pe, pid->de, pid->ie, pid->e, pid->out,
      (pid->pv_up ? 1 : 0), (pid->pv_down ? 1 : 0));

  log_stderr(LOG_DEBUG, "looping %zu times, for a max of seconds\n", len);
  for (time_idx = 1; time_idx < data[len - 1].second; time_idx++) {
    /* Ensure we are using the latest pv from the dataset */
    while (data[data_idx].second <= time_idx) {
      pid->pv = data[data_idx++].val;
      log_stderr(LOG_DEBUG, "Advancing data_idx to: %zu\n", data_idx);
    }
    log_stderr(LOG_DEBUG, "Time: %d, value: %f, data_idx: %zu", time_idx,
        data[data_idx].val, data_idx);

    /* perform controller action on current value of pv */
    pid_controller(pid);

    /* print csv */
    log_stdout(LOG_INFO, "%d, %f, %f, %f, %f, %f, %f, %f, %d, %d", time_idx,
        pid->sp, pid->pv, pid->pe, pid->de, pid->ie, pid->e, pid->out,
        (pid->pv_up ? 1 : 0), (pid->pv_down ? 1 : 0));

#if 0
    /* break for debugging */
    if (time_idx>1000)
      break;
#endif
  }

  return;
}

int main(int argc, char **argv) {

  int ret;
  int c, option_index = 0;
  char topic[MAX_TOPIC_LEN] = MQTT_DEFAULT_TOPIC;
  char broker_ip[16] = MQTT_BROKER_IP;
  int broker_port = MQTT_BROKER_PORT;
  char clientid[UMQTT_CLIENTID_MAX_LEN] = "\0";
  char test_file[512] = "\0";
  char json[1026] = "\0";
  size_t len = 0;
  uint8_t retain = 0;

  /* select variables */
  fd_set read_fds;
  int nfds = 0;
  struct timeval timeout;

  /* PID variables */
  struct pid_ctrl pid = { 0 };

  /* reading variables */

  static struct option long_options[] =
  {
    /* These options set a flag. */
    {"help",   no_argument,             0, 'h'},
    {"sample-int", required_argument,   0, 'i'},
    {"set-point", required_argument,    0, 'S'},
    {"Kp", required_argument,           0, 'P'},
    {"Ki", required_argument,           0, 'I'},
    {"Kd", required_argument,           0, 'D'},
    {"pv-topic", required_argument,     0, 'T'},
    {"pv-sid", required_argument,       0, 's'},
    {"pv-name", required_argument,      0, 'n'},
    {"verbose", required_argument,      0, 'v'},
    {"topic", required_argument,        0, 't'},
    {"retain",  no_argument,            0, 'r'},
    {"broker", required_argument,       0, 'b'},
    {"port", required_argument,         0, 'p'},
    {"clientid", required_argument,     0, 'c'},
    {"test-mode", required_argument,    0, 'M'},
    {"ie-max", required_argument,       0, 'E'},
    {"ie-min", required_argument,       0, 'e'},
    {"idle-upper", required_argument,   0, 'U'},
    {"idle-lower", required_argument,   0, 'u'},
    {"up-on", required_argument,        0, 'O'},
    {"down-on", required_argument,      0, 'o'},
    {0, 0, 0, 0}
  };

  /* get arguments */
  while (1)
  {
    if ((c = getopt_long(argc, argv,
            "hi:S:P:I:D:T:s:n:V:T:v:t:rb:p:c:M:E:e:U:u:O:o:",
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
            strcpy(topic, optarg);
          } else {
            log_stderr(LOG_ERROR,
                "The topic flag should be followed by a topic");
            return print_usage();
          }
          break;

        case 'i':
          /* set the sampling interval */
          if (optarg) {
            pid.sinterval = atoi(optarg);
          } else {
            log_stderr(LOG_ERROR,
                "The sampling interval flag should be followed by an interval");
            return print_usage();
          }
          break;

        case 'S':
          /* set the setpoint */
          if (optarg) {
            pid.sp = atoi(optarg);
          } else {
            log_stderr(LOG_ERROR,
                "The set-point interval flag should be followed by an set-point");
            return print_usage();
          }
          break;

        case 'P':
          /* set the proportional gain */
          if (optarg) {
            pid.kp = atof(optarg);
          } else {
            log_stderr(LOG_ERROR,
                "The Kp flag should be followed by a gain value");
            return print_usage();
          }
          break;

        case 'I':
          /* set the integral gain */
          if (optarg) {
            pid.ki = atof(optarg);
          } else {
            log_stderr(LOG_ERROR,
                "The Ki flag should be followed by a gain value");
            return print_usage();
          }
          break;

        case 'D':
          /* set the differential gain */
          if (optarg) {
            pid.kd = atof(optarg);
          } else {
            log_stderr(LOG_ERROR,
                "The Kd flag should be followed by a gain value");
            return print_usage();
          }
          break;

        case 'E':
          /* integral max value */
          if (optarg) {
            pid.ie_max = atof(optarg);
          } else {
            log_stderr(LOG_ERROR,
                "The integral max flag should be followed by a value");
            return print_usage();
          }

        case 'e':
          /* integral min value */
          if (optarg) {
            pid.ie_min = atof(optarg);
          } else {
            log_stderr(LOG_ERROR,
                "The integral min flag should be followed by a value");
            return print_usage();
          }

        case 'U':
          /* idle upper limit */
          if (optarg) {
            pid.idle_ulimit = atof(optarg);
          } else {
            log_stderr(LOG_ERROR,
                "The idle upper limit flag should be followed by a value");
            return print_usage();
          }

        case 'u':
          /* idle lower limit */
          if (optarg) {
            pid.idle_llimit = atof(optarg);
          } else {
            log_stderr(LOG_ERROR,
                "The idle lower limit flag should be followed by a value");
            return print_usage();
          }

        case 'O':
          /* PV increaser on bound */
          if (optarg) {
            pid.up_on = atof(optarg);
          } else {
            log_stderr(LOG_ERROR,
                "The up-on flag should be followed by a value");
            return print_usage();
          }

        case 'o':
          /* PV decreaser on bound */
          if (optarg) {
            pid.down_on = atof(optarg);
          } else {
            log_stderr(LOG_ERROR,
                "The down-on flag should be followed by a value");
            return print_usage();
          }

        case 'T':
          /* Set the PV topic */
          if (optarg) {
            strcpy(pid.pv_topic, optarg);
          } else {
            log_stderr(LOG_ERROR,
                "The topic flag should be followed by a topic");
            return print_usage();
          }
          break;

        case 's':
          /* set the PV sensor_id */
          if (optarg) {
            pid.pv_sid = atoi(optarg);
          } else {
            log_stderr(LOG_ERROR,
                "The sensor_id flag should follow a sensorid");
            return print_usage();
          }
          break;

        case 'n':
          /* set PV name */
          if (optarg) {
            strcpy(pid.pv_name, optarg);
          } else {
            log_stderr(LOG_ERROR,
                "The pv-name flag should be followed by a string");
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

        case 'M':
          /* Test mode */
          if (optarg) {
            strcpy(test_file, optarg);
          } else {
            log_stderr(LOG_ERROR,
                "The test-mode flag should be followed with a file of test data");
            return print_usage();
          }
          break;
      }
    } else {
      /* Final arguement */
      break;
    }
  }

  /* Set PID default values */
  if (!pid.ie_min)
    pid.ie_min = -2.0;
  if (!pid.ie_max)
    pid.ie_max = 2.0;

  if (!pid.down_on)
    pid.down_on = -1.0;
  if (!pid.up_on)
    pid.up_on = 1.0;

  if (!pid.idle_ulimit)
    pid.idle_ulimit = 1.0;
  if (!pid.idle_llimit)
    pid.idle_llimit = -1.0;

  if (test_file[0]) {
    /* enter test mode */
    test_mode(&pid, test_file);
    printf("%f, %f, %f, %f, %f, %f\n",
        pid.ie_min,
        pid.ie_max,
        pid.down_on,
        pid.up_on,
        pid.idle_ulimit,
        pid.idle_llimit);
    return 0;
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

  /* subscribe to PV-TOPICS */
  log_stdout(LOG_INFO, "Subscribing to the following topic:");
  log_stdout(LOG_INFO, "%s", pid.pv_topic);

  /* Find length of topic and subscribe */
  const char *end = strchr(pid.pv_topic, '\0');
  if (!end || (ret = broker_subscribe(conn, pid.pv_topic, end - pid.pv_topic))) {

    log_stderr(LOG_ERROR, "Subscribing to topic.");
    return ret;
  }

  /* start up delay */
  time_t t;
  time_t sample_time = time(NULL) + 2;
  //time_t time = time();
  struct reading *r;
  struct mqtt_packet *pkt = NULL;
  struct mqtt_packet *rx_pkt = NULL;
  char msg[1028] = "\0";
  int i;

  /* Start listening for packets */
  while (1) {

    /* initialise reading */
    ret = reading_init(&r);
    if (ret) {
      log_stderr(LOG_ERROR, "Failed to initialie reading");
      break;
    }

    /* set reading date/time to now - fallback */
    t = time(0);
    localtime_r(&t, &r->t);

    /* should really be some form of time interrupt */
    if (sample_time <= time(NULL)) {
      /* perform controller action on current value of pv, set next sample time */
      log_stdout(LOG_INFO, "Invoking controller action");
      sample_time += 5;

      if (!pid.update_count) {
        /* do not start PID action until we've had at least 1 update */
        goto next;
      }

      pid_controller(&pid);

      /* send new control values */
      ret = convert_pid_reading(&pid, r);
      if (ret) {
        log_stderr(LOG_ERROR, "Failed to convert PID to reading");
        goto next;
      }

      pkt = construct_packet_headers(PUBLISH);
      if (!pkt) {
        log_stderr(LOG_ERROR, "Failed to initialise PUBLISH packet");
        break;
      }

      /* build topic string */
      sprintf(topic, "%s/controller", pid.pv_topic);
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
      len = sizeof(msg);
      ret = convert_reading_json(r, msg, &len);
      if (ret) {
        log_stderr(LOG_ERROR, "Failed to convert reading to JSON");
        goto next;
      }

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

      /* Send control packet */
      ret = broker_send_packet(conn, pkt);
      if (ret) {
        log_stderr(LOG_ERROR, "Sending control packet failed");
        goto next;
      } else {
        log_stdout(LOG_INFO, "Successfully sent control packet");
      }
    }

    /* select init */
    FD_ZERO(&read_fds);
    nfds = skt->sockfd + 1;
    FD_SET(skt->sockfd, &read_fds);

    timeout.tv_usec = 250;
    timeout.tv_sec = 0;
    ret = select(nfds, &read_fds, NULL, NULL, &timeout);
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
      goto next;
    }

    /* Determine if any fds are ready */
    if (FD_ISSET(skt->sockfd, &read_fds)) {
      /* process MQTT input */

      ret = init_packet(&rx_pkt);
      if (ret) {
        log_stderr(LOG_ERROR, "Failed to initialise receive packet");
        break;
      }

      ret = conn->receive_method(conn, rx_pkt);
      if (ret) {
        log_stderr(LOG_ERROR, "failed to process packet input");
        goto next;
      }

      log_stdout(LOG_INFO, "Received packet - Attempting to convert to a reading");

      /* copy payload data - which should be JSON */
      /* Currently, we assume readings is json, but it could equally be ini? */
      strncpy(json, (const char *)&rx_pkt->payload->data, rx_pkt->pay_len);
      json[rx_pkt->pay_len + 1] = '\0';
      log_stderr(LOG_INFO, "PKT: %s", json);

      ret = convert_json_reading(r, json, len);
      if (ret) {
        log_stderr(LOG_ERROR, "Converting reading from JSON");
        goto next;
      }

      /* if PV-topic get new PV value */
      if (decode_utf8_string(topic, &rx_pkt->variable->publish.topic)) {
        if (!strcmp(topic, pid.pv_topic)) {
          /* Look for PV in reading */
          for (i = 0; i < r->count; i++) {
            if (!strcmp(r->meas[i]->name, pid.pv_name)) {
              log_stdout(LOG_INFO, "Updating process variable: %s - %s",
                  r->meas[i]->name, r->meas[i]->meas);
              pid.pv = atof(r->meas[i]->meas);
              pid.update_count++;
              break;
            }
          }
          /*preferend code from different branch.
            char measurement[64] = "\0";
            ret = get_sensor_name_measurement(r, pid.pv_name, measurement);
            if (measurement[0]) {
            pid.pv = atof(measurement);
            pid.update_count++;
            } */
        }
      }
    }

next:
    free_reading(r);
    r = NULL;
    free_packet(pkt);
    pkt = NULL;
    free_packet(rx_pkt);
    rx_pkt = NULL;
    continue;
  }

free:
  broker_disconnect(conn);
  free_reading(r);
  free_connection(conn);
  free_packet(pkt);
  free_packet(rx_pkt);
  return ret;
}

