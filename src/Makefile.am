SUBDIRS = reading serial controller uMQTT

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
	          -Iserial \
	          -Icontroller
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
	          -Iserial \
	          -Icontroller
endif


AM_LDFLAGS = libreading.a \
             libserial.a \
             libcontroller.a \
             -LuMQTT/lib \
             -luMQTT_client \
             -luMQTT_linux_client \
             -luMQTT_helper \
             -luMQTT \
             -lrrd

lib_LIBRARIES = libreading.a libserial.a libcontroller.a

if RRD_H
RRDTOOL = reading/reading_rrdtool.c
endif

libreading_a_SOURCES = reading/reading.c reading/reading_ini.c \
                        reading/reading_json.c reading/reading_cc_dev.c \
                        $(RRDTOOL) log.c

libserial_a_SOURCES = serial/tty_conn.c log.c
libcontroller_a_SOURCES = controller/pid.c log.c

bin_PROGRAMS = tty_mqtt reading_mqtt pid_mqtt $(RRDTOOL_BIN)

pid_mqtt_SOURCES = pid_mqtt.c log.c
pid_mqtt_LDADD = $(AM_LDFLAGS)

reading_mqtt_SOURCES = reading_mqtt.c log.c
reading_mqtt_LDADD = $(AM_LDFLAGS)

tty_mqtt_SOURCES = tty_mqtt.c log.c
tty_mqtt_LDADD = $(AM_LDFLAGS)

if RRD_H
RRDTOOL_BIN = mqtt_rrdtool
mqtt_rrdtool_SOURCES = mqtt_rrdtool.c log.c
mqtt_rrdtool_LDADD = $(AM_LDFLAGS)
endif
