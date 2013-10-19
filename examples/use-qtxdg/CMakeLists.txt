project(use-qtxdg)
cmake_minimum_required(VERSION 2.8)

option(USE_QT5 "Build using Qt5. Default off" OFF)

if(USE_QT5)
    find_package(qt5xdg)
    include_directories(${QTXDG_INCLUDE_DIRS})
else()
    find_package(qtxdg)
    include(${QTXDG_USE_FILE})
endif()

add_executable(use-qtxdg main.cpp)

if(USE_QT5)
    target_link_libraries(use-qtxdg ${QTXDG_LIBRARIES})
else()
    # The QTXDG_QT_LIBRARIES variable contains the needed Qt libraries. They are
    #     set taking in account the choosed Qt version.
    target_link_libraries(use-qtxdg ${QTXDG_QT_LIBRARIES} ${QTXDG_LIBRARIES})
endif()
