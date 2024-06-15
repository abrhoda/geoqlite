cmake_minimum_required(VERSION 3.11)
set(PROJECT_VERSION 0.1.0)
project(geoqlite LANGUAGES C VERSION ${PROJECT_VERSION} DESCRIPTION "an embeddable goespatial and geofencing database")

string(TOLOWER "${PROJECT_NAME}" PROJECT_NAME_LOWER)
option(WITH_UNIT_TESTING "Build the test executable and link to the lib to allow for unit tests to be run" OFF)
option(WITH_DEBUG "Build with `-Werror -fsanitize=undefined -fsanitize=address` flags" OFF)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Werror -Wextra")
if (WITH_DEBUG)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -ggdb -fno-omit-frame-pointer -fsanitize=address -fsanitize=undefined")
endif()

message("Compilation flags being used = ${CMAKE_C_FLAGS}")

set(INCLUDE_LIST
  include/stringutils.h
  include/geometry.h
  include/parse.h
  )

set(SRC_LIST 
  ${INCLUDE_LIST}
  src/stringutils.c
  src/geometry.c
  src/parse.c
  src/cli.c
)


configure_file(geoqlite.h.in geoqlite.h)
add_executable(geoqlite ${SRC_LIST})
target_include_directories(geoqlite PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include ${PROJECT_BINARY_DIR})
