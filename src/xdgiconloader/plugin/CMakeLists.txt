set(xdgiconengineplugin_CPP_FILES
    xdgiconengineplugin.cpp
)

add_library(${QTXDGX_ICONENGINEPLUGIN_LIBRARY_NAME} MODULE
    ${xdgiconengineplugin_CPP_FILES}
)

target_compile_definitions(${QTXDGX_ICONENGINEPLUGIN_LIBRARY_NAME}
    PRIVATE
        "QT_NO_KEYWORDS"
)
target_link_libraries(${QTXDGX_ICONENGINEPLUGIN_LIBRARY_NAME}
    PUBLIC
        Qt6::Gui
        "${QTXDGX_ICONLOADER_LIBRARY_NAME}"
)

target_include_directories("${QTXDGX_ICONENGINEPLUGIN_LIBRARY_NAME}"
    PRIVATE
        "${Qt6Gui_PRIVATE_INCLUDE_DIRS}"
)

mark_as_advanced(QTXDGX_ICONENGINEPLUGIN_INSTALL_PATH)

if (NOT DEFINED QTXDGX_ICONENGINEPLUGIN_INSTALL_PATH)
    include(LXQtQueryQt)
    lxqt_query_qt(_QT_PLUGINS_DIR QT_INSTALL_PLUGINS)

    if (NOT _QT_PLUGINS_DIR)
        message(FATAL_ERROR "Qt6 plugin directory cannot be detected.")
    endif()
    set(QTXDGX_ICONENGINEPLUGIN_INSTALL_PATH "${_QT_PLUGINS_DIR}/iconengines")
endif()

message(STATUS "XdgIconEnginePlugin will be installed into: ${QTXDGX_ICONENGINEPLUGIN_INSTALL_PATH}")

install(TARGETS
    "${QTXDGX_ICONENGINEPLUGIN_LIBRARY_NAME}" DESTINATION "${QTXDGX_ICONENGINEPLUGIN_INSTALL_PATH}"
    COMPONENT Runtime
)

