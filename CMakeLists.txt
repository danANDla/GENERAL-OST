build_lib(
  LIBNAME ost
  SOURCE_FILES
    model/ost_node.c
    model/ost_socket.c
    model/ost-header.c
    model/timer_fifo.c
  HEADER_FILES
	model/ost_node.h
	model/ost-header.h
  LIBRARIES_TO_LINK ${core} 
  TEST_SOURCES 
	test/ost-main-test.cc
)

