# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)


set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# initalize pico_sdk from installed location
# (note this can come from environment, CMake cache etc)
#set(PICO_SDK_PATH "/data/pico/pico-sdk")

# Pull in Raspberry Pi Pico SDK (must be before project)
#include(pico_sdk_import.cmake)
include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)

project(football C CXX ASM)
pico_sdk_init()

# library configuration (override if necessary, see pico-st7789 for a complete list)
set(SPI_TFT_PORT "spi1" CACHE STRING "TFT spi port-name")
#set(TFT_RST       "6"   CACHE STRING "TFT RST pin number")
set(TFT_RST       "10"   CACHE STRING "TFT RST pin number")
#set(TFT_DC        "7"   CACHE STRING "TFT DC  pin number")
set(TFT_DC        "11"   CACHE STRING "TFT DC  pin number")
#set(SPI_TFT_CS    "9"    CACHE STRING "TFT CS  pin number")
#set(SPI_TFT_CS    "13"    CACHE STRING "TFT CS  pin number") # To LED pin of TFT

set(SPI_TFT_RX   "12"   CACHE STRING "RX  pin number")
set(SPI_TFT_TX    "15"   CACHE STRING "TX  pin number")
set(SPI_TFT_SCK   "14"   CACHE STRING "SCK pin number")
set(TFT_BACKLIGHT "13"   CACHE STRING "TFT backlight")

set( BUTTON1 "18"  CACHE STRING "BUTTON 1 GPIO number")
set( BUTTON2 "19"  CACHE STRING "BUTTON 2 GPIO number")
set( BUTTON3 "20"  CACHE STRING "BUTTON 3 GPIO number")
set( BUTTON4 "21"  CACHE STRING "BUTTON 4 GPIO number")

set( ST7789_TFTWIDTH "240" CACHE STRING "Display width")
set( ST7789_TFTHEIGHT "320" CACHE STRING "Display height")

#add_compile_definitions(ST7789_TFTWIDTH=${ST7789_TFT_WIDTH})
#add_compile_definitions(ST7789_TFTHEIGHT=${ST7789_TFTHEIGHT})

add_compile_definitions(SPI_TFT_PORT=${SPI_TFT_PORT})
add_compile_definitions(TFT RST=${TFT_RST})
add_compile_definitions(TFT_DC=${TFT_DC})
#add_compile_definitions(SPI_TFT_CS=${SPI_TFT_CS})
add_compile_definitions(SPI_TFT_RX=${SPI_TFT_RX})
add_compile_definitions(SPI_TFT_TX=${SPI_TFT_TX})
add_compile_definitions(SPI_TFT_SCK=${SPI_TFT_SCK})
add_compile_definitions(TFT_BACKLIGHT=${TFT_BACKLIGHT})

add_compile_definitions( BUTTON1=${BUTTON1})
add_compile_definitions( BUTTON2=${BUTTON2})
add_compile_definitions( BUTTON3=${BUTTON3})
add_compile_definitions( BUTTON4=${BUTTON4})

# add preprocessor-constant DEBUG for Debug-builds
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  set(CMAKE_VERBOSE_MAKEFILE 1)
  add_compile_definitions(DEBUG)
else()
endif()

add_executable(${PROJECT_NAME} "football.cpp")

pico_enable_stdio_uart(${PROJECT_NAME} 0)
pico_enable_stdio_usb(${PROJECT_NAME} 1)

pico_set_program_name(${PROJECT_NAME} "football")
pico_set_program_version(${PROJECT_NAME} "0.1")

# build the st7789-library from our submodule
add_subdirectory(Pico_ST7789)

target_include_directories( ${PROJECT_NAME} PRIVATE Fonts Pico_ST7789 )

#set(CMAKE_VERBOSE_MAKEFILE TRUE)

# add libraries
target_link_libraries(${PROJECT_NAME}
  pico_stdlib
  hardware_spi
  pico_st7789
)

pico_add_extra_outputs(${PROJECT_NAME})
