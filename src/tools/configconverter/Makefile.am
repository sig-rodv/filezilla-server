bin_PROGRAMS = filezilla-server-config-converter
    
SOURCES_H = \
    converters.hpp \
    old_config.hpp \
    util.hpp
    
filezilla_server_config_converter_SOURCES = \
    $(SOURCES_H) \
    converters.cpp \
    main.cpp

filezilla_server_config_converter_CXXFLAGS = -pthread -fno-exceptions $(LIBFILEZILLA_CFLAGS) 

if FZ_WINDOWS
    filezilla_server_config_converter_LDFLAGS = -municode
    EXTRA_LIBS =
endif

filezilla_server_config_converter_LDADD    = ../../filezilla/libfilezilla-common.a $(EXTRA_LIBS) $(LIBFILEZILLA_LIBS) $(PUGIXML_LIBS)

