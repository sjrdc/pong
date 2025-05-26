#pragma once

#include <array>
#include <utility>

namespace pong
{
    constexpr auto nr_extremeties = 4;
    using extremeties = std::array<std::pair<float, float>, nr_extremeties>;
    struct players
    {
        extremeties left_player;
        extremeties right_player;
    };
}