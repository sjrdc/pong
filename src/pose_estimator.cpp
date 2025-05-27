#include "pose_estimator.h"

const long double _M_PI = 3.141592653589793238L;
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
    pose_estimator::pose_estimator(const QSize& s) :
        size(s)
    {        
    }

    pose_estimator::pose_estimator(std::string modPath, float confThresh = 0.5, 
        cv::dnn::Backend bId = cv::dnn::DNN_BACKEND_DEFAULT, 
        cv::dnn::Target tId = cv::dnn::DNN_TARGET_CPU) :
        modelPath(modPath), confThreshold(confThresh),
        backendId(bId), targetId(tId)
    {
        this->inputSize = cv::Size(256, 256);
        this->net = cv::dnn::readNet(this->modelPath);
        this->net.setPreferableBackend(this->backendId);
        this->net.setPreferableTarget(this->targetId);

        // RoI will be larger so the performance will be better, but preprocess will be slower.Default to 1.
        this->personBoxPreEnlargeFactor = 1;
        this->personBoxEnlargeFactor = 1.25;
    }

    std::tuple<cv::Mat, cv::Mat, float, cv::Mat, cv::Size> pose_estimator::preprocess(cv::Mat image, cv::Mat person)
    {
        /***
                Rotate input for inference.
                Parameters:
                  image - input image of BGR channel order
                  face_bbox - human face bounding box found in image of format [[x1, y1], [x2, y2]] (top-left and bottom-right points)
                  person_landmarks - 4 landmarks (2 full body points, 2 upper body points) of shape [4, 2]
                Returns:
                  rotated_person - rotated person image for inference
                  rotate_person_bbox - person box of interest range
                  angle - rotate angle for person
                  rotation_matrix - matrix for rotation and de-rotation
                  pad_bias - pad pixels of interest range
        */
        //  crop and pad image to interest range
        cv::Size padBias(0, 0); // left, top
        cv::Mat personKeypoints = person.colRange(4, 12).reshape(0, 4);
        cv::Point2f midHipPoint = cv::Point2f(personKeypoints.row(0));
        cv::Point2f fullBodyPoint = cv::Point2f(personKeypoints.row(1));
        // # get RoI
        double fullDist = norm(midHipPoint - fullBodyPoint);
        cv::Mat fullBoxf, fullBox;
        cv::Mat v1 = cv::Mat(midHipPoint) - fullDist, v2 = cv::Mat(midHipPoint);
        std::vector<cv::Mat> vmat = { cv::Mat(midHipPoint) - fullDist, cv::Mat(midHipPoint) + fullDist };
        cv::hconcat(vmat, fullBoxf);
        // enlarge to make sure full body can be cover
        cv::Mat cBox, centerBox, whBox;
        cv::reduce(fullBoxf, centerBox, 1, cv::REDUCE_AVG, CV_32F);
        whBox = fullBoxf.col(1) - fullBoxf.col(0);
        cv::Mat newHalfSize = whBox * this->personBoxPreEnlargeFactor / 2;
        vmat[0] = centerBox - newHalfSize;
        vmat[1] = centerBox + newHalfSize;
        cv::hconcat(vmat, fullBox);
        cv::Mat personBox;
        fullBox.convertTo(personBox, CV_32S);
        // refine person bbox
        cv::Mat idx = personBox.row(0) < 0;
        personBox.row(0).setTo(0, idx);
        idx = personBox.row(0) >= image.cols;
        personBox.row(0).setTo(image.cols , idx);
        idx = personBox.row(1) < 0;
        personBox.row(1).setTo(0, idx);
        idx = personBox.row(1) >= image.rows;
        personBox.row(1).setTo(image.rows, idx);        // crop to the size of interest

        image = image(cv::Rect(personBox.at<int>(0, 0), personBox.at<int>(1, 0), personBox.at<int>(0, 1) - personBox.at<int>(0, 0), personBox.at<int>(1, 1) - personBox.at<int>(1, 0)));
        // pad to square
        int top = int(personBox.at<int>(1, 0) - fullBox.at<float>(1, 0));
        int left = int(personBox.at<int>(0, 0) - fullBox.at<float>(0, 0));
        int bottom = int(fullBox.at<float>(1, 1) - personBox.at<int>(1, 1));
        int right = int(fullBox.at<float>(0, 1) - personBox.at<int>(0, 1));
        cv::copyMakeBorder(image, image, top, bottom, left, right, cv::BORDER_CONSTANT, cv::Scalar(0, 0, 0));
        padBias = cv::Point(padBias) + cv::Point(personBox.col(0)) - cv::Point(left, top);
        // compute rotation
        midHipPoint -= cv::Point2f(padBias);
        fullBodyPoint -= cv::Point2f(padBias);
        float radians = float(_M_PI / 2 - atan2(-(fullBodyPoint.y - midHipPoint.y), fullBodyPoint.x - midHipPoint.x));
        radians = radians - 2 * float(_M_PI) * int((radians + _M_PI) / (2 * _M_PI));
        float angle = (radians * 180 / float(_M_PI));
        //  get rotation matrix*
        cv::Mat rotationMatrix = cv::getRotationMatrix2D(midHipPoint, angle, 1.0);
        //  get rotated image
        cv::Mat rotatedImage;
        cv::warpAffine(image, rotatedImage, rotationMatrix, cv::Size(image.cols, image.rows));
        //  get landmark bounding box
        cv::Mat blob;
        cv::dnn::Image2BlobParams paramPoseMediapipe;
        paramPoseMediapipe.datalayout = cv::dnn::DNN_LAYOUT_NHWC;
        paramPoseMediapipe.ddepth = CV_32F;
        paramPoseMediapipe.mean = cv::Scalar::all(0);
        paramPoseMediapipe.scalefactor = cv::Scalar::all(1 / 255.);
        paramPoseMediapipe.size = this->inputSize;
        paramPoseMediapipe.swapRB = true;
        paramPoseMediapipe.paddingmode = cv::dnn::DNN_PMODE_NULL;
        blob = cv::dnn::blobFromImageWithParams(rotatedImage, paramPoseMediapipe); // resize INTER_AREA becomes INTER_LINEAR in blobFromImage
        cv::Mat rotatedPersonBox = (cv::Mat_<float>(2, 2) << 0, 0, image.cols, image.rows);

        return std::tuple<cv::Mat, cv::Mat, float, cv::Mat, cv::Size>(blob, rotatedPersonBox, angle, rotationMatrix, padBias);
    }

    std::tuple<cv::Mat, cv::Mat, cv::Mat, cv::Mat, cv::Mat, float> pose_estimator::infer(cv::Mat image, cv::Mat person)
    {
        int h = image.rows;
        int w = image.cols;
        // Preprocess
        std::tuple<cv::Mat, cv::Mat, float, cv::Mat, cv::Size> tw;
        tw = this->preprocess(image, person);
        cv::Mat inputBlob = get<0>(tw);
        cv::Mat rotatedPersonBbox = get<1>(tw);
        float  angle = get<2>(tw);
        cv::Mat rotationMatrix = get<3>(tw);
        cv::Size padBias = get<4>(tw);

        // Forward
        this->net.setInput(inputBlob);
        std::vector<cv::Mat> outputBlob;
        this->net.forward(outputBlob, this->net.getUnconnectedOutLayersNames());

        // Postprocess
        return 
          this->postprocess(outputBlob, rotatedPersonBbox, angle, rotationMatrix, padBias, cv::Size(w, h));
    }

    std::tuple<cv::Mat, cv::Mat, cv::Mat, cv::Mat, cv::Mat, float> pose_estimator::postprocess(std::vector<cv::Mat> blob, cv::Mat rotatedPersonBox, float angle, cv::Mat rotationMatrix, cv::Size padBias, cv::Size imgSize)
    {
        float valConf = blob[1].at<float>(0);
        if (valConf < this->confThreshold)
            return std::tuple<cv::Mat, cv::Mat, cv::Mat, cv::Mat, cv::Mat, float>(cv::Mat(), cv::Mat(), cv::Mat(), cv::Mat(), cv::Mat(), valConf);
        cv::Mat landmarks = blob[0].reshape(0, 39);
        cv::Mat mask = blob[2];
        cv::Mat heatmap = blob[3];
        cv::Mat landmarksWorld = blob[4].reshape(0, 39);

        cv::Mat deno;
        // recover sigmoid score
        cv::exp(-landmarks.colRange(3, landmarks.cols), deno);
        cv::divide(1.0, 1 + deno, landmarks.colRange(3, landmarks.cols));
        // TODO: refine landmarks with heatmap. reference: https://github.com/tensorflow/tfjs-models/blob/master/pose-detection/src/blazepose_tfjs/detector.ts#L577-L582
        heatmap = heatmap.reshape(0, heatmap.size[0]);
        // transform coords back to the input coords
        cv::Mat whRotatedPersonPbox = rotatedPersonBox.row(1) - rotatedPersonBox.row(0);
        cv::Mat scaleFactor = whRotatedPersonPbox.clone();
        scaleFactor.col(0) /= this->inputSize.width;
        scaleFactor.col(1) /= this->inputSize.height;
        landmarks.col(0) = (landmarks.col(0) - this->inputSize.width / 2) * scaleFactor.at<float>(0);
        landmarks.col(1) = (landmarks.col(1) - this->inputSize.height / 2) * scaleFactor.at<float>(1);
        landmarks.col(2) = landmarks.col(2) * std::max(scaleFactor.at<float>(1), scaleFactor.at<float>(0));
        cv::Mat coordsRotationMatrix;
        cv::getRotationMatrix2D(cv::Point(0, 0), angle, 1.0).convertTo(coordsRotationMatrix, CV_32F);
        cv::Mat rotatedLandmarks = landmarks.colRange(0, 2) * coordsRotationMatrix.colRange(0, 2);
        cv::hconcat(rotatedLandmarks, landmarks.colRange(2, landmarks.cols), rotatedLandmarks);
        cv::Mat rotatedLandmarksWorld = landmarksWorld.colRange(0, 2) * coordsRotationMatrix.colRange(0, 2);
        cv::hconcat(rotatedLandmarksWorld, landmarksWorld.col(2), rotatedLandmarksWorld);
        // invert rotation
        cv::Mat rotationComponent  = (cv::Mat_<double>(2, 2) <<rotationMatrix.at<double>(0,0), rotationMatrix.at<double>(1, 0), rotationMatrix.at<double>(0, 1), rotationMatrix.at<double>(1, 1));
        cv::Mat translationComponent = rotationMatrix(cv::Rect(2, 0, 1, 2)).clone();
        cv::Mat invertedTranslation = -rotationComponent * translationComponent;
        cv::Mat inverseRotationMatrix;
        cv::hconcat(rotationComponent, invertedTranslation, inverseRotationMatrix);
        cv::Mat center, rc;
        cv::reduce(rotatedPersonBox, rc, 0, cv::REDUCE_AVG, CV_64F);
        cv::hconcat(rc, cv::Mat(1, 1, CV_64FC1, 1) , center);
        //  get box center
        cv::Mat originalCenter(2, 1, CV_64FC1);
        originalCenter.at<double>(0) = center.dot(inverseRotationMatrix.row(0));
        originalCenter.at<double>(1) = center.dot(inverseRotationMatrix.row(1));
        for (int idxRow = 0; idxRow < rotatedLandmarks.rows; idxRow++)
        {
            landmarks.at<float>(idxRow, 0) = float(rotatedLandmarks.at<float>(idxRow, 0) + originalCenter.at<double>(0) + padBias.width); // 
            landmarks.at<float>(idxRow, 1) = float(rotatedLandmarks.at<float>(idxRow, 1) + originalCenter.at<double>(1) + padBias.height); // 
        }
        // get bounding box from rotated_landmarks
        double vmin0, vmin1, vmax0, vmax1;
        cv::minMaxLoc(landmarks.col(0), &vmin0, &vmax0);
        cv::minMaxLoc(landmarks.col(1), &vmin1, &vmax1);
        cv::Mat bbox = (cv::Mat_<float>(2, 2) << vmin0, vmin1, vmax0, vmax1);
        cv::Mat centerBox;
        cv::reduce(bbox, centerBox, 0, cv::REDUCE_AVG, CV_32F);
        cv::Mat whBox = bbox.row(1) - bbox.row(0);
        cv::Mat newHalfSize = whBox * this->personBoxEnlargeFactor / 2;
        std::vector<cv::Mat> vmat(2);
        vmat[0] = centerBox - newHalfSize;
        vmat[1] = centerBox + newHalfSize;
        cv::vconcat(vmat, bbox);
        // invert rotation for mask
        mask = mask.reshape(1, 256);
        cv::Mat invertRotationMatrix = cv::getRotationMatrix2D(cv::Point(mask.cols / 2, mask.rows / 2), -angle, 1.0);
        cv::Mat invertRotationMask;
        cv::warpAffine(mask, invertRotationMask, invertRotationMatrix, cv::Size(mask.cols, mask.rows));
        // enlarge mask
        cv::resize(invertRotationMask, invertRotationMask, cv::Size(int(whRotatedPersonPbox.at<float>(0)), int(whRotatedPersonPbox.at<float>(1))));
        // crop and pad mask
        int minW = -std::min(padBias.width, 0);
        int minH= -std::min(padBias.height, 0);
        int left = std::max(padBias.width, 0);
        int top = std::max(padBias.height, 0);
        cv::Size padOver = imgSize - cv::Size(invertRotationMask.cols, invertRotationMask.rows) - padBias;
        int maxW = std::min(padOver.width, 0) + invertRotationMask.cols;
        int maxH = std::min(padOver.height, 0) + invertRotationMask.rows;
        int right = std::max(padOver.width, 0);
        int bottom = std::max(padOver.height, 0);
        invertRotationMask = invertRotationMask(cv::Rect(minW, minH, maxW - minW, maxH - minH)).clone();
        cv::copyMakeBorder(invertRotationMask, invertRotationMask, top, bottom, left, right, cv::BORDER_CONSTANT, cv::Scalar::all(0));
        // binarize mask
        cv::threshold(invertRotationMask, invertRotationMask, 1, 255, cv::THRESH_BINARY);

        /* 2*2 person bbox: [[x1, y1], [x2, y2]]
        # 39*5 screen landmarks: 33 keypoints and 6 auxiliary points with [x, y, z, visibility, presence], z value is relative to HIP
        # Visibility is probability that a keypoint is located within the frame and not occluded by another bigger body part or another object
        # Presence is probability that a keypoint is located within the frame
        # 39*3 world landmarks: 33 keypoints and 6 auxiliary points with [x, y, z] 3D metric x, y, z coordinate
        # img_height*img_width mask: gray mask, where 255 indicates the full body of a person and 0 means background
        # 64*64*39 heatmap: currently only used for refining landmarks, requires sigmod processing before use
        # conf: confidence of prediction*/
        return std::tuple<cv::Mat , cv::Mat, cv::Mat, cv::Mat, cv::Mat, float>(bbox, landmarks, rotatedLandmarksWorld, invertRotationMask, heatmap, valConf);
    }

    players pose_estimator::infer(const image& img)
    {
        return get_player_coordinates(generator, size);
    }
}
