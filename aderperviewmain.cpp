#include "aderperviewmain.h"

#include <QDebug>
#include <QTime>

//#include <cstring>
//#include <sstream>
//#include <chrono>
//#include <cmath>
//#include <thread>
//#include <algorithm>
#include "aderperviewprocess.h"
#include "aderperviewvideo.h"
//#include "version.hpp"
//#include "cxxopts.hpp"


//derperview/superview
//https://intofpv.com/t-using-free-command-line-sorcery-to-fake-superview
//https://github.com/Niek/superview


using namespace DerperView;

//cxxopts::Options options("derperview", "creates derperview (like superview) from 4:3 videos");

QString ADerperView::Go(const string inputFilename, const string outputFilename, const int totalThreads)
{
    InputVideoFile input(inputFilename);
    if (input.GetLastError() != 0)
        return QString::number(input.GetLastError());
    input.Dump();

    auto inputVideoInfo = input.GetVideoInfo();

    if (inputVideoInfo.width * 3 / 4 != inputVideoInfo.height)
    {
//        cerr << "Source not in 4:3 aspect ratio" << endl;
        QString message = "Source not in 4:3 aspect ratio (" + QString::number(inputVideoInfo.width) + "x" + QString::number(inputVideoInfo.height) + ")";
        emit processLog("error", message);
        return message;
    }
    if (
        inputVideoInfo.pixelFormat != AVPixelFormat::AV_PIX_FMT_YUV420P
        && inputVideoInfo.pixelFormat != AVPixelFormat::AV_PIX_FMT_YUVJ420P
    )
    {
//        cerr << "Source not in compatible pixel format" << endl;
        QString message = "Source not in compatible pixel format (AV_PIX_FMT_YUV420P or AV_PIX_FMT_YUVJ420P) (" + QString(inputVideoInfo.pixelFormat) + "). Remux to Mp3/yuv420 first.";
        emit processLog("error", message);
        return message;
    }

    auto outputVideoInfo = inputVideoInfo; // Copy video info and tweak for output
    outputVideoInfo.width = GetDerpedWidth(inputVideoInfo.width);
    outputVideoInfo.bitRate = static_cast<int>(inputVideoInfo.bitRate * 1.4);
    OutputVideoFile output(outputFilename, outputVideoInfo);
    if (output.GetLastError() != 0)
        return QString::number(output.GetLastError());

    int64_t percentageMarker = static_cast<int64_t>(floor(static_cast<float>(inputVideoInfo.totalFrames) / 100));
    int64_t frameCount = 0;
    int64_t encodedPacketCount = 0;

    AVFrame *outputFrame = nullptr;
    vector<vector<unsigned char>> data(totalThreads);
    vector<vector<unsigned char>> derperviewedData(totalThreads);
//    vector<thread> threads(totalThreads);
    int threadIndex = 0;

    // Allocate buffers
    auto frameBufferSize = av_image_get_buffer_size(static_cast<AVPixelFormat>(inputVideoInfo.pixelFormat), inputVideoInfo.width, inputVideoInfo.height, 1);
    auto derpBufferSize = av_image_get_buffer_size(static_cast<AVPixelFormat>(outputVideoInfo.pixelFormat), outputVideoInfo.width, outputVideoInfo.height, 1);
    for (int i = 0; i < totalThreads; i++)
    {
        data[i].resize(frameBufferSize);
        derperviewedData[i].resize(derpBufferSize);
    }

//    cout << "Running up with " << totalThreads << " thread" << (totalThreads > 1 ? "s" : "") << "..." << endl;
//    cout << "--------------------------------------------------------------------" <<  endl;

    QTime previousMediaTime = QTime();
    QTime previousRealTime = QTime();
    QTime mediaTime = QTime();

    auto frame = input.GetNextFrame();

//    QImage image = QImage((uchar*)frame->data[0], 200, 200, frame->linesize[0], QImage::Format_RGB888);

    while (frame != nullptr && !processStopped)
    {
        if (frame->width == 0) // Audio - stream it through
        {
            output.WriteNextFrame(frame);
        }
        else // Video, stretch that bad boy.
        {
            // Copy data into a contiguous buffer we can mess with
            auto copyResult = av_image_copy_to_buffer(data[threadIndex].data(), frameBufferSize, frame->data, frame->linesize, static_cast<AVPixelFormat>(frame->format), frame->width, frame->height, 1);

            // Set up thread to perform the stretchy stuff
            if (inputVideoInfo.pixelFormat == AVPixelFormat::AV_PIX_FMT_YUV420P || inputVideoInfo.pixelFormat == AVPixelFormat::AV_PIX_FMT_YUVJ420P)
//                threads[threadIndex] = thread(ProcessFrameYuv, frame->width, frame->height, ref(data[threadIndex]), ref(derperviewedData[threadIndex]));
                ProcessFrameYuv(frame->width, frame->height, ref(data[threadIndex]), ref(derperviewedData[threadIndex]));
            threadIndex ++;

            // If we've got all of our threads, then join the lot and write them to the output
            if (threadIndex >= totalThreads)
            {
//                for (int i = 0; i < totalThreads; i ++)
//                    threads[i].join();

                for (int i = 0; i < totalThreads; i++)
                {
                    // Put new data into an AVFrame
                    outputFrame = av_frame_alloc();
                    outputFrame->width = outputVideoInfo.width;
                    outputFrame->height = outputVideoInfo.height;
                    outputFrame->format = outputVideoInfo.pixelFormat;
                    outputFrame->pts = frameCount;
                    av_image_fill_arrays(outputFrame->data, outputFrame->linesize, reinterpret_cast<uint8_t *>(derperviewedData[i].data()), static_cast<AVPixelFormat>(outputFrame->format), outputFrame->width, outputFrame->height, 1);

                    encodedPacketCount += output.WriteNextFrame(outputFrame);

                    av_frame_free(&outputFrame);
                    frameCount++;

                    if (frameCount % percentageMarker == 0)
                    {
//                        cout << " " << ceil(static_cast<float>(frameCount) * 100 / inputVideoInfo.totalFrames) << "% ";
//                        cout.flush();

                        mediaTime = QTime::fromMSecsSinceStartOfDay(1000.0 * frameCount / (inputVideoInfo.frameRate.num / inputVideoInfo.frameRate.den));

                        double speed = 0;
                        if (previousRealTime != QTime())
                            speed = double(previousMediaTime.msecsTo(mediaTime)) / double(previousRealTime.msecsTo(QTime::currentTime()));

//                        qDebug()<<speed<<previousMediaTime<<mediaTime<<previousRealTime<<QTime::currentTime()<<previousMediaTime.msecsTo(mediaTime) << previousRealTime.msecsTo(QTime::currentTime());

                        emit processLog("output", "frame=" + QString::number(frameCount) + " time=" + mediaTime.toString("HH:mm:ss.zzz") + " bitrate=" + QString::number(inputVideoInfo.bitRate) + " speed=" + QString::number(speed, 'f', 3) + "x framerate=" + QString::number(inputVideoInfo.frameRate.num) + "/" + QString::number(inputVideoInfo.frameRate.den) + "=" + QString::number(inputVideoInfo.frameRate.num / inputVideoInfo.frameRate.den));

                        previousMediaTime = mediaTime;
                        previousRealTime = QTime::currentTime();
                    }
                    else if (frameCount % 5 == 0)
                    {
//                        cout << ".";
//                        cout.flush();
                    }
                }

                threadIndex = 0;
            }
        }

        frame = input.GetNextFrame();
    }

    // Clear left-over frames out of the thread buffer
//    for (int i = 0; i < threadIndex; i++)
//        threads[i].join();

    if (!processStopped)
    {
        for (int i = 0; i < threadIndex; i++)
        {
            outputFrame = av_frame_alloc();
            outputFrame->width = outputVideoInfo.width;
            outputFrame->height = outputVideoInfo.height;
            outputFrame->format = outputVideoInfo.pixelFormat;
            outputFrame->pts = frameCount;
            av_image_fill_arrays(outputFrame->data, outputFrame->linesize, reinterpret_cast<uint8_t *>(derperviewedData[i].data()), static_cast<AVPixelFormat>(outputFrame->format), outputFrame->width, outputFrame->height, 1);

            encodedPacketCount += output.WriteNextFrame(outputFrame);

            av_frame_free(&outputFrame);
            frameCount++;

            if (frameCount % percentageMarker == 0)
            {
    //            cout << " " << ceil(static_cast<float>(frameCount) * 100 / inputVideoInfo.totalFrames) << "% ";
            }
            else if (frameCount % 5 == 0)
            {
    //            cout << ".";
            }
        }
    }

    output.Flush();

    emit processLog("output", "Encoded packet count: " + QString::number(encodedPacketCount));
    emit processLog("output", "Frames read: " + QString::number(frameCount));
//    cout << endl;
//    cout << "Encoded packet count: " << encodedPacketCount << endl;
//    cout << "Frames read: " << frameCount << endl;

//    cout << "--------------------------------------------------------------------" << endl;

    if (processStopped)
    {
        QString message = "Job cancelled";
        emit processLog("error", message);
        return message;
    }
    else
        return "";
}

void ADerperView::onStopThreadProcess()
{
    qDebug()<<"ADerperView::onStopThreadProcess";

    processStopped = true;
}
