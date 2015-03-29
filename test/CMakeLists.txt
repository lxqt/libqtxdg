set(PROJECT_NAME "qtxdg_test")

set(${PROJECT_NAME}_SRCS
    qtxdg_test.cpp
)

set(${PROJECT_NAME}_MOCS
    qtxdg_test.h
)

set(LIBRARIES
    ${QTXDGX_LIBRARY_NAME}
)


if (BUILD_TESTS)
    add_definitions(-DQTXDG_BUILDING_TESTS=1)
endif()

qt5_wrap_cpp(MOCS ${${PROJECT_NAME}_MOCS})

include_directories (
    ${CMAKE_SOURCE_DIR}
)

add_definitions(${Qt5Test_DEFINITINS})
include_directories (
    ${Qt5Test_INCLUDE_DIRS}
)
set(CMAKE_CXX_FLAGS
    "${CMAKE_CXX_FLAGS} ${Qt5Test_EXECUTABLE_COMPILE_FLAGS}"
)

add_executable(${PROJECT_NAME} ${${PROJECT_NAME}_SRCS} ${UIS} ${RSCS} ${TRS} ${MOCS} )

target_link_libraries ( ${PROJECT_NAME} ${Qt5Test_LIBRARIES} ${LIBRARIES} )

add_test(NAME ${PROJECT_NAME} COMMAND ${PROJECT_NAME})
