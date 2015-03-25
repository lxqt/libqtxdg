find_package(Qt5Widgets REQUIRED)
find_package(Qt5Xml REQUIRED)
find_package(Qt5DBus REQUIRED)

if (QTXDG_QTMIMETYPES)
    add_definitions("-DQT_MIMETYPES")
endif()

add_definitions(${Qt5Core_DEFINITIONS})
set(CMAKE_CXX_FLAGS
    "${CMAKE_CXX_FLAGS} ${Qt5Widgets_EXECUTABLE_COMPILE_FLAGS}"
)
set(QTXDG_QT_LIBRARIES ${Qt5Widgets_LIBRARIES} ${Qt5Xml_LIBRARIES} ${Qt5DBus_LIBRARIES})
set(QTXDG_LIBRARIES ${QTXDG_LIBRARIES} ${QTXDG_QT_LIBRARIES})
set(QTXDG_INCLUDE_DIRS
    "${QTXDG_INCLUDE_DIRS}"
    "${Qt5Widgets_INCLUDE_DIRS}"
    "${Qt5Xml_INCLUDE_DIRS}"
    "${Qt5DBus_INCLUDE_DIRS}"
)

link_directories("${QTXDG_LIBRARY_DIRS}")
include_directories("${QTXDG_INCLUDE_DIRS}")
