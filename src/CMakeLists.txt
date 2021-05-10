cmake_minimum_required(VERSION 2.8)

if (MSVC)
    link_libraries(opengl32)
else()
    link_libraries(GL)
endif()
link_libraries(glfw)
link_libraries(glad)

set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
set (CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DUSE_DEBUG_CONTEXT -g")

if (BUILD_SHARED_LIBS)
    link_libraries("${MATH_LIBRARY}")
endif()

if (MSVC)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif()

set (LODEPNG_SOURCE "../deps/lodepng/lodepng.cpp")

file(GLOB CUSTOM_SOURCE "cpp/custom.cpp" "cpp/oglwrap_example.cpp" ${LODEPNG_SOURCE})
set (CUSTOM_BINARY_NAME "custom")

if (CMAKE_BUILD_TYPE MATCHES "RELEASE")
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DOGLWRAP_DEBUG=0")
endif()

add_executable(${CUSTOM_BINARY_NAME} WIN32 ${CUSTOM_SOURCE} ${ICON})

set(WINDOWS_BINARIES ${CUSTOM_BINARY_NAME})
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR})

