#include <QtWidgets/QApplication>
#include <QTimer>
#include <QCamera>
#include <QMediaDevices>
#include <QImageCapture>
#include <QMediaCaptureSession>
#include <QRandomGenerator>

#include <opencv2/opencv.hpp>

#include "mainwindow.h"
#include "pose_estimator.h"

#include <array>
#include <iostream>

namespace
{

    void run_updates(pong::mainwindow& w, pong::pose_estimator estimator, QTimer& timer)
    {
        auto update_players = [&estimator, &w]()
        {
            w.update(estimator.infer(w.size()));
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
        // std::cerr << "no camera available - exit" << std::endl;
        // return 1;
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
    pong::pose_estimator estimator;
    run_updates(w, estimator, timer);

    return QApplication::exec();
}
