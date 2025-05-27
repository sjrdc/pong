#pragma once

#include "datatypes.h"

#include <QRandomGenerator>
#include <QSize>

#include <opencv2/opencv.hpp>

namespace pong
{
    class pose_estimator
    {
    public:
        pose_estimator(const QSize&);
        pose_estimator(std::string, float, cv::dnn::Backend, cv::dnn::Target);

        players infer(const image& img);

    private:
        QRandomGenerator generator;
        QSize size;

    private:
        cv::dnn::Net net;
        std::string modelPath;
        cv::Size inputSize;
        float confThreshold;
        cv::dnn::Backend backendId;
        cv::dnn::Target targetId;
        float personBoxPreEnlargeFactor;
        float personBoxEnlargeFactor;
        cv::Mat anchors;
    
        std::tuple<cv::Mat, cv::Mat, cv::Mat, cv::Mat, cv::Mat, float> infer(cv::Mat image, cv::Mat person);
        std::tuple<cv::Mat, cv::Mat, float, cv::Mat, cv::Size> preprocess(cv::Mat image, cv::Mat person);
        std::tuple<cv::Mat, cv::Mat, cv::Mat, cv::Mat, cv::Mat, float> postprocess(std::vector<cv::Mat> blob, cv::Mat rotatedPersonBox, float angle, cv::Mat rotationMatrix, cv::Size padBias, cv::Size imgSize);
    };
}
