cmake_minimum_required(VERSION 2.8.11)
project(libqtxdg)

option(BUILD_TESTS "Builds tests" OFF)

# Standard directories for installation
include(GNUInstallDirs)

# additional cmake files
set(CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH}" "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

set(QTXDG_MAJOR_VERSION 1)
set(QTXDG_MINOR_VERSION 2)
set(QTXDG_PATCH_VERSION 0)
set(QTXDG_VERSION_STRING ${QTXDG_MAJOR_VERSION}.${QTXDG_MINOR_VERSION}.${QTXDG_PATCH_VERSION})

add_definitions(-Wall -DQTXDG_COMPILATION=1)

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    set(QTXDG_COMPILER_IS_CLANGCXX 1)
endif()

if (CMAKE_COMPILER_IS_GNUCXX OR QTXDG_COMPILER_IS_CLANGCXX)
    # set visibility to hidden to hide symbols, unless they're exported manually in the code
    set(CMAKE_CXX_FLAGS "-fvisibility=hidden -fvisibility-inlines-hidden -fno-exceptions ${CMAKE_CXX_FLAGS}")
endif()

find_package(Qt5Widgets REQUIRED QUIET)
find_package(Qt5Xml REQUIRED QUIET)
find_package(Qt5DBus REQUIRED QUIET)
if (BUILD_TESTS)
    find_package(Qt5Test REQUIRED QUIET)
endif()


set(QTXDGX_LIBRARY_NAME "Qt5Xdg")
set(QTXDGX_FILE_NAME "qt5xdg")

set(QTXDGX_PKG_CONFIG_DESCRIPTION "Qt5Xdg, a Qt5 implementation of XDG standards")
set(QTXDGX_PKG_CONFIG_REQUIRES "Qt5Core, Qt5Xml, Qt5Widgets, Qt5DBus")

include_directories(
    "${Qt5Gui_PRIVATE_INCLUDE_DIRS}"
)

set(QTX_LIBRARIES ${Qt5Widgets_LIBRARIES} ${Qt5Xml_LIBRARIES} ${Qt5DBus_LIBRARIES})
message(STATUS "Building with Qt ${Qt5Core_VERSION_STRING}")

set(libqtxdg_PUBLIC_H_FILES
    xdgaction.h
    xdgdesktopfile.h
    xdgdirs.h
    xdgicon.h
    xdgmenu.h
    xdgmenuwidget.h
    xmlhelper.h
    xdgautostart.h
    xdgmacros.h
    xdgmimetype.h
)

set(libqtxdg_PUBLIC_CLASSES
    XdgAction
    XdgDesktopFile
    XdgDirs
    XdgIcon
    XdgMenu
    XdgMenuWidget
    XmlHelper
    XdgAutoStart
    XdgMimeType
)

set(libqtxdg_PRIVATE_H_FILES
    xdgmenuapplinkprocessor.h
    xdgmenulayoutprocessor.h
    xdgmenu_p.h
    xdgmenureader.h
    xdgmenurules.h
    xdgdesktopfile_p.h
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
    xmlhelper.cpp
    xdgautostart.cpp
    xdgmimetype.cpp
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

set(libqtxdg_PRIVATE_INSTALLABLE_H_FILES
    qiconfix/qiconloader_p.h
)

#************************************************
# Build 2 config.cmake files
# One for in-tree build and second for normal one.
#************************************************

# In tree compilation ......................
set(QTXDG_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR};${CMAKE_CURRENT_BINARY_DIR}")

set(QTXDG_PRIVATE_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/qiconfix")
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/qt5xdg-config.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/cmake/${QTXDGX_FILE_NAME}-config.cmake"
    @ONLY
)

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/${QTXDGX_FILE_NAME}_use.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/cmake/${QTXDGX_FILE_NAME}_use.cmake"
    @ONLY
)

# Instalable ...............................
set(QTXDG_INCLUDE_DIR "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_INCLUDEDIR}/${QTXDGX_FILE_NAME}")

set(QTXDG_PRIVATE_INCLUDE_DIR "${QTXDG_INCLUDE_DIR}/${QTXDG_VERSION_STRING}")
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/qt5xdg-config.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/${QTXDGX_FILE_NAME}-config.cmake"
    @ONLY
)

#**********************************************************

QT5_WRAP_CPP(libqtxdg_CXX_FILES ${libqtxdg_MOCS})


if (NOT CMAKE_BUILD_TYPE)
    set ( CMAKE_BUILD_TYPE Release )
endif (NOT CMAKE_BUILD_TYPE)


add_library(${QTXDGX_LIBRARY_NAME} SHARED
    ${libqtxdg_PUBLIC_H_FILES}
    ${libqtxdg_PRIVATE_H_FILES}
    ${libqtxdg_PRIVATE_INSTALLABLE_H_FILES}
    ${libqtxdg_PRIVATE_H_FILES}
    ${libqtxdg_CPP_FILES}
    ${libqtxdg_CXX_FILES}
)


target_link_libraries(${QTXDGX_LIBRARY_NAME}
    ${QTX_LIBRARIES}
)

set_target_properties(${QTXDGX_LIBRARY_NAME} PROPERTIES
  VERSION ${QTXDG_VERSION_STRING}
  SOVERSION ${QTXDG_MAJOR_VERSION}
)

# create the portable headers
include(create_portable_headers)
create_portable_headers(libqtxdg_PORTABLE_HEADERS ${libqtxdg_PUBLIC_CLASSES})

install(TARGETS ${QTXDGX_LIBRARY_NAME} DESTINATION "${CMAKE_INSTALL_LIBDIR}")
install(FILES ${libqtxdg_PUBLIC_H_FILES} DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/${QTXDGX_FILE_NAME}")

install(FILES
        ${libqtxdg_PRIVATE_INSTALLABLE_H_FILES}
    DESTINATION
        "${QTXDG_PRIVATE_INCLUDE_DIR}/private/qtxdg"
)

install(FILES "${CMAKE_CURRENT_BINARY_DIR}/${QTXDGX_FILE_NAME}-config.cmake" DESTINATION "${CMAKE_INSTALL_DATADIR}/cmake/${QTXDGX_FILE_NAME}")
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/cmake/${QTXDGX_FILE_NAME}_use.cmake" DESTINATION "${CMAKE_INSTALL_DATADIR}/cmake/${QTXDGX_FILE_NAME}")
install(FILES ${libqtxdg_PORTABLE_HEADERS} DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/${QTXDGX_FILE_NAME}")

include(create_pkgconfig_file)
create_pkgconfig_file(${QTXDGX_LIBRARY_NAME}
    ${QTXDGX_PKG_CONFIG_DESCRIPTION}
    ${QTXDGX_PKG_CONFIG_REQUIRES}
    ${QTXDGX_FILE_NAME}
    ${QTXDG_VERSION_STRING}
)


if(BUILD_TESTS)
    enable_testing()
    add_definitions(-DQTXDG_TESTS=1)
    add_subdirectory(test)
else()
    message(STATUS "")
    message(STATUS "For building tests use -DBUILD_TESTS=Yes option.")
    message(STATUS "")
endif()

# uninstall target
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/cmake_uninstall.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
    IMMEDIATE @ONLY)

add_custom_target(uninstall
    COMMAND ${CMAKE_COMMAND} -P "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake")


# building tarball with CPack -------------------------------------------------
include (InstallRequiredSystemLibraries)
set (CPACK_PACKAGE_VERSION_MAJOR ${QTXDG_MAJOR_VERSION})
set (CPACK_PACKAGE_VERSION_MINOR ${QTXDG_MINOR_VERSION})
set (CPACK_PACKAGE_VERSION_PATCH ${QTXDG_PATCH_VERSION})
set (CPACK_GENERATOR TBZ2)
set (CPACK_SOURCE_GENERATOR TBZ2)
set (CPACK_SOURCE_IGNORE_FILES /build/;.gitignore;.*~;.git;.kdev4;temp)
include (CPack)
