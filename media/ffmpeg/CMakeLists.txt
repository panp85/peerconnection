########################################################################
# Prevent in-tree builds
########################################################################
if(${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})
    message(FATAL_ERROR "Prevented in-tree build. This is bad practice.")
endif(${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})

########################################################################
# Project setup
########################################################################
cmake_minimum_required(VERSION 2.6)
project( ffmpeg-api )

if(NOT CMAKE_BUILD_TYPE)
   set(CMAKE_BUILD_TYPE Release)
   message(STATUS "Build type not specified: defaulting to Release.")
endif(NOT CMAKE_BUILD_TYPE)
set(CMAKE_BUILD_TYPE ${CMAKE_BUILD_TYPE} CACHE STRING "")

########################################################################
# Install Dirs
########################################################################
if (NOT CMAKE_INSTALL_LIBDIR)
    include(GNUInstallDirs)
endif (NOT CMAKE_INSTALL_LIBDIR)

# Fall back to just "lib" if the item provided by GNUInstallDirs doesn't exist
if (NOT EXISTS "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}")
    message(STATUS "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR} does not exist. Defaulting install location to ${CMAKE_INSTALL_PREFIX}/lib.")
    set(CMAKE_INSTALL_LIBDIR lib)
endif()

set(RUNTIME_DIR bin)
set(LIBRARY_DIR ${CMAKE_INSTALL_LIBDIR})
set(INCLUDE_DIR include)

#INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/includes)
INCLUDE_DIRECTORIES("/home/panpan/player/ffmpeg-demo/test/include")
#add_subdirectory("../../../../../../../../ffmpeg-demo/test/lib")
link_directories("/home/panpan/player/ffmpeg-demo/test/lib")

#INCLUDE_DIRECTORIES(${PROJECT_BINARY_DIR}/includes)
#add_library(hal STATIC hal_main.c ${SOURCES} ${COMMON_SRCS} ${I2C_SRCS} ${SPI_SRCS} ${AD9361_SRCS} ${cal77s244_SRCS})
#add_library(ffmpeg-api STATIC ffmpeg-api.c)
#add_library(ffmpeg-api STATIC ffmpeg-api.c)
add_library(ffmpeg-api SHARED ffmpeg-api.c)

target_link_libraries(ffmpeg-api avcodec
	        avformat
	        avutil
		avfilter
	        swscale)
#target_link_libraries(jysw_common config)
install(TARGETS ffmpeg-api DESTINATION ${LIBRARY_DIR})
