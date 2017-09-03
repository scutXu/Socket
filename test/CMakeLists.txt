project(echo_server)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

include_directories (
	../src
)

aux_source_directory (
	../ src SOCKET_LIBRARY
)

set(SOURCES_FILES ${SOCKET_LIBRARY} echo_server.cpp)

add_executable(echo_server ${ SOURCE_FILES })