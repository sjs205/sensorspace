/******************************************************************************
 * File: db_transports.c
 * Description: functions to provide data transport functionality
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

#include <stdlib.h>
#include <string.h>

#include "db.h"
#include "log.h"

/**
 * \brief converts string to db_type
 */
int convert_db_engine_str(char *db_engine) {
  int ret;
  if (strstr(db_engine, "SQLITE")) {
    ret = DB_SQLITE;
  } else if (strstr(db_engine, "MYSQL")) {
    ret = DB_MYSQL;
  } else {
    ret = DB_NO_ENGINE;
  }
  return ret;
}

/**
 * \brief function to set db config options
 */
int db_tran_file_config(struct db_transport *db_tran, char *buf, unsigned int size) {
  char *delim_char;

  /* db config options */
  if (strstr(buf, "db=")) {
    delim_char = strstr(buf, "=");
    if (delim_char) {
      strcpy(db_tran->db, delim_char+1);
      log_stdout(LOG_DEBUG, "DB: %s", db_tran->db);
    }

  } else if (strstr(buf, "engine=")) {
    delim_char = strstr(buf, "=");
    if (delim_char) {
      db_tran->engine = convert_db_engine_str(delim_char+1);
      log_stdout(LOG_DEBUG, "DB engine: %d", db_tran->engine);
    }

  } else if (strstr(buf, "host=")) {
    delim_char = strstr(buf, "=");
    if (delim_char) {
      strcpy(db_tran->db_host, delim_char+1);
      log_stdout(LOG_DEBUG, "DB host: %s", db_tran->db_host);
    }

  } else if (strstr(buf, "user=")) {
    delim_char = strstr(buf, "=");
    if (delim_char) {
      strcpy(db_tran->db_user, delim_char+1);
      log_stdout(LOG_DEBUG, "DB username: %s", db_tran->db_user);
    }

  } else if (strstr(buf, "pass=")) {
    delim_char = strstr(buf, "=");
    if (delim_char) {
      strcpy(db_tran->db_pass, delim_char+1);
      log_stdout(LOG_DEBUG, "DB password: *******");
    }

  } else {
    return SS_CFG_NO_MATCH;
  }

  return SS_SUCCESS;
}

/**
 * \brief Initialise database context. Note that we initailsie all database
 *        engines with all variables to save config errors
 */
int db_tran_init(struct db_transport **db) {

  struct db_transport *db_tran;

  if (!(db_tran = calloc(1, sizeof(struct db_transport)))) {
    log_stderr(LOG_ERROR, "DB transport: Out of memory");
    goto free;
  }

  if (!(db_tran->db = calloc(DB_STRING_MAX, sizeof(char)))) {
    log_stderr(LOG_ERROR, "DB transport: Out of memory");
    goto free;
  }

  if (!(db_tran->db_user = calloc(DB_USER_STRING_MAX, sizeof(char)))) {
    goto free;
    log_stderr(LOG_ERROR, "DB transport: Out of memory");
  }

  if (!(db_tran->db_pass = calloc(DB_PASS_STRING_MAX, sizeof(char)))) {
    log_stderr(LOG_ERROR, "DB transport: Out of memory");
    goto free;
  }

  if (!(db_tran->db_host = calloc(DB_HOST_STRING_MAX, sizeof(char)))) {
    log_stderr(LOG_ERROR, "DB transport: Out of memory");
    goto free;
  }

  /* statically init mutex */
  db_tran->db_mutex = (pthread_mutex_t)PTHREAD_MUTEX_INITIALIZER;

  *db = db_tran;
  return SS_SUCCESS;

free:
  free_db_tran(db_tran);
  return SS_OUT_OF_MEM_ERROR;
}

/**
 * \brief function to connect to database engine
 */
int db_tran_connect(struct db_transport *db_tran) {

  int ret = SS_SUCCESS;

  /* start of CRITICAL SECTION */
  while (pthread_mutex_lock(&db_tran->db_mutex)) {
    log_stdout(LOG_DEBUG_THREAD, "DB thread Block");
  }
  
  switch (db_tran->engine) {
  case DB_MYSQL:
    if (!(ret = db_mysql_open(db_tran))) {
      log_stdout(LOG_INFO, "Connection successful");
    }
    break;

  case DB_SQLITE:
    if (!(ret = db_sqlite_open(db_tran))) {
      log_stdout(LOG_INFO, "Connection successful");
    }

    break;
  case DB_NO_ENGINE:
    ret = SS_CFG_NO_DB;               
    break;
  }

  /* end of CRITICAL SECTION */
  pthread_mutex_unlock(&db_tran->db_mutex);

  return ret;

}

/**
 * \brief Reconnect to database after failure
 */
static int db_tran_reconnect(struct db_transport *db_tran) {
  int ret = SS_SUCCESS;

  switch (db_tran->engine) {
  case DB_MYSQL:
    ret = db_mysql_reconnect(db_tran);
    break;

  case DB_SQLITE:
    ret = db_sqlite_open(db_tran);
    break;

  case DB_NO_ENGINE:
    break;
  }

  if (ret && ret != SS_CONN_RECONNECT) {
    log_stderr(LOG_ERROR, "Database connection failed");
  }

  return ret;
}


/**
 * \brief Function to initialise query instance
 */
int db_query_init(struct db_query **q_p) {

  struct db_query *q;
  if (!(q = calloc(1, sizeof(struct db_query)))) {
    log_stderr(LOG_ERROR, "DB transport: Out of memory");
    goto free;
  }

  if (!(q->query = calloc(DB_QRY_MAX_LEN, sizeof(char)))) {
    log_stderr(LOG_ERROR, "DB transport: Out of memory");
    goto free;
  }

  *q_p = q;
  return SS_SUCCESS;

free:
  free_db_query(q);
  return SS_OUT_OF_MEM_ERROR;
}


/**
 * \brief function to build SELECT query and query database
 * \param trn transport query is linked to
 * \param r reading result return
 * \param qry query struct
 */
int db_select_query(struct db_transport *trn, struct db_query *qry) {

  int ret = SS_SUCCESS;

  switch (qry->payload_t) {
    case SS_DEV_CONFIG:
      break;
    case SS_SENSE_CONFIG:
      break;
    case SS_SS_CFG:
      break;
    case SS_READING:;
      /*
      struct sensor *sens = (struct sensor *) qry->pl_ctx;

      qry->qry_len = sprintf(qry->query,
        "SELECT * FROM sensors_reading AS r, "
        "sensors_measurement AS m "
        "WHERE r.readingID=m.readingID "
        "AND m.sensorID=%d", sens->sensorID);
        */
      break;
  }

  if (ret) {
    return ret;
  } else {
    return db_query_db(trn, qry);
  }

}

/**
 * \brief function to build reading INSERT query and query database
 */
int db_insert_query(struct db_transport *trn, struct db_query *qry) {

  int ret = SS_SUCCESS;

  switch (qry->payload_t) {
    case SS_DEV_CONFIG:
      break;
    case SS_SENSE_CONFIG:
      break;
    case SS_SS_CFG:
      break;
    case SS_READING:;
      /* insert reading into database */
      struct reading *r = (struct reading *)qry->pl_ctx;

      char db_date[20];
      convert_tm_db_date(&r->t, db_date);

      qry->qry_len = sprintf(qry->query,
        "INSERT INTO sensors_reading(deviceID, date) "
        "VALUES ('%d', '%s')", r->deviceID, db_date);

      if ((ret = db_query_db(trn, qry))) {
        /* error */
        log_stderr(LOG_ERROR, "Reading insert failed");
        break;
      }

      int readingID = qry->insertID;

      /* insert measurements into database - reuse qry */
      int i;
      for(i=0; i < r->count; i++) {
        qry->qry_len = sprintf(qry->query,
            "INSERT INTO sensors_measurement"
            "(readingID,sensorID,measurement) VALUES ('%d','%d','%s')",
            readingID, r->meas[i]->sensorID, r->meas[i]->meas);

        if ((ret = db_query_db(trn, qry))) {
          /* error */
          log_stderr(LOG_ERROR, "Measurement insert failed");
        }
      }

      break;
  }

  return ret;
}

/**
 * \brief Function to build and "INSERT" db query - post transport
 * \param ctx Transport context struct
 * \param payload Transport payload type
 * \param pl_ctx Payload context struct - expects payload of payload_t type
 */
int db_post_method(struct db_transport *db_tran, trn_payload_t payload_t, void *pl_ctx) {

  int ret = SS_SUCCESS;

  struct db_query *qry;
  if (db_query_init(&qry)) {
    /* error */
    log_stderr(LOG_ERROR, "db_query init failed");
  } else {
    qry->q_type = DB_INSERT;
    qry->payload_t = payload_t;
    qry->pl_ctx = pl_ctx;

    if (pl_ctx) {
      if ((ret = db_insert_query(db_tran, qry))) {
        log_stderr(LOG_ERROR, "db INSERT query failed");
      } else {
        /* result processing */
        if (qry->r_count) {
          switch (payload_t) {
            case SS_DEV_CONFIG:
              break;
            case SS_SENSE_CONFIG:
              break;
            case SS_SS_CFG:
              break;
            case SS_READING:;
              struct reading *r = (struct reading *)pl_ctx;
              r->readingID = qry->insertID;
              break;
          }
        }
      }
    }
  }

  /* cleanup */
  free_db_query(qry);

  return ret;
}

/**
 * \brief Function to run "SELECT" db query - get transport
 * \param ctx Transport context struct
 * \param payload_t type of payload passed in pl_ctx
 * \param pl_ctx Payload context struct 
 * \param size pointer to the size of the returned data - pl_ctx dependent
 */
int db_get_method(struct db_transport *db_tran, trn_payload_t payload_t, void *pl_ctx,
    int *size) {

  int ret = SS_SUCCESS;

  /* build query */ 
  struct db_query *qry;
  if (db_query_init(&qry)) {
    log_stderr(LOG_ERROR, "db_query init failed");
  } else {
    qry->q_type = DB_SELECT;
    qry->payload_t = payload_t;
    qry->pl_ctx = pl_ctx;

    if (pl_ctx) {
      if ((ret = db_select_query(db_tran, qry))) {
        /* error */
        log_stderr(LOG_ERROR, "db SELECT query failed");
      } else {
        *size = qry->r_count;

        /* result processing */
        if (qry->r_count) {
          switch (payload_t) {
            case SS_DEV_CONFIG:
              break;
            case SS_SENSE_CONFIG:
              break;
            case SS_SS_CFG:
              break;
            case SS_READING:
              break;

          }
        } else {
          ret = SS_GET_EMPTY;
        }
      }
    }
  }

  /* cleanup */
  free_db_query(qry);

  return ret;
}

/**
 * \brief Function to query db database
 */
int db_query_db(struct db_transport *trn, struct db_query *qry) {

  int ret = SS_SUCCESS;

  /* start of CRITICAL SECTION */
  while (pthread_mutex_lock(&trn->db_mutex)) {
    log_stdout(LOG_DEBUG_THREAD, "DB thread Block");
  }

  log_stdout(LOG_DEBUG, "Query: \n\t%s", qry->query);

  switch (trn->engine) {
    case DB_MYSQL:
      ret = db_mysql_query(trn, qry);
      if (ret) {
        log_stderr(LOG_ERROR, "db_mysql_query failed: \n\t%d", ret);

        ret = db_tran_reconnect(trn);
        if ((ret == SS_CONN_RECONNECT )) {
          /* reconnected to database, rerun query */
          ret = db_mysql_query(trn, qry);
        } 
      }
      break;

    case DB_SQLITE:
      ret = db_sqlite_query(trn, qry);
      if (ret) {
        log_stderr(LOG_ERROR, "db_sqlite_query failed: \n\t%d", ret);
        if ((ret = db_tran_reconnect(trn))) {
          /* database connection failed */
        } else {
          ret = db_sqlite_query(trn, qry);
        }
      }
      break;

    case DB_NO_ENGINE:
      break;
  }

  /* end of CRITICAL SECTION */
  pthread_mutex_unlock(&trn->db_mutex);
  return ret;
}

/** 
 * \brief create new result of type struct reading
 */
int db_new_reading_result(struct db_query *qry) {
  int ret = SS_SUCCESS;

  if (qry->r_count < DB_MAX_NO_RESULTS) {
    if (!(ret = reading_init((struct reading **)
            &qry->result[qry->r_count]))) {
      /* Limitation - only one measurement per result */
      ret = measurement_init((struct reading *)
          qry->result[qry->r_count++]);
    }
  } else {
    log_stderr(LOG_WARN, "DB returned more results than expected");
    ret = SS_BUF_FULL;
  }

  return ret;
}

/**
 * \brief function to close database connection
 */
void db_close_method(struct db_transport *db_tran) {

  /* start of CRITICAL SECTION */
  while (pthread_mutex_lock(&db_tran->db_mutex)) {
    log_stdout(LOG_DEBUG_THREAD, "DB thread Block");
  }

  switch (db_tran->engine) {
    case DB_MYSQL:
      db_mysql_close(db_tran);
      break;

    case DB_SQLITE:
      db_sqlite_close(db_tran);
      break;

    case DB_NO_ENGINE:
      break;
  }

  /* end of CRITICAL SECTION */
  pthread_mutex_unlock(&db_tran->db_mutex);

  return;
}

void free_db_query(struct db_query *q) {
  if (q) {
    free(q->query);

    if(q->my_result) {
      mysql_free_result(q->my_result);
    }

    int i;
    if (q->q_type == DB_SELECT) {
      switch (q->payload_t) {
        case SS_DEV_CONFIG:
          break;
        case SS_SENSE_CONFIG:
          break;
        case SS_SS_CFG:
          break;
        case SS_READING:
          for (i=0; i < q->r_count; i++) {
            struct reading *r = (struct reading *)q->result[i];
            free_reading(r);
          }

          break;
      }
    }

    free(q);
  }
  return;
}

void free_db_tran(struct db_transport *db_tran) {
  if (db_tran) {
    log_stdout(LOG_DEBUG, "Freeing db transport: %s", db_tran->db);
    free(db_tran->db);
    free(db_tran->db_user);
    free(db_tran->db_pass);
    free(db_tran->db_host);
    free(db_tran);
  }
  return;
}
