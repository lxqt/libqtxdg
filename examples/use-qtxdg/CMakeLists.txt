project(use-qtxdg)
cmake_minimum_required(VERSION 2.8.11)


find_package(QT5XDG)

include(${QTXDG_USE_FILE})

add_executable(use-qtxdg main.cpp)

# The QTXDG_QT_LIBRARIES variable contains the needed Qt libraries. They are
#     set taking in account the choosed Qt version.
target_link_libraries(use-qtxdg ${QTXDG_QT_LIBRARIES} ${QTXDG_LIBRARIES})
