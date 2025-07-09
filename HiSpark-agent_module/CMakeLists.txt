    set(SOURCES_LIST
        ${CMAKE_CURRENT_SOURCE_DIR}/agent_module_main.c
        ${CMAKE_CURRENT_SOURCE_DIR}/driver/wm8978/wm8978.c
        ${CMAKE_CURRENT_SOURCE_DIR}/driver/iic/soft_iic.c
        ${CMAKE_CURRENT_SOURCE_DIR}/services/soundService.c
        ${CMAKE_CURRENT_SOURCE_DIR}/services/wifiService.c
        ${CMAKE_CURRENT_SOURCE_DIR}/services/websocketService.c
        ${CMAKE_CURRENT_SOURCE_DIR}/services/wavRecorderService.c
        ${CMAKE_CURRENT_SOURCE_DIR}/services/agentMainService.c
        ${CMAKE_CURRENT_SOURCE_DIR}/services/persistentWsClient.c
        ${CMAKE_CURRENT_SOURCE_DIR}/services/gateService.c
        ${CMAKE_CURRENT_SOURCE_DIR}/driver/sle/sle_uart_server_adv.c
        ${CMAKE_CURRENT_SOURCE_DIR}/driver/sle/sle_uart_server.c
        ${CMAKE_CURRENT_SOURCE_DIR}/utils/debugUtils.c
        ${CMAKE_CURRENT_SOURCE_DIR}/services/keyService.c
        ${CMAKE_CURRENT_SOURCE_DIR}/services/wsAudioPlayer.c
        ${CMAKE_CURRENT_SOURCE_DIR}/driver/oled/my_ssd1306.c
        ${CMAKE_CURRENT_SOURCE_DIR}/driver/oled/my_ssd1306_fonts.c
        ${CMAKE_CURRENT_SOURCE_DIR}/services/oledService.c
        ${CMAKE_CURRENT_SOURCE_DIR}/driver/sle/sle_uart_client.c
    )

    set(PUBLIC_HEADER_LIST
        ${CMAKE_CURRENT_SOURCE_DIR}/include/driver
        ${CMAKE_CURRENT_SOURCE_DIR}/include/services
        ${CMAKE_CURRENT_SOURCE_DIR}/include/utils
        ${CMAKE_CURRENT_SOURCE_DIR}/include/resources
        ${CMAKE_CURRENT_SOURCE_DIR}
    )

    set(SOURCES "${SOURCES_LIST}" PARENT_SCOPE)
    set(PUBLIC_HEADER "${PUBLIC_HEADER_LIST}" PARENT_SCOPE)
