remove_definitions(
    -DQT_USE_QSTRINGBUILDER
    -DQT_NO_CAST_FROM_ASCII
)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC TRUE)

macro(qtxdg_add_test)
    foreach(_testname ${ARGN})
        add_executable(${_testname} ${_testname}.cpp)
        target_link_libraries(${_testname} Qt5::Test ${QTXDGX_LIBRARY_NAME})
        add_test(NAME ${_testname} COMMAND ${_testname})
    endforeach()
endmacro()

include_directories (
    "${CMAKE_SOURCE_DIR}/qtxdg"
)

set_property(DIRECTORY APPEND
    PROPERTY COMPILE_DEFINITIONS "QTXDG_BUILDING_TESTS=\"1\""
)

qtxdg_add_test(
    qtxdg_test
    tst_xdgdirs
    tst_xdgdesktopfile
)
