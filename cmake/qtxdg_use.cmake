# - Find the Razor-qt include and library dirs and define a some macros
#


find_package(Qt4 REQUIRED QtCore QtGui QtXml QUIET)
include(${QT_USE_FILE})

set(QTXDG_QT_LIBRARIES
    ${QT_QTCORE_LIBRARY}
    ${QT_QTGUI_LIBRARY}
    ${QT_QTXML_LIBRARY}
)

set(QTXDG_LIBRARIES ${QTXDG_LIBRARY} ${QTXDG_QT_LIBRARIES})

include_directories(${QTXDG_INCLUDE_DIRS})
link_directories(${QTXDG_LIBRARY_DIRS})

set(QTXDG_INCLUDE_DIRS
    ${QTXDG_INCLUDE_DIRS}
    ${QT_QTCORE_INCLUDE_DIR}
    ${QT_QTGUI_INCLUDE_DIR}
    ${QT_QTXML_INCLUDE_DIR}
)
