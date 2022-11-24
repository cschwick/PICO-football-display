# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
cmake_minimum_required(VERSION 3.13)

include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)

project(pico_st7789)

# library configuration (override if necessary, see pico-st7789 for a complete list)
set(SPI_TFT_PORT "spi1" CACHE STRING "TFT spi port-name")
set(TFT_RST  "11"   CACHE STRING "TFT RST pin number")
set(TFT_DC   "10"   CACHE STRING "TFT DC  pin number")
#set(SPI_TFT_CS   "9"    CACHE STRING "TFT CS  pin number")
set(SPI_TFT_RX   "12"   CACHE STRING "RX  pin number")
set(SPI_TFT_TX   "15"   CACHE STRING "TX  pin number")
set(SPI_TFT_SCK  "14"   CACHE STRING "SCK pin number")

add_compile_definitions(SPI_TFT_PORT=${SPI_TFT_PORT})
add_compile_definitions(TFT_RST=${TFT_RST})
add_compile_definitions(TFT_DC=${TFT_DC})
#add_compile_definitions(SPI_TFT_CS=${SPI_TFT_CS})
add_compile_definitions(SPI_TFT_RX=${SPI_TFT_RX})
add_compile_definitions(SPI_TFT_TX=${SPI_TFT_TX})
add_compile_definitions(SPI_TFT_SCK=${SPI_TFT_SCK})


add_library(${PROJECT_NAME} Pico_ST7789.cpp )
target_link_libraries(${PROJECT_NAME} pico_stdlib hardware_spi)

#target_include_directories(${PROJECT_NAME} PUBLIC include )

#pico_add_extra_outputs(${PROJECT_NAME})
