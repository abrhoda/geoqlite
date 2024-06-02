cmake_minimum_required(VERSION 3.10)
set(PROJECT_VERSION 0.1.0)
project(geoqlite LANGUAGES C VERSION ${PROJECT_VERSION} DESCRIPTION "an embeddable goespatial and geofencing database")

string(TOLOWER "${PROJECT_NAME}" PROJECT_NAME_LOWER)
option(BUILD_SHARED_LIBS "Build shared libraries (.dll/.so) instead of static ones (.lib/.a)" ON)
option(WITH_UNIT_TESTING "Build the test executable and link to the lib to allow for unit tests to be run" OFF)
option(WITH_DEBUG "Build with `-Werror -fsanitize=undefined -fsanitize=address` flags" OFF)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Werror -Wextra")
if (WITH_DEBUG)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -ggdb -fno-omit-frame-pointer -fsanitize=address -fsanitize=undefined")
endif()

message("Compilation flags being used = ${CMAKE_C_FLAGS}")

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/lib/include)

set(INCLUDE_LIST
  lib/include/parser.h
  )

set(SRC_LIST 
  ${INCLUDE_LIST}
  lib/src/parser.c
)

if (BUILD_SHARED_LIBS) 
  set(LIBRARY_NAME ${PROJECT_NAME_LOWER}-${PROJECT_VERSION})
else ()
  set(LIBRARY_NAME ${PROJECT_NAME_LOWER}-static-${PROJECT_VERSION})
endif ()

set(DEFAULT_LIBRARY_INSTALL_DIRECTORY "lib")

# Allow user supplied lib install dir or take the default
if(NOT LIBRARY_INSTALL_DIRECTORY)
    set(LIBRARY_INSTALL_DIRECTORY ${DEFAULT_LIBRARY_INSTALL_DIRECTORY})
endif()

# Add output library: add_library(libname sourcelist) respects BUILD_SHARED_LIBS
add_library(${LIBRARY_NAME} ${SRC_LIST})
message("Built library ${LIBRARY_NAME}")

if (WITH_UNIT_TESTING)
  enable_testing()
  list(APPEND parser)

  foreach(action ${actions})
    # Testing executable
    add_executable("${action}-test" "test/${action}.c" test/testing_utils.h test/testing_utils.c)
    target_link_libraries("${action}-test" PRIVATE ${LIBRARY_NAME})
    message("Added executeable ${action}-test and linked to ${LIBRARY_NAME}")

    # ctest 
    add_test(NAME "${action}-test" COMMAND "${action}-test")
  endforeach()
endif()

# Install the shared library file
install(
  TARGETS ${LIBRARY_NAME}
  DESTINATION ${LIBRARY_INSTALL_DIRECTORY}
)
