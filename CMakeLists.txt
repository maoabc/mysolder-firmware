
cmake_minimum_required(VERSION 3.20.0)


set(BOARD_ROOT ${CMAKE_SOURCE_DIR})
#set(BOARD c210_g431)



find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(c210)

target_sources(app PRIVATE
  src/main.c
  src/usb_pd.c
  src/app_ui.c
  src/temperature_adc.c
  src/heater_controller.c
  src/pid_controller.c
  src/sleep_detection.c
  src/moving_average.c
  src/tft/canvas.c
  src/tft/fonts.c
)

