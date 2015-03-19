cmake_minimum_required( VERSION 2.8.5 )

project(libqtxdg)

# Support different versions of Qt
option(USE_QT4 "Build with Qt4." $ENV{USE_QT4})
option(BUILD_TESTS "Builds tests" OFF)

if (USE_QT4)
    set(USE_QT5 FALSE)
else()
    set(USE_QT5 TRUE)
endif()

# The Qt4 version can be compiled with libmagic or with QtMimeTypes
# QtMimeTypes is the preferred way and also the default. libmagic will be
# dropped in future releases.
if (NOT USE_QT5)
    option(USE_QTMIMETYPES "Use QtMimeTypes library" ON)
endif()

# Standard directories for installation
include(GNUInstallDirs)

# additional cmake files
set(CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH}" "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

set (MAJOR_VERSION 1)
set (MINOR_VERSION 0)
set (PATCH_VERSION 0)
set(QTXDG_VERSION_STRING ${MAJOR_VERSION}.${MINOR_VERSION}.${PATCH_VERSION})

add_definitions(-Wall -DQTXDG_COMPILATION=1)

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    set(QTXDG_COMPILER_IS_CLANGCXX 1)
endif()

if (CMAKE_COMPILER_IS_GNUCXX OR QTXDG_COMPILER_IS_CLANGCXX)
    # set visibility to hidden to hide symbols, unless they're exported manually in the code
    set(CMAKE_CXX_FLAGS "-fvisibility=hidden -fvisibility-inlines-hidden -fno-exceptions ${CMAKE_CXX_FLAGS}")
endif()

find_package(PkgConfig)

if (USE_QT5)
    cmake_minimum_required(VERSION 2.8.11)
    find_package(Qt5Widgets REQUIRED QUIET)
    find_package(Qt5Xml REQUIRED QUIET)
    find_package(Qt5DBus REQUIRED QUIET)
    if (BUILD_TESTS)
        find_package(Qt5Test REQUIRED QUIET)
    endif()

    # if both Qt4 and Qt5 are installed we must check what version was found
    if (NOT ${Qt5Core_VERSION_MAJOR} EQUAL 5)
        message(FATAL_ERROR "Qt was found, but NOT Qt5.")
    endif()

    set(QTXDGX_LIBRARY_NAME "Qt5Xdg")
    set(QTXDGX_FILE_NAME "qt5xdg")

    set(QTXDGX_PKG_CONFIG_DESCRIPTION "Qt5Xdg, a Qt5 implementation of XDG standards")
    set(QTXDGX_PKG_CONFIG_REQUIRES "Qt5Core, Qt5Xml, Qt5Widgets")

    include_directories(
        "${Qt5Widgets_INCLUDE_DIRS}"
        "${Qt5Gui_PRIVATE_INCLUDE_DIRS}"
        "${Qt5Xml_INCLUDE_DIRS}"
    )
    add_definitions(${Qt5Core_DEFINITIONS})
#    set(CMAKE_CXX_FLAGS
#        "${CMAKE_CXX_FLAGS} ${Qt5Widgets_EXECUTABLE_COMPILE_FLAGS}"
#    )

    # QMimeDatabase and QMimeType are part of Qt5Core
    # We just use that as an mimetype provider.
    # An empty MIMETYPES_PROVIDER_LIBRARY means we are using Qt internal
    # mimetypes support
    set(MIMETYPES_PROVIDER_LIBRARY "")
    add_definitions("-DHAVE_QTMIMETYPES")

    set(QTX_LIBRARIES ${Qt5Widgets_LIBRARIES} ${Qt5Xml_LIBRARIES} ${Qt5DBus_LIBRARIES})
    message(STATUS "Building with Qt ${Qt5Core_VERSION_STRING}")
else()
    find_package(Qt4 REQUIRED QtCore QtGui QtXml QtDBus QUIET)
    if (BUILD_TESTS)
        find_package(Qt4 REQUIRED QtTest QUIET)
    endif()

    # if both Qt4 and Qt5 are installed we must check what version was found
    if (NOT ${QT_VERSION_MAJOR} EQUAL 4)
        message(FATAL_ERROR "Qt was found, but NOT Qt4")
    endif()

    set(QTXDGX_LIBRARY_NAME "qtxdg")
    set(QTXDGX_FILE_NAME "qtxdg")

    set(QTXDGX_PKG_CONFIG_DESCRIPTION "QtXdg, a Qt4 implementation of XDG standards")

    include(${QT_USE_FILE})
    set(QTX_LIBRARIES
        ${QT_QTCORE_LIBRARY}
        ${QT_QTGUI_LIBRARY}
        ${QT_QTXML_LIBRARY}
        ${QT_QTDBUS_LIBRARY}
    )
    message(STATUS "Building with Qt ${QTVERSION}")
endif()


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

if (USE_QT5)
    list(APPEND libqtxdg_PRIVATE_INSTALLABLE_H_FILES qiconfix/qiconloader_p.h)
    list(APPEND libqtxdg_CPP_FILES qiconfix/qiconloader.cpp)
else()
    list(APPEND libqtxdg_PRIVATE_H_FILES qiconfix/qiconloader_p_qt4.h)
    list(APPEND libqtxdg_CPP_FILES qiconfix/qiconloader_qt4.cpp)
endif()

if (NOT USE_QT5)
    if (USE_QTMIMETYPES)
        # Using QtMimeTypes to provide a better mimetype support on Qt4
        # Project repo: https://qt.gitorious.org/qtplayground/mimetypes
        pkg_check_modules(QTMIMETYPES REQUIRED
            QtMimeTypes
        )
        include_directories("${QTMIMETYPES_INCLUDE_DIRS}")
        set(QTXDGX_PKG_CONFIG_REQUIRES "QtCore, QtXml, QtMimeTypes")
        set(MIMETYPES_PROVIDER_LIBRARY ${QTMIMETYPES_LIBRARIES})
        link_directories("${QTMIMETYPES_LIBRARY_DIRS}")
        add_definitions("-DHAVE_QTMIMETYPES")
    else()
        # Use libmagic
        find_package(LibMagic REQUIRED QUIET)
        set(MIMETYPES_PROVIDER_LIBRARY ${LIBMAGIC_LIBRARY})
        set(QTXDGX_PKG_CONFIG_REQUIRES "QtCore, QtXml")

        list(APPEND libqtxdg_PUBLIC_H_FILES xdgmime.h)
        list(APPEND libqtxdg_PUBLIC_CLASSES XdgMime)
        list(APPEND libqtxdg_CPP_FILES xdgmime.cpp)
    endif()
endif()

if (USE_QTMIMETYPES OR USE_QT5)
    list(APPEND libqtxdg_PUBLIC_H_FILES xdgmimetype.h)
    list(APPEND libqtxdg_PUBLIC_CLASSES XdgMimeType)
    list(APPEND libqtxdg_CPP_FILES xdgmimetype.cpp)
endif()


set(APP_SHARE_DIR "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_DATADIR}/lib${QTXDGX_FILE_NAME}")
add_definitions(-DTRANSLATIONS_DIR=\"${APP_SHARE_DIR}\")


#************************************************
# Build 2 config.cmake files
# One for in-tree build and second for normal one.
#************************************************
set(QTXDG_MAJOR_VERSION ${MAJOR_VERSION})
set(QTXDG_MINOR_VERSION ${MINOR_VERSION})
set(QTXDG_PATCH_VERSION ${PATCH_VERSION})

# In tree compilation ......................
set(QTXDG_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR};${CMAKE_CURRENT_BINARY_DIR}")

if (USE_QT5)
    set(QTXDG_PRIVATE_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/qiconfix")
    configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/cmake/qt5xdg-config.cmake.in"
        "${CMAKE_CURRENT_BINARY_DIR}/cmake/${QTXDGX_FILE_NAME}-config.cmake"
        @ONLY
    )
else()
    configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/cmake/qtxdg-config.cmake.in"
        "${CMAKE_CURRENT_BINARY_DIR}/cmake/${QTXDGX_FILE_NAME}-config.cmake"
        @ONLY
    )
endif()

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/${QTXDGX_FILE_NAME}_use.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/cmake/${QTXDGX_FILE_NAME}_use.cmake"
    @ONLY
)

# Instalable ...............................
set(QTXDG_INCLUDE_DIR "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_INCLUDEDIR}/${QTXDGX_FILE_NAME}")

if (USE_QT5)
    set(QTXDG_PRIVATE_INCLUDE_DIR "${QTXDG_INCLUDE_DIR}/${QTXDG_VERSION_STRING}")
    configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/cmake/qt5xdg-config.cmake.in"
        "${CMAKE_CURRENT_BINARY_DIR}/${QTXDGX_FILE_NAME}-config.cmake"
        @ONLY
    )
else()
    configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/cmake/qtxdg-config.cmake.in"
        "${CMAKE_CURRENT_BINARY_DIR}/${QTXDGX_FILE_NAME}-config.cmake"
        @ONLY
    )
endif()

#**********************************************************

include(FindLibSuffix)

if(USE_QT5)
    QT5_WRAP_CPP(libqtxdg_CXX_FILES ${libqtxdg_MOCS})
else()
    QT4_WRAP_CPP(libqtxdg_CXX_FILES ${libqtxdg_MOCS})
endif()


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
    ${MIMETYPES_PROVIDER_LIBRARY}
)

set_target_properties(${QTXDGX_LIBRARY_NAME} PROPERTIES
  VERSION ${MAJOR_VERSION}.${MINOR_VERSION}.${PATCH_VERSION}
  SOVERSION ${MAJOR_VERSION}
)

# create the portable headers
include(create_portable_headers)
create_portable_headers(libqtxdg_PORTABLE_HEADERS ${libqtxdg_PUBLIC_CLASSES})

install(TARGETS ${QTXDGX_LIBRARY_NAME} DESTINATION "${CMAKE_INSTALL_LIBDIR}")
install(FILES ${libqtxdg_PUBLIC_H_FILES} DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/${QTXDGX_FILE_NAME}")

if (USE_QT5)
    install(FILES
            ${libqtxdg_PRIVATE_INSTALLABLE_H_FILES}
        DESTINATION
            "${QTXDG_PRIVATE_INCLUDE_DIR}/private/qtxdg"
    )
endif()

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
set (CPACK_PACKAGE_VERSION_MAJOR ${MAJOR_VERSION})
set (CPACK_PACKAGE_VERSION_MINOR ${MINOR_VERSION})
set (CPACK_PACKAGE_VERSION_PATCH ${PATCH_VERSION})
set (CPACK_GENERATOR TBZ2)
set (CPACK_SOURCE_GENERATOR TBZ2)
set (CPACK_SOURCE_IGNORE_FILES /build/;.gitignore;.*~;.git;.kdev4;temp)
include (CPack)
