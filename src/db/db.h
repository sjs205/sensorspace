#ifndef DB_TRANSPORT__H
#define DB_TRANSPORT__H
/******************************************************************************
 * File: db_transports.h
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
#include <pthread.h>
#include <my_global.h>
#include <mysql.h>
#include <sqlite3.h>

#include "sensorspace.h"
#include "reading.h"

#define DB_QRY_PEND_MAX          16
#define DB_QRY_MAX_LEN           1024
#define DB_SS_CFG_HANDLE        (const char *)"TRANSPORT_DB"
#define DB_STRING_MAX         SS_NAME_STR_MAX
#define DB_HOST_STRING_MAX    32
#define DB_USER_STRING_MAX    32
#define DB_PASS_STRING_MAX    32
#define DB_PASS_STRING_MAX    32

#define DB_MAX_NO_RESULTS       2048

/* standard query defines */
#define DB_DEV_CONFIG_QRY "SELECT * FROM Devices"

typedef enum {
  DB_NO_ENGINE,

  DB_MYSQL,
  DB_SQLITE,
} db_engine_t;

typedef enum {
  DB_SELECT,
  DB_INSERT,
  DB_DELETE,
  DB_UPDATE,
} query_t;

typedef enum {
  QRY_PENDING,
  QRY_SUCCESS,
  QRY_ERROR,
} qry_status;

/**
 * \brief Structure to store database query and result
 * \param query Query in string form
 * \param q_type Type of query, e.g.: SELECT, INSERT
 * \param pl_ctx pointer to the struct sensor that the query relates to
 * \param result void pointer to the result as defined in payload_t
 * \param r_count number of results returned
 */
struct db_query {
  char *query;
  int qry_len;
  query_t q_type;
  char *condition;

  void *pl_ctx;

  int insertID;
  int status;

  trn_payload_t payload_t;

  /* MySQL variables */
  MYSQL_RES *my_result;
  MYSQL_ROW row;

  /* sqlite variables */
  sqlite3_stmt *sqlite_pre_state;

  void *result[DB_MAX_NO_RESULTS];
  int r_count;

  int num_fields;
  int err;
};

struct db_transport {
  db_engine_t engine;

  char *db;
  char *db_user;
  char *db_pass;
  char *db_host;

  MYSQL *mysql_conn;
  sqlite3 *sqlite_conn;

  unsigned long mysql_threadID;

  struct db_query* qry[DB_QRY_PEND_MAX];

  pthread_mutex_t db_mutex;
};

int convert_db_engine_str(char *db_engine);

int db_tran_file_config(struct db_transport *db_trans, char *buf, unsigned int size);

int db_post_method(struct db_transport *db_trans, trn_payload_t payload_t, void *pl_ctx);
int db_get_method(struct db_transport *db_trans, trn_payload_t payload_t, void *pl_ctx,
    int *size);

int db_query_init(struct db_query **q_p);
int db_insert_query(struct db_transport *trn, struct db_query *qry);
int db_select_query(struct db_transport *trn, struct db_query *qry);
int db_query_db(struct db_transport *trn, struct db_query *qry);
int db_new_reading_result(struct db_query *qry);
void free_db_query(struct db_query *q);

int db_tran_init(struct db_transport **db);
int db_tran_connect(struct db_transport *db_trans);
void db_close_method(struct db_transport *db_trans);
void free_db_tran(struct db_transport *db_trans);
void db_free_ctx(struct db_transport *db_trans);

/* db_mysql.c */
int db_mysql_open(struct db_transport *db_tran);
int db_mysql_reconnect(struct db_transport *db_tran);
int db_mysql_query(struct db_transport *trn, struct db_query *qry);
int db_mysql_proc_result(struct db_query *qry);
void db_mysql_close(struct db_transport *db_tran);

/* db_sqlite.c */
int db_sqlite_open(struct db_transport *db_tran);
int db_sqlite_query(struct db_transport *trn, struct db_query *qry);
int db_sqlite_proc_result(struct db_query *qry);
void db_sqlite_close(struct db_transport *db_tran);

#endif /* DB_TRANSPORT__H */
