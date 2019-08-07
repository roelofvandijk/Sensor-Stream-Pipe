//
// Created by amourao on 07/08/19.
//

#include <iostream>
#include <chrono>
#include <thread>
#include <unistd.h>

#include <zmq.hpp>

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>
#include <libavutil/pixdesc.h>
#include <libswscale/swscale.h>
}


#include "../encoders/FrameEncoder.h"
#include "../structs/FrameStruct.hpp"
#include "../readers/FrameReader.h"
#include "../utils/Utils.h"
#include "../utils/VideoUtils.hpp"


int main(int argc, char *argv[]) {
    srand(time(NULL) * getpid());
    //srand(getpid());

    std::unordered_map<std::string, AVCodec *> pCodecs;
    std::unordered_map<std::string, AVCodecContext *> pCodecContexts;
    std::unordered_map<std::string, AVCodecParameters *> pCodecParameters;

    AVPacket *pPacket = av_packet_alloc();
    AVFrame *pFrame = av_frame_alloc();

    cv::Mat img;
    bool imgChanged = false;

    av_register_all();

    int i = 0;

    try {
        if (argc < 3) {
            std::cerr << "Usage: video_encoder_test <frame_file> <codec parameters>"
                      << std::endl;
            return 1;
        }
        std::string frame_file = std::string(argv[1]);
        std::string codec_parameters_file = std::string(argv[2]);


        FrameEncoder fc(frame_file, codec_parameters_file);
        FrameReader fr(frame_file);

        while (fc.hasNextFrame()) {
            FrameStruct fo = fr.currentFrame();
            FrameStruct f = fc.currentFrameVid();

            cv::Mat frameOri = cv::imdecode(fo.frame, CV_LOAD_IMAGE_UNCHANGED);

            double localMin, localMax;
            cv::minMaxLoc(frameOri, &localMin, &localMax);

            if (pCodecs.find(f.streamId) == pCodecs.end()) {
                prepareDecodingStruct(f, pCodecs, pCodecContexts, pCodecParameters);
            }

            if (f.frameId == 1) { // reset the codec context pm video reset
                avcodec_flush_buffers(pCodecContexts[f.streamId]);
            }

            AVCodecContext *pCodecContext = pCodecContexts[f.streamId];

            pPacket->data = &f.frame[0];
            pPacket->size = f.frame.size();

            int response = avcodec_send_packet(pCodecContext, pPacket);
            if (response >= 0) {
                // Return decoded output data (into a frame) from a decoder
                response = avcodec_receive_frame(pCodecContext, pFrame);
                if (response >= 0) {
                    i++;
                    avframeToMat(pFrame, img);
                    imgChanged = true;
                    fr.goToFrame(i);
                    fr.nextFrame();
                }
            }

            if (imgChanged && !img.empty()) {
                cv::namedWindow("Original");
                cv::imshow("Original", frameOri);
                cv::namedWindow("Encoded");
                cv::imshow("Encoded", img);
                cv::waitKey(1);
            }

            fc.nextFrame();
        }
    }
    catch (std::exception &e) {
        std::cerr << e.what() << std::endl;
    }

    return 0;
}