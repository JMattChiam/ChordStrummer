cmake_minimum_required(VERSION 3.13)

include(pico_sdk_import.cmake)
add_subdirectory(../../libraries/pico-mpr121/lib mpr121)

project(chord_machine C CXX ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
pico_sdk_init()

add_executable(chord
  chord.c
  usb_descriptors.c
  74HC595.c
)

pico_enable_stdio_usb(chord 0)
pico_enable_stdio_uart(chord 1)

target_include_directories(chord PRIVATE ${CMAKE_CURRENT_LIST_DIR})

target_link_libraries(chord PRIVATE 
  pico_stdlib 
  pico-mpr121
  hardware_uart
  hardware_i2c
  tinyusb_device 
  tinyusb_board
)

pico_add_extra_outputs(chord)