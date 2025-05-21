#include <QtWidgets/QApplication>
#include <QTimer>
#include <QCamera>
#include <QMediaDevices>
#include <QImageCapture>
#include <QMediaCaptureSession>

#include <QRandomGenerator>
#include "mainwindow.h"

#include <array>
#include <iostream>

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

    void run_updates(pong::mainwindow& w, QTimer& timer, QRandomGenerator& generator, QImageCapture& capture)
    {
        auto update_players = [&generator, &w, &capture]()
        {
            w.update(get_player_coordinates(generator, w.size()));
            std::cout << "ready for capture: " << std::boolalpha << capture.isReadyForCapture() << std::endl;
            if (capture.isReadyForCapture()) capture.capture();
        };
        QObject::connect(&timer, &QTimer::timeout, update_players);
        timer.start(500);
    }
    
    void process_captured_image(int requestId, const QImage& img)
    {
        Q_UNUSED(requestId);
        // QImage scaledImage =
                // img.scaled(ui->viewfinder->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);

        // ui->lastImagePreviewLabel->setPixmap(QPixmap::fromImage(scaledImage));

        // // Display captured image for 4 seconds.
        // displayCapturedImage();
        // QTimer::singleShot(4000, this, &Camera::displayViewfinder);
    }

    void display_error(QCamera::Error error, const QString& message)
    {
        std::cerr << message.toStdString() << std::endl;
    }
}

int main(int argc, char* argv[])
{
    QApplication a(argc, argv);
    
    const auto cameras = QMediaDevices::videoInputs();
    if (cameras.isEmpty())
    {
        std::cerr << "no camera available - exit" << std::endl;
        return 1;
    }

    auto camera = QCamera(QMediaDevices::defaultVideoInput());
    QImageCapture capture;
    QMediaCaptureSession capture_session;
    capture_session.setCamera(&camera);
    capture_session.setImageCapture(&capture);

    QObject::connect(&capture, &QImageCapture::imageCaptured, process_captured_image);
    QObject::connect(&camera, &QCamera::errorOccurred, display_error);

    camera.start();
    
    pong::mainwindow w;
    QTimer timer;
    QRandomGenerator generator;
    run_updates(w, timer, generator, capture);

    return QApplication::exec();
}
