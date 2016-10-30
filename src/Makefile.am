SUBDIRS = reading serial uMQTT

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
	          -IuMQTT/src/inc \
	          -Ireading \
	          -Iserial
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
	          -IuMQTT/src/inc \
	          -Ireading \
	          -Iserial
endif


AM_LDFLAGS = libreading.a \
             libserial.a \
             -LuMQTT/lib \
             -luMQTT_client \
             -luMQTT_linux_client \
             -luMQTT_helper \
             -luMQTT \
             -lrrd


lib_LIBRARIES = libreading.a libserial.a

if RRD_H
RRDTOOL = reading/reading_rrdtool.c
endif

libreading_a_SOURCES = reading/reading.c reading/reading_ini.c \
                        reading/reading_json.c reading/reading_cc_dev.c \
                        $(RRDTOOL) log.c

libserial_a_SOURCES = serial/tty_conn.c log.c


bin_PROGRAMS = tty_mqtt reading_mqtt

reading_mqtt_SOURCES = reading_mqtt.c log.c
reading_mqtt_LDADD = $(AM_LDFLAGS)

tty_mqtt_SOURCES = tty_mqtt.c log.c
tty_mqtt_LDADD = $(AM_LDFLAGS)
