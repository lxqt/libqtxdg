cmake_minimum_required(VERSION 3.0.2 FATAL_ERROR)
project(libqtxdg)

option(BUILD_TESTS "Builds tests" OFF)
option(BUILD_DEV_UTILS "Builds and install development utils" OFF)

# additional cmake files
set(CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH}" "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

if (CMAKE_VERSION VERSION_LESS "3.1")
    include(CheckCXXCompilerFlag)
    CHECK_CXX_COMPILER_FLAG("-std=c++11" COMPILER_SUPPORTS_CXX11)
    if(COMPILER_SUPPORTS_CXX11)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
    else()
        CHECK_CXX_COMPILER_FLAG("-std=c++0x" COMPILER_SUPPORTS_CXX0X)
        # -std=c++0x is deprecated but some tools e.g. qmake or older gcc are still using it
        if(COMPILER_SUPPORTS_CXX0X)
            set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")
        else()
            message(FATAL_ERROR "Compiler ${CMAKE_CXX_COMPILER} does not support c++11/c++0x")
        endif()
    endif()
else()
  set(CMAKE_CXX_STANDARD 11)
endif()

set(QTXDG_MAJOR_VERSION 3)
set(QTXDG_MINOR_VERSION 2)
set(QTXDG_PATCH_VERSION 0)
set(QTXDG_VERSION_STRING ${QTXDG_MAJOR_VERSION}.${QTXDG_MINOR_VERSION}.${QTXDG_PATCH_VERSION})

set(QT_MINIMUM_VERSION "5.6.1")

include(GNUInstallDirs)             # Standard directories for installation
include(CMakePackageConfigHelpers)
include(GenerateExportHeader)
include(prevent_in_source_builds)
include(create_portable_headers)
include(create_pkgconfig_file)
include(compiler_settings NO_POLICY_SCOPE)

set(CMAKE_AUTOMOC ON)

find_package(Qt5 ${QT_MINIMUM_VERSION} CONFIG REQUIRED Widgets Svg Xml DBus)

if (BUILD_TESTS)
    find_package(Qt5 ${QT_MINIMUM_VERSION} CONFIG REQUIRED Test)
endif()


set(QTXDGX_LIBRARY_NAME "Qt5Xdg")
set(QTXDGX_FILE_NAME "qt5xdg")

set(QTXDGX_ICONLOADER_LIBRARY_NAME "Qt5XdgIconLoader")
set(QTXDGX_ICONLOADER_FILE_NAME "qt5xdgiconloader")
set(QTXDGX_ICONENGINEPLUGIN_LIBRARY_NAME "Qt5XdgIconPlugin")

set(QTXDGX_PKG_CONFIG_DESCRIPTION "Qt5Xdg, a Qt5 implementation of XDG standards")
set(QTXDGX_PKG_CONFIG_REQUIRES "Qt5Core >= ${QT_MINIMUM_VERSION}, Qt5Xml >= ${QT_MINIMUM_VERSION}, Qt5Widgets >= ${QT_MINIMUM_VERSION}, Qt5DBus >= ${QT_MINIMUM_VERSION}, Qt5XdgIconLoader = ${QTXDG_VERSION_STRING}")

set(QTXDGX_ICONLOADER_PKG_CONFIG_DESCRIPTION "Qt5XdgIconLader, a Qt5 XDG Icon Loader")
set(QTXDGX_ICONLOADER_PKG_CONFIG_REQUIRES "Qt5Gui >= ${QT_MINIMUM_VERSION}, Qt5Svg >= ${QT_MINIMUM_VERSION}")

set(QTXDGX_INTREE_INCLUDEDIR "${CMAKE_CURRENT_BINARY_DIR}/InTreeBuild/include")

if (NOT CMAKE_BUILD_TYPE)
    set ( CMAKE_BUILD_TYPE Release )
endif (NOT CMAKE_BUILD_TYPE)

message(STATUS "Building ${PROJECT_NAME} with Qt ${Qt5Core_VERSION}")

add_subdirectory(xdgiconloader)
add_subdirectory(qtxdg)

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

if (BUILD_DEV_UTILS)
    add_subdirectory(util)
endif()

configure_package_config_file(
    "${PROJECT_SOURCE_DIR}/cmake/${QTXDGX_FILE_NAME}-config.cmake.in"
    "${CMAKE_BINARY_DIR}/${QTXDGX_FILE_NAME}-config.cmake"
    INSTALL_DESTINATION "${CMAKE_INSTALL_DATADIR}/cmake/${QTXDGX_FILE_NAME}"
)

write_basic_package_version_file(
    "${CMAKE_BINARY_DIR}/${QTXDGX_FILE_NAME}-config-version.cmake"
    VERSION ${QTXDG_VERSION_STRING}
    COMPATIBILITY AnyNewerVersion
)

configure_package_config_file(
    "${PROJECT_SOURCE_DIR}/cmake/${QTXDGX_ICONLOADER_FILE_NAME}-config.cmake.in"
    "${CMAKE_BINARY_DIR}/${QTXDGX_ICONLOADER_FILE_NAME}-config.cmake"
    INSTALL_DESTINATION "${CMAKE_INSTALL_DATADIR}/cmake/${QTXDGX_ICONLOADER_FILE_NAME}"
)

write_basic_package_version_file(
    "${CMAKE_BINARY_DIR}/${QTXDGX_ICONLOADER_FILE_NAME}-config-version.cmake"
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
    REQUIRES_PRIVATE ${QTXDGX_ICONLOADER_LIBRARY_NAME}
    VERSION ${QTXDG_VERSION_STRING}
    INSTALL
)

create_pkgconfig_file(
    PACKAGE_NAME ${QTXDGX_ICONLOADER_LIBRARY_NAME}
    DESCRIPTIVE_NAME ${QTXDGX_ICONLOADER_LIBRARY_NAME}
    DESCRIPTION ${QTXDGX_ICONLOADER_PKG_CONFIG_DESCRIPTION}
    INCLUDEDIRS ${QTXDGX_ICONLOADER_FILE_NAME}
    LIBS ${QTXDGX_ICONLOADER_LIBRARY_NAME}
    REQUIRES ${QTXDGX_ICONLOADER_PKG_CONFIG_REQUIRES}
    VERSION ${QTXDG_VERSION_STRING}
    INSTALL
)

install(FILES
    "${CMAKE_BINARY_DIR}/${QTXDGX_FILE_NAME}-config.cmake"
    "${CMAKE_BINARY_DIR}/${QTXDGX_FILE_NAME}-config-version.cmake"
    DESTINATION "${CMAKE_INSTALL_DATADIR}/cmake/${QTXDGX_FILE_NAME}"
    COMPONENT Devel
)

install(FILES
    "${CMAKE_BINARY_DIR}/${QTXDGX_ICONLOADER_FILE_NAME}-config.cmake"
    "${CMAKE_BINARY_DIR}/${QTXDGX_ICONLOADER_FILE_NAME}-config-version.cmake"
    DESTINATION "${CMAKE_INSTALL_DATADIR}/cmake/${QTXDGX_ICONLOADER_FILE_NAME}"
    COMPONENT Devel
)

install(EXPORT
    "${QTXDGX_FILE_NAME}-targets"
    DESTINATION "${CMAKE_INSTALL_DATADIR}/cmake/${QTXDGX_FILE_NAME}"
    FILE "${QTXDGX_FILE_NAME}-targets.cmake"
    COMPONENT Devel
)

install(EXPORT
    "${QTXDGX_ICONLOADER_FILE_NAME}-targets"
    DESTINATION "${CMAKE_INSTALL_DATADIR}/cmake/${QTXDGX_ICONLOADER_FILE_NAME}"
    FILE "${QTXDGX_ICONLOADER_FILE_NAME}-targets.cmake"
    COMPONENT Devel
)

# uninstall target
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/cmake_uninstall.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
    IMMEDIATE @ONLY)

#add_custom_target(uninstall
#    COMMAND ${CMAKE_COMMAND} -P "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake")
