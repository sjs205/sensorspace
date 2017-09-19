SUBDIRS = reading serial controller actuator ssdisplay uMQTT

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
	          -Icontroller\
	          -Iactuator \
            -Issdisplay
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
	          -Icontroller \
	          -Iactuator \
            -Issdisplay
endif


AM_LDFLAGS = libreading.a \
             libserial.a \
             libcontroller.a \
             libactuator.a \
             libssdisplay.a \
             -LuMQTT/lib \
             -luMQTT_client \
             -luMQTT_linux_client \
             -luMQTT_helper \
             -luMQTT

AM_LDFLAGS += @PLATFORM_LIBS@

AM_CPPFLAGS = @PLATFORM_CPPFLAGS@

lib_LIBRARIES = libreading.a libserial.a libcontroller.a libactuator.a libssdisplay.a

libreading_a_SOURCES = reading/reading.c reading/reading_ini.c \
                        reading/reading_json.c reading/reading_cc_dev.c log.c
libserial_a_SOURCES = serial/tty_conn.c log.c
libcontroller_a_SOURCES = controller/pid.c log.c
libactuator_a_SOURCES = actuator/pi_pin_actuator.c log.c
libssdisplay_a_SOURCES = ssdisplay/74hc595_7_seg.c log.c

bin_PROGRAMS = tty_mqtt reading_mqtt pid_mqtt actuator_mqtt display_mqtt ds18b20_mqtt

actuator_mqtt_SOURCES = actuator_mqtt.c log.c
actuator_mqtt_LDADD = $(AM_LDFLAGS)

pid_mqtt_SOURCES = pid_mqtt.c log.c
pid_mqtt_LDADD = $(AM_LDFLAGS)

reading_mqtt_SOURCES = reading_mqtt.c log.c
reading_mqtt_LDADD = $(AM_LDFLAGS)

tty_mqtt_SOURCES = tty_mqtt.c log.c
tty_mqtt_LDADD = $(AM_LDFLAGS)

display_mqtt_SOURCES = display_mqtt.c log.c
display_mqtt_LDADD = $(AM_LDFLAGS)

ds18b20_mqtt_SOURCES = ds18b20_mqtt.c log.c
ds18b20_mqtt_LDADD = $(AM_LDFLAGS)
