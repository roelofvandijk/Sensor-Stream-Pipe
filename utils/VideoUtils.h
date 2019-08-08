//
// Created by amourao on 07/08/19.
//

#pragma once

#include <ctime>
#include <iostream>
#include <string>
#include <stdlib.h>
#include <thread>
#include <unistd.h>


extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>
#include <libavutil/pixdesc.h>
#include <libswscale/swscale.h>
}

#include <opencv2/core/mat.hpp>
#include <opencv2/highgui.hpp>

#include "../structs/FrameStruct.hpp"

#define MAX_DEPTH_VALUE 4096

void avframeToMat(const AVFrame *frame, cv::Mat &image);

void prepareDecodingStruct(FrameStruct &f, std::unordered_map<std::string, AVCodec *> &pCodecs,
                           std::unordered_map<std::string, AVCodecContext *> &pCodecContexts,
                           std::unordered_map<std::string, AVCodecParameters *> &pCodecParameters);


cv::Mat getFloat(cv::Mat &input);

cv::Mat getUMat(cv::Mat &input);

std::vector<ushort> unique(const cv::Mat &input, bool sort = false);

void prepareGrayDepthFrame(cv::Mat frame, AVCodecContext *pCodecContext, AVFrame *pFrame, int range);

void prepareDepthFrame(cv::Mat frame, AVCodecContext *pCodecContext, AVFrame *pFrame);

void prepareColorFrame(cv::Mat frame, AVCodecContext *pCodecContext, AVFrame *pFrame);