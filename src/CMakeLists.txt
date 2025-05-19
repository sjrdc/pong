include_directories(${Boost_include_dirs})
# add_library(pong)

add_executable(pong main.cpp mainwindow.cpp)
target_link_libraries(pong Qt::Widgets)
