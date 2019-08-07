//
// Created by amourao on 27-06-2019.
//

#pragma once

#include <iostream>
#include <fstream>
#include <vector>

#include <cereal/archives/binary.hpp>

#include <opencv2/core/mat.hpp>
#include <opencv2/highgui.hpp>
#include <cv.hpp>

#include <yaml-cpp/yaml.h>

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>
#include <libavutil/pixdesc.h>
#include <libswscale/swscale.h>
#include <libavutil/opt.h>
#include <libavutil/imgutils.h>
}

#include "../structs/FrameStruct.hpp"
#include "../readers/FrameReader.h"

class FrameEncoder : public FrameReader {
private:

    unsigned int totalCurrentFrameCounter;

    AVFormatContext *pFormatContext;
    AVCodecParameters *pCodecParameters;
    AVCodecContext *pCodecContext;
    AVCodec *pCodec;

    AVFrame *pFrame;
    AVPacket *pPacket;

    YAML::Node codec_parameters;

    bool libAVReady;
    std::string streamId;

    void init();

    void encode();

    void prepareDepthFrame(cv::Mat frame);

    void prepareColorFrame(cv::Mat frame);

    void prepareGrayDepthFrame(cv::Mat frame, int range);

public:

    FrameEncoder(std::string filename, std::string frame_filename);

    ~FrameEncoder();

    void nextFrame();

    std::vector<unsigned char> currentFrameBytes();

    unsigned int currentFrameId();

    CodecParamsStruct getCodecParamsStruct();

    std::string getStreamID();

    FrameStruct currentFrameVid();
};