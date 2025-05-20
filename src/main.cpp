#include <QtWidgets/QApplication>
#include <QTimer>
#include <QCamera>
#include <QImageCapture>

#include <QRandomGenerator>
#include "mainwindow.h"

#include <array>

namespace
{
    void generate_coordinates(auto& generator, auto highest, auto& coordinates)
    {
        for (auto& point : coordinates)
        {
            point = QPoint(generator.bounded(highest.width()), generator.bounded(highest.height()));
        }
    }

    auto get_player_coordinates(auto& generator, auto highest)
    {
            pong::players players;
            generate_coordinates(generator, highest, players.left_player);
            generate_coordinates(generator, highest, players.right_player);
            return players;
    }

    void run_updates(pong::mainwindow& w, QTimer& timer, QRandomGenerator& generator)
    {
        auto update_players = [&generator, &w]()
        {
            w.update(get_player_coordinates(generator, w.size()));
        };
        QObject::connect(&timer, &QTimer::timeout, update_players);
        timer.start(500);
    }
}

int main(int argc, char* argv[])
{
    QApplication a(argc, argv);
    
    QCamera camera;
    QImageCapture capture;
    
    pong::mainwindow w;
    QTimer timer;
    QRandomGenerator generator;
    run_updates(w, timer, generator);

    return QApplication::exec();
}
