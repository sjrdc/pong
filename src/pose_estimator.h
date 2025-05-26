#pragma once

#include "datatypes.h"

#include <QRandomGenerator>
#include <QSize>

namespace pong
{
    class pose_estimator
    {
    public:
        players infer(const QSize&);

    private:
        QRandomGenerator generator;
    };
}
