#include <QtWidgets/QWidget>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QAbstractGraphicsShapeItem>

#include <array>
#include <memory>
#include <utility>

namespace pong
{
    constexpr auto nr_extremeties = 4;
    using extremeties = std::array<QPointF, nr_extremeties>;
    struct players
    {
        extremeties left_player;
        extremeties right_player;
    };

    class mainwindow : public QWidget
    {
    public:
        mainwindow(QWidget* parent = nullptr);
        void update(const players&);
    
    private:
        void populate_scene();

        QGraphicsScene scene;
        QGraphicsView view;

        using player_graphics = std::array<std::unique_ptr<QAbstractGraphicsShapeItem>, nr_extremeties>;
        player_graphics left_player_graphics;
        player_graphics right_player_graphics;
        std::array<std::unique_ptr<QAbstractGraphicsShapeItem>, 4> rectangles;
    };
}
