cmake_minimum_required( VERSION 2.6 )

project(libqtxdg)

option(USE_QT5 "Use Qt5. Defaults to Qt4" OFF)
option(BUILD_TESTS "Builds tests" OFF)

# additional cmake files
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

set (MAJOR_VERSION 0)
set (MINOR_VERSION 5)
set (PATCH_VERSION 0)


add_definitions(-Wall)

if (USE_QT5)
    cmake_minimum_required(VERSION 2.8.3)
    find_package(Qt5Widgets REQUIRED)
    find_package(Qt5Xml REQUIRED)
    find_package(Qt5LinguistTools REQUIRED)
    if (BUILD_TESTS)
        find_package(Qt5Test REQUIRED)
    endif()

    # if both Qt4 and Qt5 are installed we must check what version was found
    if (NOT ${Qt5Core_VERSION_MAJOR} EQUAL 5)
        message(FATAL_ERROR "Qt was found, but NOT Qt5.")
    endif()

    include_directories(${Qt5Widgets_INCLUDE_DIRS} ${Qt5Xml_INCLUDE_DIRS})
    add_definitions(${Qt5Core_DEFINITIONS})
    set(CMAKE_CXX_FLAGS
        "${CMAKE_CXX_FLAGS} ${Qt5Widgets_EXECUTABLE_COMPILE_FLAGS}"
    )

    message(STATUS "Building with Qt ${Qt5Core_VERSION_STRING}")
else()
    find_package(Qt4 REQUIRED QtCore QtGui QtXml)
    if (BUILD_TESTS)
        find_package(Qt4 REQUIRED QtTest)
    endif()

    # if both Qt4 and Qt5 are installed we must check what version was found
    if (NOT ${QT_VERSION_MAJOR} EQUAL 4)
        message(FATAL_ERROR "Qt was found, but NOT Qt4")
    endif()

    message(STATUS "Building with Qt ${QTVERSION}")
endif()


find_package(LibMagic REQUIRED)


set(libqtxdg_PUBLIC_H_FILES
    xdgaction.h
    xdgdesktopfile.h
    xdgdirs.h
    xdgicon.h
    xdgmenu.h
    xdgmenuwidget.h
    xdgmime.h
    xmlhelper.h
    xdgautostart.h
)

set(libqtxdg_PRIVATE_H_FILES
    xdgmenuapplinkprocessor.h
    xdgmenulayoutprocessor.h
    xdgmenu_p.h
    xdgmenureader.h
    xdgmenurules.h
    qiconfix/qiconloader_p.h
)

set(libqtxdg_CPP_FILES
    xdgaction.cpp
    xdgdesktopfile.cpp
    xdgdirs.cpp
    xdgicon.cpp
    xdgmenuapplinkprocessor.cpp
    xdgmenu.cpp
    xdgmenulayoutprocessor.cpp
    xdgmenureader.cpp
    xdgmenurules.cpp
    xdgmenuwidget.cpp
    xdgmime.cpp
    xmlhelper.cpp
    xdgautostart.cpp
    qiconfix/qiconloader.cpp
)

set(libqtxdg_MOCS
    xdgaction.h
    xdgmenuapplinkprocessor.h
    xdgmenu.h
    xdgmenu_p.h
    xdgmenureader.h
    xdgmenurules.h
    xdgmenuwidget.h
)

#************************************************
# Build 2 config.cmake files
# One for in-tree build and second for normal one.
#************************************************
set(QTXDG_MAJOR_VERSION ${MAJOR_VERSION})
set(QTXDG_MINOR_VERSION ${MINOR_VERSION})
set(QTXDG_PATCH_VERSION ${PATCH_VERSION})

# In tree compilation ......................
set(QTXDG_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR})
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/qtxdg-config.cmake.in
    ${CMAKE_BINARY_DIR}/cmake/qtxdg-config.cmake
    @ONLY
)

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/qtxdg_use.cmake
    ${CMAKE_BINARY_DIR}/cmake/qtxdg_use.cmake
    @ONLY
)

# Instalable ...............................
set(QTXDG_INCLUDE_DIR ${CMAKE_INSTALL_PREFIX}/include/qtxdg)

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/qtxdg-config.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/qtxdg-config.cmake
    @ONLY
)

if (NOT USE_QT5)
    set(QT_USE_QTXML TRUE)
endif()


# Translations **********************************
# QtXdg is an independent library, so I do not use razor_translate_ts
include(qtxdgTranslate)
qtxdg_translate_ts(libqtxdg_QM_FILES
    SOURCES
        ${libqtxdg_PUBLIC_H_FILES}
        ${libqtxdg_PRIVATE_H_FILES}
        ${libqtxdg_CPP_FILES}
        ${libqtxdg_UI_FILES}
)

#************************************************


#**********************************************************

include(RazorLibSuffix)
if (NOT USE_QT5)
    include(${QT_USE_FILE})
endif()

if (USE_QT5)
    QT5_WRAP_CPP(libqtxdg_CXX_FILES ${libqtxdg_MOCS})
else()
    QT4_WRAP_CPP(libqtxdg_CXX_FILES ${libqtxdg_MOCS})
endif()

if (NOT CMAKE_BUILD_TYPE)
    set ( CMAKE_BUILD_TYPE Release )
endif (NOT CMAKE_BUILD_TYPE)


set(APP_SHARE_DIR ${CMAKE_INSTALL_PREFIX}/share/libqtxdg)
add_definitions(-DTRANSLATIONS_DIR=\"${APP_SHARE_DIR}\")

add_library(qtxdg SHARED
    ${libqtxdg_PUBLIC_H_FILES}
    ${libqtxdg_PRIVATE_H_FILES}
    ${libqtxdg_CPP_FILES}
    ${libqtxdg_CXX_FILES}
    ${libqtxdg_QM_FILES}
)

if(USE_QT5)
    target_link_libraries(qtxdg
        ${Qt5Widgets_LIBRARIES}
        ${Qt5Xml_LIBRARIES}
        ${LIBMAGIC_LIBRARY}
    )
else()
    target_link_libraries(qtxdg  ${QT_LIBRARIES} ${LIBMAGIC_LIBRARY})
endif()

set_target_properties(qtxdg PROPERTIES
  VERSION ${MAJOR_VERSION}.${MINOR_VERSION}.${PATCH_VERSION}
  SOVERSION ${MAJOR_VERSION}
)

install(TARGETS qtxdg DESTINATION lib${LIB_SUFFIX})
install(FILES ${libqtxdg_PUBLIC_H_FILES} DESTINATION include/qtxdg)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/qtxdg-config.cmake DESTINATION share/cmake/qtxdg)
install(FILES cmake/qtxdg_use.cmake DESTINATION share/cmake/qtxdg)

include(create_pkgconfig_file)
create_pkgconfig_file(qtxdg "QtXdg, a Qt implementation of XDG standards")

if (${CMAKE_CURRENT_BINARY_DIR} STREQUAL ${CMAKE_BINARY_DIR})
    message(STATUS "")
    message(STATUS "For building tests use -DBUILD_TESTS=Yes option.")
    message(STATUS "")
endif()

if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(test)
endif()
