# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.2.0)
set(toolchainVersion 14_2_Rel1)
set(picotoolVersion 2.2.0-a4)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
cmake_minimum_required(VERSION 3.13)

# Configurações de ambiente para a Extensão do Pico VS Code
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()

set(sdkVersion 2.2.0)
set(toolchainVersion 14_2_Rel1)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

set(PICO_BOARD pico_w CACHE STRING "Board type")

include(pico_sdk_import.cmake)
project(blink C CXX ASM)
pico_sdk_init()

set(FREERTOS_KERNEL_PATH ${CMAKE_CURRENT_LIST_DIR}/FreeRTOS-LTS/FreeRTOS/FreeRTOS-Kernel)
include(${FREERTOS_KERNEL_PATH}/portable/ThirdParty/GCC/RP2040/FreeRTOS_Kernel_import.cmake)

add_executable(blink
    blink.c
    inc/bmp280.c
    inc/bmp280_low_level.c
    inc/ssd1306.c
    inc/max30101.c
    inc/vl53l1x.c
    inc/vl53l0x.c
)

# Diretórios de inclusão (Headers)
target_include_directories(blink PRIVATE ${CMAKE_CURRENT_LIST_DIR} ${CMAKE_CURRENT_LIST_DIR}/inc)

# No seu CMakeLists.txt
target_link_libraries(blink
    pico_stdlib
    pico_cyw43_arch_lwip_sys_freertos
    pico_lwip_mqtt
    FreeRTOS-Kernel
    FreeRTOS-Kernel-Heap4
    hardware_i2c
    hardware_pwm                               
    hardware_adc
)

# Credenciais Wi‑Fi via arquivo .env (WIFI_SSID, WIFI_PASSWORD)
set(ENV_FILE "${CMAKE_CURRENT_LIST_DIR}/.env")
# Reconfigure when .env changes
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${ENV_FILE}")
if(EXISTS "${ENV_FILE}")
    file(READ "${ENV_FILE}" ENV_CONTENT)
    # Extrai WIFI_SSID=... (busca em qualquer linha)
    string(REGEX MATCH "WIFI_SSID[ \t]*=([^\r\n]*)" _ssid_line "${ENV_CONTENT}")
    if(CMAKE_MATCH_1)
        string(STRIP "${CMAKE_MATCH_1}" WIFI_SSID)
    endif()
    # Extrai WIFI_PASSWORD=... (busca em qualquer linha)
    string(REGEX MATCH "WIFI_PASSWORD[ \t]*=([^\r\n]*)" _pass_line "${ENV_CONTENT}")
    if(CMAKE_MATCH_1)
        string(STRIP "${CMAKE_MATCH_1}" WIFI_PASSWORD)
    endif()

    # Extrai DIST_THRESHOLD_MM=... (valor numérico)
    string(REGEX MATCH "DIST_THRESHOLD_MM[ \t]*=([^\r\n]*)" _th_line "${ENV_CONTENT}")
    if(CMAKE_MATCH_1)
        string(STRIP "${CMAKE_MATCH_1}" DIST_THRESHOLD_MM)
    endif()

    # Extrai TEMP_THRESHOLD_C=... (valor numérico em Celsius)
    string(REGEX MATCH "TEMP_THRESHOLD_C[ \t]*=([^\\r\\n]*)" _tth_line "${ENV_CONTENT}")
    if(CMAKE_MATCH_1)
        string(STRIP "${CMAKE_MATCH_1}" TEMP_THRESHOLD_C)
    endif()
endif()

if(NOT WIFI_SSID OR NOT WIFI_PASSWORD)
    message(WARNING "WIFI_SSID/WIFI_PASSWORD não definidos. Preencha .env (WIFI_SSID=..., WIFI_PASSWORD=...).")
endif()

# Valor padrão para threshold se não vier no .env
if(NOT DIST_THRESHOLD_MM)
    set(DIST_THRESHOLD_MM 200)
endif()
if(NOT TEMP_THRESHOLD_C)
    set(TEMP_THRESHOLD_C 30.0)
endif()

# Garante que as definições sejam sempre literais de string (inclui vazio "")
set(WIFI_SSID_DEFINED "\"${WIFI_SSID}\"")
set(WIFI_PASSWORD_DEFINED "\"${WIFI_PASSWORD}\"")

target_compile_definitions(blink PRIVATE
    WIFI_SSID=${WIFI_SSID_DEFINED}
    WIFI_PASSWORD=${WIFI_PASSWORD_DEFINED}
    DIST_THRESHOLD_MM=${DIST_THRESHOLD_MM}
    TEMP_THRESHOLD_C=${TEMP_THRESHOLD_C}
)

# Habilitar saída via USB (para ver o printf no terminal)
pico_enable_stdio_uart(blink 0)
pico_enable_stdio_usb(blink 1)

pico_add_extra_outputs(blink)