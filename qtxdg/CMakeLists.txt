set(QTX_LIBRARIES Qt5::Widgets Qt5::Xml Qt5::DBus)

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

add_library(${QTXDGX_LIBRARY_NAME} SHARED
    ${libqtxdg_PUBLIC_H_FILES}
    ${libqtxdg_PRIVATE_H_FILES}
    ${libqtxdg_CPP_FILES}
    ${libqtxdg_MOCS}
)

target_link_libraries(${QTXDGX_LIBRARY_NAME}
    PUBLIC
        ${QTX_LIBRARIES}
        ${QTXDGX_ICONLOADER_LIBRARY_NAME}
)

set_target_properties(${QTXDGX_LIBRARY_NAME} PROPERTIES
  VERSION ${QTXDG_VERSION_STRING}
  SOVERSION ${QTXDG_MAJOR_VERSION}
)

target_compile_definitions(${QTXDGX_LIBRARY_NAME}
    PRIVATE
        "QTXDG_COMPILATION=\"1\""
        "QTXDG_VERSION=\"${QTXDG_VERSION_STRING}\""
        "QT_NO_KEYWORDS"
)

target_include_directories(${QTXDGX_LIBRARY_NAME}
    INTERFACE
        "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/${QTXDGX_FILE_NAME}>"
        "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
        "$<BUILD_INTERFACE:${QTXDGX_INTREE_INCLUDEDIR}/${QTXDGX_FILE_NAME}>"
        "$<BUILD_INTERFACE:${QTXDGX_INTREE_INCLUDEDIR}>"
    PRIVATE
        ${Qt5Gui_PRIVATE_INCLUDE_DIRS}
)

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
    COMPONENT Runtime
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
    ${libqtxdg_PORTABLE_HEADERS}
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/${QTXDGX_FILE_NAME}"
    COMPONENT Devel
)
