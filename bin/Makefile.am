bin_PROGRAMS = freecwmpd

freecwmpd_SOURCES =		\
	../src/b64.h		\
	../src/b64.c		\
	../src/config.h		\
	../src/config.c		\
	../src/cwmp.h		\
	../src/cwmp.c		\
	../src/external.h	\
	../src/external.c	\
	../src/freecwmp.h	\
	../src/freecwmp.c	\
	../src/http.h		\
	../src/http.c		\
	../src/time.h		\
	../src/time.c		\
	../src/ubus.h		\
	../src/ubus.c		\
	../src/xml.h		\
	../src/xml.c

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

