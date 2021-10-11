cmake_minimum_required(VERSION 3.4)

project(bsod-afl)

include(FindPkgConfig)
pkg_check_modules (LIBVMI REQUIRED libvmi)
pkg_check_modules (CAPSTONE REQUIRED capstone)

include_directories(${CAPSTONE_INCLUDE_DIRS})
add_executable(bsod-afl src/main.c src/signals.c src/vmi.c src/afl.c src/tracer.c src/tracer_dynamic.c src/fuzz.c src/breakpoint.c)
target_link_libraries(bsod-afl ${LIBVMI_LIBRARIES} ${CAPSTONE_LIBRARIES})