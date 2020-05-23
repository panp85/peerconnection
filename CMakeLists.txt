cmake_minimum_required(VERSION 3.5)
project(demo)

set(CMAKE_POSITION_INDEPENDENT_CODE     TRUE)

set(CMAKE_C_FLAGS               "${CMAKE_C_FLAGS}")
SET(CMAKE_CXX_FLAGS             "${CMAKE_CXX_FLAGS} -fno-exceptions -fPIC")
set(CMAKE_SHARED_LINKER_FLAGS   "${CMAKE_SHARED_LINKER_FLAGS}")
set(CMAKE_EXE_LINKER_FLAGS      "${CMAKE_EXE_LINKER_FLAGS}")

set(ARCH_PATH linux)
set(WEBRTC_PATH "/mnt/w3/player/webrtc-0415/webrtc/src")
set(WEBRTC_LIBRARY_PATH "/mnt/w3/player/webrtc-0415/webrtc/src/out/linux/obj")

add_definitions(
    "-DWEBRTC_POSIX"
    "-DWEBRTC_LINUX"
    "-DUSE_GLIB=1"
    "-std=c++11"
)

if (Linux)
set(ARCH_PATH linux)
elseif (Arm)
set(ARCH_PATH arm)
endif ()

include_directories(
    ${WEBRTC_PATH}
    ${WEBRTC_PATH}/third_party/abseil-cpp
    ${WEBRTC_PATH}/build/linux/debian_sid_amd64-sysroot/usr/include/glib-2.0
    )

#set(SOURCE_FILES
##    main.cc
#)

link_directories(
    ${link_directories}
    ${WEBRTC_PATH}/out/linux3/obj
    /usr/lib/x86_64-linux-gnu/
)
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/janus JANUS)
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/janus/plugins JANUS_PLUGINS)
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/linux LINUX_MAIN)
link_libraries(
    "${WEBRTC_LIBRARY_PATH}/libwebrtc.a"
    )

add_executable(janus_client ${LINUX_MAIN} ${JANUS} ${JANUS_PLUGINS})

TARGET_LINK_LIBRARIES(janus_client pthread stdc++)

set_target_properties(janus_client PROPERTIES OUTPUT_NAME "janus_client")
