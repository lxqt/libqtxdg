project(use-qtxdg)
cmake_minimum_required(VERSION 2.8.12)

find_package(QT5XDG)
add_executable(use-qtxdg main.cpp)
target_link_libraries(use-qtxdg Qt5Xdg)
