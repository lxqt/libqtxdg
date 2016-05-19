include_directories (
    "${CMAKE_PROJECT_DIR}/qtxdg"
    ${CMAKE_CURRENT_BINARY_DIR}
)

set(QTXDG_DESKTOP_FILE_START_SRCS
    qtxdg-desktop-file-start.cpp
)

add_executable(qtxdg-desktop-file-start
    ${QTXDG_DESKTOP_FILE_START_SRCS}
)

target_compile_definitions(qtxdg-desktop-file-start
    PRIVATE "-DQTXDG_VERSION=\"${QTXDG_VERSION_STRING}\""
)

target_link_libraries(qtxdg-desktop-file-start
    ${QTXDGX_LIBRARY_NAME}
)

install(TARGETS
    qtxdg-desktop-file-start
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
    COMPONENT Runtime
)
