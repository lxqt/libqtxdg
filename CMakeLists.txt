cmake_minimum_required(VERSION 3.0.2 FATAL_ERROR)
project(libqtxdg)

option(BUILD_TESTS "Builds tests" OFF)

# additional cmake files
set(CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH}" "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

set(QTXDG_MAJOR_VERSION 1)
set(QTXDG_MINOR_VERSION 2)
set(QTXDG_PATCH_VERSION 0)
set(QTXDG_VERSION_STRING ${QTXDG_MAJOR_VERSION}.${QTXDG_MINOR_VERSION}.${QTXDG_PATCH_VERSION})


include(GNUInstallDirs)             # Standard directories for installation
include(CMakePackageConfigHelpers)
include(create_portable_headers)
include(create_pkgconfig_file)

add_definitions(-Wall)

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
set(QTXDGX_INTREE_INCLUDEDIR "${CMAKE_CURRENT_BINARY_DIR}/InTreeBuild/include")

include_directories(
    "${Qt5Gui_PRIVATE_INCLUDE_DIRS}"
)

set(QTX_LIBRARIES Qt5::Widgets Qt5::Xml Qt5::DBus)

if (NOT CMAKE_BUILD_TYPE)
    set ( CMAKE_BUILD_TYPE Release )
endif (NOT CMAKE_BUILD_TYPE)

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

QT5_WRAP_CPP(libqtxdg_CXX_FILES ${libqtxdg_MOCS})

add_library(${QTXDGX_LIBRARY_NAME} SHARED
    ${libqtxdg_PUBLIC_H_FILES}
    ${libqtxdg_PRIVATE_H_FILES}
    ${libqtxdg_PRIVATE_INSTALLABLE_H_FILES}
    ${libqtxdg_PRIVATE_H_FILES}
    ${libqtxdg_CPP_FILES}
    ${libqtxdg_CXX_FILES}
)


target_link_libraries(${QTXDGX_LIBRARY_NAME}
    PUBLIC
        ${QTX_LIBRARIES}
)

set_target_properties(${QTXDGX_LIBRARY_NAME} PROPERTIES
  VERSION ${QTXDG_VERSION_STRING}
  SOVERSION ${QTXDG_MAJOR_VERSION}
)


configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/${QTXDGX_FILE_NAME}-config.cmake.in"
    "${CMAKE_BINARY_DIR}/${QTXDGX_FILE_NAME}-config.cmake"
    INSTALL_DESTINATION "${CMAKE_INSTALL_DATADIR}/cmake/${QTXDGX_FILE_NAME}"
)

write_basic_package_version_file(
    "${CMAKE_BINARY_DIR}/${QTXDGX_FILE_NAME}-config-version.cmake"
    VERSION ${QTXDG_VERSION_STRING}
    COMPATIBILITY AnyNewerVersion
)

create_pkgconfig_file(
    PACKAGE_NAME ${QTXDGX_LIBRARY_NAME}
    DESCRIPTIVE_NAME ${QTXDGX_LIBRARY_NAME}
    DESCRIPTION ${QTXDGX_PKG_CONFIG_DESCRIPTION}
    INCLUDEDIRS ${QTXDGX_FILE_NAME}
    LIBS ${QTXDGX_LIBRARY_NAME}
    REQUIRES ${QTXDGX_PKG_CONFIG_REQUIRES}
    VERSION ${QTXDG_VERSION_STRING}
    INSTALL
)

target_compile_definitions(${QTXDGX_LIBRARY_NAME}
    PRIVATE "QTXDG_COMPILATION=\"1\""
)

target_include_directories(${QTXDGX_LIBRARY_NAME}
    INTERFACE "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/${QTXDGX_FILE_NAME}>"
    INTERFACE "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
    INTERFACE "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/${QTXDGX_FILE_NAME}/${QTXDG_VERSION_STRING}>"
)

# include directories and targets for the in tree build
target_include_directories(${QTXDGX_LIBRARY_NAME}
    INTERFACE "$<BUILD_INTERFACE:${QTXDGX_INTREE_INCLUDEDIR}/${QTXDGX_FILE_NAME}>"
    INTERFACE "$<BUILD_INTERFACE:${QTXDGX_INTREE_INCLUDEDIR}>"
    INTERFACE "$<BUILD_INTERFACE:${QTXDGX_INTREE_INCLUDEDIR}/${QTXDGX_FILE_NAME}/${QTXDG_VERSION_STRING}>"
)

export(TARGETS ${QTXDGX_LIBRARY_NAME} FILE "${CMAKE_BINARY_DIR}/${QTXDGX_FILE_NAME}-targets.cmake")
# end of in tree build stuff

# create the portble headers
create_portable_headers(libqtxdg_PORTABLE_HEADERS
    HEADER_NAMES ${libqtxdg_PUBLIC_CLASSES}
    OUTPUT_DIR "${QTXDGX_INTREE_INCLUDEDIR}/${QTXDGX_FILE_NAME}"
)

# Copy public headers (in tree building)
foreach(h ${libqtxdg_PUBLIC_H_FILES})
    get_filename_component(bh ${h} NAME)
    configure_file(${h} "${QTXDGX_INTREE_INCLUDEDIR}/${QTXDGX_FILE_NAME}/${bh}" COPYONLY)
endforeach()

# Copy private headers (in tree building)
foreach(h ${libqtxdg_PRIVATE_INSTALLABLE_H_FILES})
    get_filename_component(bh ${h} NAME)
    configure_file(${h} "${QTXDGX_INTREE_INCLUDEDIR}/${QTXDGX_FILE_NAME}/${QTXDG_VERSION_STRING}/private/qtxdg/${bh}" COPYONLY)
endforeach()

install(TARGETS
    ${QTXDGX_LIBRARY_NAME} DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    EXPORT "${QTXDGX_FILE_NAME}-targets"
)

install(EXPORT
    "${QTXDGX_FILE_NAME}-targets"
    DESTINATION "${CMAKE_INSTALL_DATADIR}/cmake/${QTXDGX_FILE_NAME}"
    COMPONENT Devel
)

install(FILES
    ${libqtxdg_PUBLIC_H_FILES}
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/${QTXDGX_FILE_NAME}"
    COMPONENT Devel
)

install(FILES
    ${libqtxdg_PRIVATE_INSTALLABLE_H_FILES}
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/${QTXDGX_FILE_NAME}/${QTXDG_VERSION_STRING}/private/qtxdg"
    COMPONENT Devel
)

install(FILES
    "${CMAKE_BINARY_DIR}/${QTXDGX_FILE_NAME}-config.cmake"
    "${CMAKE_BINARY_DIR}/${QTXDGX_FILE_NAME}-config-version.cmake"
    DESTINATION "${CMAKE_INSTALL_DATADIR}/cmake/${QTXDGX_FILE_NAME}"
    COMPONENT Devel
)

install(FILES
    ${libqtxdg_PORTABLE_HEADERS}
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/${QTXDGX_FILE_NAME}"
    COMPONENT Devel
)

if(BUILD_TESTS)
    enable_testing()
    target_compile_definitions(${QTXDGX_LIBRARY_NAME}
        PRIVATE "QTXDG_TESTS=\"1\""
    )
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
