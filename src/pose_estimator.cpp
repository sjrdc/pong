#include "pose_estimator.h"

namespace
{
    void generate_coordinates(auto& generator, auto highest, auto& coordinates)
    {
        for (auto& point : coordinates)
        {
            point = std::make_pair(generator.bounded(highest.width()), generator.bounded(highest.height()));
        }
    }

    auto get_player_coordinates(auto& generator, auto highest)
    {
            pong::players players;
            generate_coordinates(generator, highest, players.left_player);
            generate_coordinates(generator, highest, players.right_player);
            return players;
    }
}

namespace pong
{
    players pose_estimator::infer(const QSize& size)
    {
        return get_player_coordinates(generator, size);
    }
}
