/******************************************************************************
 * File: db_mysql.c
 * Description: functions to provide mysql database interface
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
#include <my_global.h>
#include <mysql.h>

#include "log.h"
#include "db.h"

/**
 * \brief function to open database connection
 */
int db_mysql_open(struct db_transport *db_tran) {
  log_stdout(LOG_INFO, "Attempting to connect to: %s@%s", db_tran->db,
      db_tran->db_host);

  db_tran->mysql_conn = mysql_init(NULL);
  if (db_tran->mysql_conn == NULL) {
    log_stderr(LOG_ERROR, "%u: %s", mysql_errno(db_tran->mysql_conn),
        mysql_error(db_tran->mysql_conn));
    return SS_INIT_ERROR;
  }

  /* set db reconnect using mysql_ping */
  const char reconnect = 1;
  if (mysql_options(db_tran->mysql_conn, MYSQL_OPT_RECONNECT, &reconnect)) {
    log_stderr(LOG_ERROR,  "%u: %s", mysql_errno(db_tran->mysql_conn),
        mysql_error(db_tran->mysql_conn));
  }

  /* connect to database */
  if(mysql_real_connect(db_tran->mysql_conn,
        db_tran->db_host, db_tran->db_user,
        db_tran->db_pass, db_tran->db,0, NULL, 0) == NULL) {
    log_stderr(LOG_ERROR,  "%u: %s", mysql_errno(db_tran->mysql_conn),
        mysql_error(db_tran->mysql_conn));
    return SS_INIT_ERROR;
  }

  /* get database threadid since we can use this to determine a reconnect */
  db_tran->mysql_threadID = mysql_thread_id(db_tran->mysql_conn);

  return SS_SUCCESS;
}

/**
 * \brief function to check db connection and reconnect if failed
 */
int db_mysql_reconnect(struct db_transport *db_tran) {

  unsigned long cur_mysql_threadID;
  log_stdout(LOG_INFO, "Checking connection to: %s@%s", db_tran->db,
      db_tran->db_host);

  if (db_tran->mysql_conn == NULL) {
    log_stderr(LOG_ERROR, "%u: %s", mysql_errno(db_tran->mysql_conn),
        mysql_error(db_tran->mysql_conn));
    return SS_INIT_ERROR;
  }

  /* check db connection and attempt to reconnect */
  if(mysql_ping(db_tran->mysql_conn)) {
    log_stderr(LOG_ERROR,  "%u: %s", mysql_errno(db_tran->mysql_conn),
        mysql_error(db_tran->mysql_conn));
    return SS_CONN_ERROR;
  }

  /* getting current threadID to determin whether reconnect occurs */
  cur_mysql_threadID = mysql_thread_id(db_tran->mysql_conn);
  if (db_tran->mysql_threadID != cur_mysql_threadID) {
    db_tran->mysql_threadID = cur_mysql_threadID;
    log_stdout(LOG_INFO, "Reconnected to databse");
    return SS_CONN_RECONNECT;
  } else {
    log_stdout(LOG_INFO, "Connection okay");
  }

  return SS_SUCCESS;
}

/**
 * \brief function to process result returned from mysql db
 * \param qry the query struct the result relates to
 */
static int db_mysql_proc_reading_result(struct db_query *qry) {

  int ret = SS_SUCCESS;

  /*
  struct sensor *sens = (struct sensor *) qry->pl_ctx;

  if (db_new_reading_result(qry)) {
    log_stderr(LOG_ERROR, "Error allocating result");
    ret = SS_OUT_OF_MEM_ERROR;
  } else {
    struct reading *r = (struct reading *)(qry->result[qry->r_count -1]);
    r->readingID = atoi(qry->row[0] ? qry->row[0] : "NULL");
    r->dev  = sens->device; 
    if (!strptime((char *)(qry->row[2] ? qry->row[2] : "NULL"),
          "%Y-%m-%d %H:%M:%S", &r->t)) {

      log_stderr(LOG_ERROR, "Could not convert database time return to struct tm");
    }
    r->meas[r->count - 1]->sensorID = sens->sensorID;
    strcpy(r->meas[r->count - 1]->meas,
        (char *)(qry->row[5] ? qry->row[5] : "NULL"));

    * log *
    int i;
    log_stdout(LOG_DEBUG, "reading result:");
    for (i = 0; i < qry->num_fields; i++) {
      log_stdout(LOG_DEBUG, "%s ", qry->row[i] ? qry->row[i] : "NULL");
    }
    log_stdout(LOG_DEBUG, "\n");
  }

  */
  return ret;
}

/**
 * \brief function to process result returned from mysql db
 * \param qry the query struct the result relates to
 */
int db_mysql_proc_result(struct db_query *qry) {

  int ret = SS_SUCCESS;

  switch (qry->payload_t) {
    case SS_DEV_CONFIG:
      break;
    case SS_SENSE_CONFIG:
      break;
    case SS_SS_CFG:
      break;
    case SS_READING:
      ret = db_mysql_proc_reading_result(qry);
      break;
  }

  return ret;
}

int db_mysql_query(struct db_transport *trn, struct db_query *qry) {

  int ret = SS_SUCCESS;

  /* execute query */
  if (mysql_real_query(trn->mysql_conn, qry->query, qry->qry_len)) {
    log_stderr(LOG_ERROR, "%u: %s", mysql_errno(trn->mysql_conn),
        mysql_error(trn->mysql_conn));
    qry->err = mysql_errno(trn->mysql_conn);
    ret = SS_POST_ERROR;
  }

  /* check query status - store result and field count */
  qry->my_result = mysql_store_result(trn->mysql_conn);
  if (qry->my_result) {
    /* query returned rows */
    qry->num_fields  = mysql_num_fields(qry->my_result);
    while ((qry->row = mysql_fetch_row(qry->my_result)))
    {
      if ((ret = db_mysql_proc_result(qry))) {
        log_stderr(LOG_ERROR, "processing db result");
        ret = SS_GET_ERROR;
        break;
      }
    }

  } else {
    /* query returned nothing */
    if (mysql_field_count(trn->mysql_conn) == 0) {
      /* was not a select query - or poss no result */
      if (mysql_insert_id(trn->mysql_conn) != 0) {
        qry->insertID = mysql_insert_id(trn->mysql_conn);
      }

    } else {
      log_stderr(LOG_ERROR, "%u: %s", mysql_errno(trn->mysql_conn),
          mysql_error(trn->mysql_conn));
      ret = SS_GET_ERROR;
    }
  }
  return ret;
}

/**
 * \brief Function to close database connection
 */
void db_mysql_close(struct db_transport *db_tran) {

  log_stdout(LOG_INFO, "Closing MySQL db connection: %s", db_tran->db);
  mysql_close(db_tran->mysql_conn);

  mysql_library_end();

  return;
}
