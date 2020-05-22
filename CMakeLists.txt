cmake_minimum_required(VERSION 3.5)
project(demo)

set(CMAKE_POSITION_INDEPENDENT_CODE     TRUE)

set(CMAKE_C_FLAGS               "${CMAKE_C_FLAGS}")
SET(CMAKE_CXX_FLAGS             "${CMAKE_CXX_FLAGS} -fno-exceptions -fPIC")
set(CMAKE_SHARED_LINKER_FLAGS   "${CMAKE_SHARED_LINKER_FLAGS}")
set(CMAKE_EXE_LINKER_FLAGS      "${CMAKE_EXE_LINKER_FLAGS}")

set(ARCH_PATH linux)
set(WEBRTC_PATH "/home/panpan/player/webrtc-0212/webrtc/src")
set(WEBRTC_LIBRARY_PATH "//home/panpan/player/webrtc-0212/webrtc/src/out/linux3/obj")

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
    )

set(SOURCE_FILES
    main.cpp
)

link_directories(
    ${link_directories}
    ${WEBRTC_PATH}/out/linux3/obj
    /usr/lib/x86_64-linux-gnu/
)

link_libraries(
    "${WEBRTC_LIBRARY_PATH}/libwebrtc.a"
    )

add_executable(demo ${SOURCE_FILES})

TARGET_LINK_LIBRARIES(demo pthread stdc++)

set_target_properties(demo PROPERTIES OUTPUT_NAME "demo")
