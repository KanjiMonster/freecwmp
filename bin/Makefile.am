bin_PROGRAMS = freecwmpd

freecwmpd_SOURCES =		\
	../src/freecwmp.h	\
	../src/freecwmp.c	\
	../src/config.h		\
	../src/config.c		\
	../src/cwmp/cwmp.h	\
	../src/cwmp/cwmp.c	\
	../src/cwmp/external.h	\
	../src/cwmp/external.c	\
	../src/cwmp/device.h	\
	../src/cwmp/device.c	\
	../src/http/b64.h	\
	../src/http/b64.c	\
	../src/http/http.h	\
	../src/http/http.c	\
	../src/mix/time.h	\
	../src/mix/time.c	\
	../src/ubus/ubus.h	\
	../src/ubus/ubus.c	\
	../src/xml/xml.h	\
	../src/xml/xml.c

freecwmpd_CFLAGS =		\
	$(AM_CFLAGS)		\
	$(LIBFREECWMP_CFLAGS)	\
	$(LIBUCI_CFLAGS)	\
	$(LIBUBOX_CFLAGS)	\
	$(LIBUBUS_CFLAGS)	\
	$(MICROXML_CFLAGS)	\
	$(LIBCURL_CFLAGS)	\
	$(LIBZSTREAM_CFLAGS)

freecwmpd_LDFLAGS =		\
	$(AM_LDFLAGS)		\
	$(LIBFREECWMP_LDLAGS)	\
	$(LIBUCI_LDFLAGS)	\
	$(LIBUBOX_LDFLAGS)	\
	$(LIBUBUS_LDFLAGS)	\
	$(MICROXML_LDFLAGS)	\
	$(LIBCURL_LDFLAGS)	\
	$(LIBZSTREAM_CFLAGS)

freecwmpd_LDADD =		\
	$(AM_LIBS)		\
	$(LIBFREECWMP_LIBS)	\
	$(LIBUCI_LIBS)		\
	$(LIBUBOX_LIBS)		\
	$(LIBUBUS_LIBS)		\
	$(MICROXML_LIBS)	\
	$(LIBCURL_LIBS)		\
	$(LIBZSTREAM_LIBS)

