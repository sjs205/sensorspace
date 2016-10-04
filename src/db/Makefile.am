# my_global.h includes functions without prototypes, hence this
# eliminates the "function declaration isn’t a prototype" error.
MYSQL_HACK = -Wno-error=strict-prototypes

if DEBUG
AM_CFLAGS = -g3 -O0 \
						-Wall \
						-Werror \
						-Wmissing-declarations \
						-Wmissing-prototypes \
						-Wnested-externs \
				 		-Wpointer-arith \
						-Wsign-compare \
						-Wchar-subscripts \
						-Wstrict-prototypes \
						-Wwrite-strings \
						-Wshadow \
						-Wformat-security \
						-Wtype-limits \
	          -I../readings \
						$(MYSQL_HACK) \
						$(MYSQL_CONFIG_CFLAGS)
else
AM_CFLAGS = -Wall \
						-Werror \
						-Wmissing-declarations \
						-Wmissing-prototypes \
						-Wnested-externs \
				 		-Wpointer-arith \
						-Wsign-compare \
						-Wchar-subscripts \
						-Wstrict-prototypes \
						-Wwrite-strings \
						-Wshadow \
						-Wformat-security \
						-Wtype-limits \
	          -I../readings \
						$(MYSQL_HACK) \
						$(MYSQL_CONFIG_CFLAGS)
endif

AM_LDFLAGS = \
             -LuMQTT/lib \
             -luMQTT \
             -luMQTT_client \
             -luMQTT_helper \
             -luMQTT_linux_client \
	           -lreadings \
						 $(MYSQL_CONFIG_LIBS) \
						 -lsqlite3 \
						 -lpthread

lib_LIBRARIES = libdb.a

libdb_a_SOURCES db.c
