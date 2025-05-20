#include "mainwindow.h"
#include <algorithm>
#include <QHBoxLayout>
#include <QScreen>
#include <QGuiApplication>
#include <iostream>
namespace
{
    constexpr auto circle_color = Qt::white;
    constexpr auto background_color = Qt::black;
    constexpr auto left_rectangle_color = Qt::green;
    constexpr auto right_rectangle_color = Qt::blue;
    constexpr auto stroke = 15.;
    constexpr auto border = 12.;
    constexpr auto diameter = 35.;
    constexpr auto radius_offset = QPointF(-0.5*diameter, -0.5*diameter);

    void update_player_positions(const auto& positions, auto& graphics)
    {
        assert(positions.size() == graphics.size());
        for (auto i = 0ul; i < positions.size(); ++i)
        {
            graphics[i]->setPos(positions[i] + radius_offset);
        }
    }
}

namespace pong
{

    mainwindow::mainwindow(QWidget* parent) :
        QWidget(parent),
        scene(this),
        view(&scene)
    {
        auto* layout = new QHBoxLayout(this);
        layout->addWidget(&view);
        setLayout(layout);
        view.setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        view.setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        this->setStyleSheet("QWidget { background-color: \"black\"; border-width: 0px; border: 0px; margin: 0px; }");
        showFullScreen();
        populate_scene();
    }

    void mainwindow::update(const players& p)
    {
        update_player_positions(p.right_player, right_player_graphics);
        update_player_positions(p.left_player, left_player_graphics);
    }

    void mainwindow::populate_scene()
    {
        const auto* screen = QGuiApplication::primaryScreen();
        const auto width = screen->availableGeometry().width() - 2.*border;
        const auto height = screen->availableGeometry().height() - 2.*border;        
        const auto half_width = 0.5*width;
        
        rectangles[0] = std::make_unique<QGraphicsRectItem>(0., 0., half_width, height);
        rectangles[0]->setBrush(left_rectangle_color);
        scene.addItem(rectangles[0].get());

        rectangles[1] = std::make_unique<QGraphicsRectItem>(stroke, stroke, half_width - 2.*stroke, height - 2.*stroke);
        rectangles[1]->setBrush(background_color);
        scene.addItem(rectangles[1].get());

        rectangles[2] = std::make_unique<QGraphicsRectItem>(half_width, 0., half_width, height);
        rectangles[2]->setBrush(right_rectangle_color);
        scene.addItem(rectangles[2].get());

        rectangles[3] = std::make_unique<QGraphicsRectItem>(half_width + stroke, stroke, half_width - 2.*stroke, height - 2.*stroke);
        rectangles[3]->setBrush(background_color);
        scene.addItem(rectangles[3].get());
        
        const auto make_ellipse = [this](auto& ptr) 
        {
            constexpr auto x = 0., y = 0.;
            ptr = std::make_unique<QGraphicsEllipseItem>(x, y, diameter, diameter); 
            ptr->setBrush(circle_color);
            scene.addItem(ptr.get());
        };
        
        std::for_each(left_player_graphics.begin(), left_player_graphics.end(), make_ellipse);
        std::for_each(right_player_graphics.begin(), right_player_graphics.end(), make_ellipse);
    }
}
