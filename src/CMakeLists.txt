
add_executable(pong mainwindow.cpp pose_estimator.cpp main.cpp)
target_link_libraries(pong PRIVATE Qt::Widgets Qt::Multimedia ${OpenCV_LIBS})