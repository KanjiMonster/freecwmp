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
	../src/cwmp/acs.h	\
	../src/cwmp/acs.c	\
	../src/cwmp/local.c	\
	../src/cwmp/local.h	\
	../src/cwmp/device.h	\
	../src/cwmp/device.c	\
	../src/http/http.h	\
	../src/http/http.c	\
	../src/xml/xml.h	\
	../src/xml/xml.c	\
	../src/mix/time.h	\
	../src/mix/time.c

freecwmpd_CFLAGS =		\
	$(AM_CFLAGS)		\
	$(LIBUCI_CFLAGS)	\
	$(LIBUBOX_CFLAGS)	\
	$(MICROXML_CFLAGS)	\
	$(LIBCURL_CFLAGS)	\
	$(LIBZSTREAM_CFLAGS)

freecwmpd_LDFLAGS =		\
	$(AM_LDFLAGS)		\
	$(LIBUCI_LDFLAGS)	\
	$(LIBUBOX_LDFLAGS)	\
	$(MICROXML_LDFLAGS)	\
	$(LIBCURL_LDFLAGS)	\
	$(LIBZSTREAM_CFLAGS)

freecwmpd_LDADD =		\
	$(AM_LIBS)		\
	$(LIBUCI_LIBS)		\
	$(LIBUBOX_LIBS)		\
	$(MICROXML_LIBS)	\
	$(LIBCURL_LIBS)		\
	$(LIBZSTREAM_LIBS)

