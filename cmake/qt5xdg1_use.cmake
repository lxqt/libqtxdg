find_package(Qt5Widgets REQUIRED)
find_package(Qt5Xml REQUIRED)

include_directories(${Qt5Widgets_INCLUDE_DIRS} ${Qt5Xml_INCLUDE_DIRS})
add_definitions(${Qt5Core_DEFINITIONS})
set(CMAKE_CXX_FLAGS
    "${CMAKE_CXX_FLAGS} ${Qt5Widgets_EXECUTABLE_COMPILE_FLAGS}"
)
set(QTXDG_QT_LIBRARIES ${Qt5Widgets_LIBRARIES} ${Qt5Xml_LIBRARIES})

include_directories(${QTXDG_INCLUDE_DIRS})
