noinst_PROGRAMS = \
    administration_client/administration_client \
    echo/echo \
    filetransfer/filetransfer \
    httpget/httpget
    
administration_client_administration_client_SOURCES = \
    administration_client/administration_client.cpp
    
echo_echo_SOURCES = \
    echo/echo.cpp

filetransfer_filetransfer_SOURCES = \
    filetransfer/filetransfer.cpp
    
httpget_httpget_SOURCES = \
    httpget/httpget.cpp

AM_CXXFLAGS = $(LIBFILEZILLA_CFLAGS) $(WX_CXXFLAGS) -fno-exceptions
LIBS     = ../src/filezilla/libfilezilla-common.a $(LIBFILEZILLA_LIBS) $(PUGIXML_LIBS) $(EXTRA_LIBS)

 
