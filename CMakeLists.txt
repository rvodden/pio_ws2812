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
set(sdkVersion 2.1.1)
set(toolchainVersion 14_2_Rel1)
set(picotoolVersion 2.1.1)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================

set(PICO_BOARD pico2_w CACHE STRING "Board type")
include(pico_sdk_import.cmake)

include(FetchContent)
FetchContent_GetProperties(freertos_kernel)
if (NOT freertos_kernel_POPULATED) 
    FetchContent_Populate(freertos_kernel
        GIT_REPOSITORY     "https://github.com/FreeRTOS/FreeRTOS-Kernel.git"
        GIT_TAG            "V11.2.0"
    )
    set(FREERTOS_KERNEL_PATH "${CMAKE_CURRENT_BINARY_DIR}/freertos_kernel-src" CACHE STRING "FreeRTOS Location" FORCE)
    include(CMakePrintHelpers)
    cmake_print_variables(FREERTOS_KERNEL_PATH freertos_kernel_SOURCE_DIR freertos_kernel_POPULATED)
    add_library(freertos_config INTERFACE)
    target_include_directories(freertos_config INTERFACE ${CMAKE_SOURCE_DIR}/include)
    message(${FREERTOS_KERNEL_PATH}/portable/ThirdParty/Community-Supported-Ports/GCC/RP2350_ARM_NTZ)
    add_subdirectory(${FREERTOS_KERNEL_PATH}/portable/ThirdParty/Community-Supported-Ports/GCC/RP2350_ARM_NTZ SYSTEM)
endif()


project(jems_disco C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

add_executable(${PROJECT_NAME})
target_sources(${PROJECT_NAME} PRIVATE 
    src/main.c
    src/ble.c
    src/ws2812.c
)
target_compile_definitions(${PROJECT_NAME} PUBLIC
    CYW43_LWIP=0
    CYW43_ENABLE_BLUETOOTH=1
    WANT_HCI_DUMP=1
)
target_include_directories(${PROJECT_NAME} PUBLIC
    ${CMAKE_CURRENT_BINARY_DIR}/generated
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_link_libraries(${PROJECT_NAME} PUBLIC
    FreeRTOS-Kernel-Heap4
    freertos_config
    pico_stdlib
    pico_btstack_ble
    pico_btstack_cyw43
    pico_cyw43_arch_none
)

pico_btstack_make_gatt_header(${PROJECT_NAME} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src/mygatt.gatt")
pico_generate_pio_header(${PROJECT_NAME} ${CMAKE_CURRENT_LIST_DIR}/src/ws2812.pio OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/generated)

pico_enable_stdio_uart(${PROJECT_NAME} 1)
pico_enable_stdio_usb(${PROJECT_NAME} 0)

pico_add_extra_outputs(${PROJECT_NAME})
