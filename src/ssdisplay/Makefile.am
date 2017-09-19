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
            -I..
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
            -I..
endif

AM_LDFLAGS = @PLATFORM_LIBS@

AM_CPPFLAGS = @PLATFORM_CPPFLAGS@

lib_LIBRARIES = libssdisplay.a

libssdisplay_a_SOURCES = 74hc595_7_seg.c
