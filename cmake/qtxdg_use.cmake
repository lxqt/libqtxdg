# - Find the Razor-qt include and library dirs and define a some macros
#


find_package(Qt4 REQUIRED QtCore QtGui QtXml QtDBus QUIET)
include(${QT_USE_FILE})

set(QTXDG_QT_LIBRARIES
    ${QT_QTCORE_LIBRARY}
    ${QT_QTGUI_LIBRARY}
    ${QT_QTXML_LIBRARY}
    ${QT_DBUS_LIBRARY}
)

set(QTXDG_LIBRARIES ${QTXDG_LIBRARY} ${QTXDG_QT_LIBRARIES})

set(QTXDG_INCLUDE_DIRS
    ${QTXDG_INCLUDE_DIRS}
    ${QT_QTCORE_INCLUDE_DIR}
    ${QT_QTGUI_INCLUDE_DIR}
    ${QT_QTXML_INCLUDE_DIR}
    ${QT_QTDBUS_INCLUDE_DIR}
)

set(QTXDG_DEFINITIONS ${QT_DEFINITIONS})

if (QTXDG_QTMIMETYPES)
    find_package(PkgConfig)
    pkg_check_modules(QTMIMETYPES REQUIRED
        QtMimeTypes
    )

    set(QTXDG_LIBRARIES ${QTXDG_LIBRARY} ${QTMIMETYPES_LIBRARIES})
    set(QTXDG_LIBRARY_DIRS ${QTXDG_LIBRARY_DIRS} ${QTMIMETYPES_LIBRARY_DIRS})
    set(QTXDG_DEFINITIONS ${QTXDG_DEFINITIONS} "-DQT_MIMETYPES")
    include_directories(${QTXDG_INCLUDE_DIR} ${QTMIMETYPES_INCLUDE_DIRS})
    link_directories(${QTXDG_LIBRARY_DIRS})
    add_definitions("-DQT_MIMETYPES")
else()
    include_directories(${QTXDG_INCLUDE_DIR} ${QTMIMETYPES_INCLUDE_DIRS})
    link_directories(${QTXDG_LIBRARY_DIRS})
endif()
