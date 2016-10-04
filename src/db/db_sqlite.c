/******************************************************************************
 * File: db_sqlite.c
 * Description: functions to provide sqlite database interface
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
#include <sqlite3.h>

#include "log.h"
#include "db.h"

int db_sqlite_open(struct db_transport *db_tran) {
  log_stdout(LOG_INFO, "Attempting to connect to: %s", db_tran->db);

  if (sqlite3_open((const char *)db_tran->db, &db_tran->sqlite_conn)) {
    log_stderr(LOG_ERROR, "%s", sqlite3_errmsg(db_tran->sqlite_conn));
    return SS_INIT_ERROR;
  }
  return SS_SUCCESS;
}

/**
 * \brief function to process result returned from sqlite db
 * \param qry the query struct the result relates to
 */
static int db_sqlite_proc_reading_result(struct db_query *qry) {

  int ret = SS_SUCCESS;

  /*
  struct sensor *sens = (struct sensor *) qry->pl_ctx;

  if (db_new_reading_result(qry)) {
    log_stderr(LOG_ERROR, "Allocating result");
    ret = SS_OUT_OF_MEM_ERROR;
  } else {
    struct reading *r = (struct reading *)(qry->result[qry->r_count - 1]);
    r->readingID = sqlite3_column_int(qry->sqlite_pre_state, 0);
    r->dev  = sens->device; 

    if (convert_db_date_to_tm(
          (char *)sqlite3_column_text(qry->sqlite_pre_state, 2),
          &r->t)) {
      * error *
      log_stderr(LOG_ERROR, 
          "Could not convert database time return to struct tm");
    }

    r->meas[r->count - 1]->sensorID = sens->sensorID;
    strcpy(r->meas[r->count - 1]->meas,
        (char *)sqlite3_column_text(qry->sqlite_pre_state, 5));

    * log *
    int i;
    log_stdout(LOG_DEBUG, "reading result:");
    for (i = 0; i <= qry->num_fields; i++) {
      log_stdout(LOG_DEBUG, "%s ",
          sqlite3_column_text(qry->sqlite_pre_state, i));
    }
    log_stdout(LOG_DEBUG, "");
  }
  */

  return ret;
}

/**
 * \brief function to process result returned from sqlite db
 * \param qry the query struct the result relates to
 */
int db_sqlite_proc_result(struct db_query *qry) {

  int ret = SS_SUCCESS;

    switch (qry->payload_t) {
      case SS_DEV_CONFIG:
        break;
      case SS_SENSE_CONFIG:
        break;
      case SS_SS_CFG:
        break;
      case SS_READING:
        ret = db_sqlite_proc_reading_result(qry);
        break;
    }

  return ret;
}

/**
 * \brief process sqlite query
 * \param trn the database transport the query relates to
 * \param qry query to process
 */
int db_sqlite_query(struct db_transport *trn, struct db_query *qry) {

  int ret = SS_SUCCESS;

  /* prepare query */
  if ((ret = sqlite3_prepare_v2(trn->sqlite_conn, qry->query,
          qry->qry_len, &qry->sqlite_pre_state, NULL))) {
    /* error */
    log_stderr(LOG_ERROR, "%s", sqlite3_errmsg(trn->sqlite_conn));
    ret = SS_POST_ERROR;

  } else {
    log_stdout(LOG_DEBUG, "sqlite query prepared");

    if (!qry->sqlite_pre_state) {
      /* no sql statment */
      log_stderr(LOG_ERROR, "sql string does not contain any sql:\n%s",
        qry->query);
      ret = SS_POST_ERROR;

    } else {

      qry->num_fields = sqlite3_column_count(qry->sqlite_pre_state);
      /* execute query */
      while ((ret = sqlite3_step(qry->sqlite_pre_state)) == SQLITE_ROW)
      {
        /* debug */
        log_stdout(LOG_DEBUG, "sqlite query return code: %d", ret);
        /* check query status - store result and field count */
        if (ret == SQLITE_ROW && db_sqlite_proc_result(qry)) {
          log_stderr(LOG_ERROR, "Processing result");
          break;
        }
      }

      if (ret !=  SQLITE_OK && ret != SQLITE_ROW && ret != SQLITE_DONE) {
        log_stderr(LOG_ERROR, "%s", sqlite3_errmsg(trn->sqlite_conn));
        ret = SS_GET_ERROR;
      } else if (ret == SQLITE_DONE) {
        /* query returned nothing - or poss no result */
        if (qry->q_type == DB_INSERT &&
            sqlite3_last_insert_rowid(trn->sqlite_conn) != 0) {
          qry->insertID = sqlite3_last_insert_rowid(trn->sqlite_conn);
        }
        ret = SS_SUCCESS;

      } else if (ret ==  SQLITE_OK) {
        ret = SS_SUCCESS;
      }
    }
    /* cleanup */
    sqlite3_finalize(qry->sqlite_pre_state);
  }

  return ret;
}

/**
 * \brief Function to close database connection
 */
void db_sqlite_close(struct db_transport *db_tran) {

  log_stdout(LOG_INFO, "Closing sqlite db connection: %s", db_tran->db);
  sqlite3_close(db_tran->sqlite_conn);

  return;
}
