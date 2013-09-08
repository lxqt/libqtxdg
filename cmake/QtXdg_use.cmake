# - Find the Razor-qt include and library dirs and define a some macros
#


find_package(Qt4 REQUIRED QUIET)
include(${QT_USE_FILE})

set(QTXDG_QT_LIBRARIES ${QT_LIBRARIES})

include_directories(${QTXDG_INCLUDE_DIRS})
link_directories(${QTXDG_LIBRARY_DIRS})
