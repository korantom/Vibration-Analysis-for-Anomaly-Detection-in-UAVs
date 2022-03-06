cmake_minimum_required(VERSION 3.13.1)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(vibration_module)

target_sources(app PRIVATE
               src/main.c

               src/accelerometer/lis2dh12/lis2dh12.c
               src/accelerometer/accelerometer.c
               src/shell_debug/lis2dh12_shell_debug.c

               src/writer/disk/disk.c
               src/writer/writer.c
              )
