# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Initialise pico_sdk from installed location
# (note this can come from environment, CMake cache etc)

# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.0.0)
set(toolchainVersion 13_2_Rel1)
set(picotoolVersion 2.0.0)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
set(PICO_BOARD pico CACHE STRING "Board type")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

project(UART-PN532Killer C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# Add executable. Default name is the project name, version 0.1

add_executable(UART-PN532Killer 
        UART-PN532Killer.c 
        Ntag/Ntag.C
        )

pico_set_program_name(UART-PN532Killer "UART-PN532Killer")
pico_set_program_version(UART-PN532Killer "0.1")

# Modify the below lines to enable/disable output over UART/USB
pico_enable_stdio_uart(UART-PN532Killer 1)
pico_enable_stdio_usb(UART-PN532Killer 0)

# Add the standard library to the build
target_link_libraries(UART-PN532Killer
        pico_stdlib)

# Add the standard include files to the build
target_include_directories(UART-PN532Killer PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
)

# Add any user requested libraries
target_link_libraries(UART-PN532Killer 
        hardware_timer
        hardware_watchdog
        hardware_clocks
        hardware_pwm
        )

pico_add_extra_outputs(UART-PN532Killer)

