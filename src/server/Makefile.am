bin_PROGRAMS = filezilla-server
    
SOURCES_H = \
	administration.hpp \
	administrator.hpp \
	administrator/debug.hpp \
	administrator/notifier.hpp \
	administrator/update_checker.hpp \
	legacy_options.hpp \
	server_config_paths.hpp \
	server_settings.hpp

filezilla_server_SOURCES = \
    $(SOURCES_H) \
	administrator.cpp \
	administrator/acme.cpp \
	administrator/admin_options.cpp \
	administrator/certificates.cpp \
	administrator/ftp_options.cpp \
	administrator/groups_and_users.cpp \
	administrator/ip_filters.cpp \
	administrator/logger_options.cpp \
	administrator/notifier.cpp \
	administrator/protocol_options.cpp \
	administrator/update_checker.cpp \
	main.cpp \
	server_settings.cpp

filezilla_server_CXXFLAGS = -pthread -fno-exceptions $(LIBFILEZILLA_CFLAGS) 

if FZ_WINDOWS
    filezilla_server_LDFLAGS = -municode
    EXTRA_LDADD = $(top_builddir)/res/filezilla-server.o
    EXTRA_LIBS =
endif

filezilla_server_LDADD = $(EXTRA_LDADD) ../filezilla/libfilezilla-common.a $(EXTRA_LIBS) $(LIBFILEZILLA_LIBS) $(PUGIXML_LIBS)


