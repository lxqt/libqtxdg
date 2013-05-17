set(PROJECT_NAME "qtxdg_test")

set(${PROJECT_NAME}_SRCS
    qtxdg_test.cpp
)

set(${PROJECT_NAME}_MOCS
    qtxdg_test.h
)


set(LIBRARIES
    qtxdg
)


add_definitions ( -Wall )

if (USE_QT5)
    qt5_wrap_cpp(MOCS ${${PROJECT_NAME}_MOCS})
else()
    qt4_wrap_cpp(MOCS ${${PROJECT_NAME}_MOCS})
endif()

include_directories (
    ${CMAKE_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_SOURCE_DIR}/libraries/
)

if (USE_QT5)
    add_definitions(${Qt5Test_DEFINITINS})
    include_directories (
        ${Qt5Test_INCLUDE_DIRS}
    )
else()
    include_directories (
        ${QT_QTCORE_INCLUDE_DIR}
    )
endif()

add_executable(${PROJECT_NAME} ${${PROJECT_NAME}_SRCS} ${UIS} ${RSCS} ${TRS} ${MOCS} )

if (USE_QT5)
    target_link_libraries ( ${PROJECT_NAME} ${Qt5Test_LIBRARIES} ${LIBRARIES} )
else()
    target_link_libraries ( ${PROJECT_NAME} ${QT_LIBRARIES}  ${LIBRARIES} )
endif()

add_dependencies(${PROJECT_NAME} razorqt)

add_test(NAME ${PROJECT_NAME} COMMAND ${PROJECT_NAME})
