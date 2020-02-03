#include "awideview.h"

#include <QDebug>

int Go(const string inputFilename, const string outputFilename, const int totalThreads)
{
    InputVideoFile input(inputFilename);
    if (input.GetLastError() != 0)
        return input.GetLastError();
    input.Dump();

    auto inputVideoInfo = input.GetVideoInfo();

    if (inputVideoInfo.width * 3 / 4 != inputVideoInfo.height)
    {
        cerr << "Source not in 4:3 aspect ratio" << endl;
        return 1;
    }
    if (
        inputVideoInfo.pixelFormat != AVPixelFormat::AV_PIX_FMT_YUV420P
        && inputVideoInfo.pixelFormat != AVPixelFormat::AV_PIX_FMT_YUVJ420P
            &&inputVideoInfo.pixelFormat != AVPixelFormat::AV_PIX_FMT_YUVJ422P
    )
    {
        cerr << "Source not in compatible pixel format" << endl;
//        return 2;
    }

    auto outputVideoInfo = inputVideoInfo; // Copy video info and tweak for output
    outputVideoInfo.width = GetDerpedWidth(inputVideoInfo.width);
    outputVideoInfo.bitRate = static_cast<int>(inputVideoInfo.bitRate * 1.4);
    OutputVideoFile output(outputFilename, outputVideoInfo);
    if (output.GetLastError() != 0)
        return output.GetLastError();

    int64_t percentageMarker = static_cast<int64_t>(floor(static_cast<float>(inputVideoInfo.totalFrames) / 100));
    int64_t frameCount = 0;
    int64_t encodedPacketCount = 0;

    AVFrame *outputFrame = nullptr;
    vector<vector<unsigned char>> data(totalThreads);
    vector<vector<unsigned char>> derperviewedData(totalThreads);
    vector<thread> threads(totalThreads);
    int threadIndex = 0;

    // Allocate buffers
    auto frameBufferSize = av_image_get_buffer_size(static_cast<AVPixelFormat>(inputVideoInfo.pixelFormat), inputVideoInfo.width, inputVideoInfo.height, 1);
    auto derpBufferSize = av_image_get_buffer_size(static_cast<AVPixelFormat>(outputVideoInfo.pixelFormat), outputVideoInfo.width, outputVideoInfo.height, 1);
    for (int i = 0; i < totalThreads; i++)
    {
        data[i].resize(frameBufferSize);
        derperviewedData[i].resize(derpBufferSize);
    }

//    cout<<"Video-1" <<threadIndex << endl;
    cout << "Running up with " << totalThreads << " thread" << (totalThreads > 1 ? "s" : "") << "..." << endl;
    cout << "--------------------------------------------------------------------" <<  endl;
//    cout<<"Video0" <<threadIndex << endl;

    auto frame = input.GetNextFrame();
//    cout<<"Video1" <<threadIndex << endl;
    while (frame != nullptr)
    {
        if (frame->width == 0) // Audio - stream it through
        {
            output.WriteNextFrame(frame);
        }
        else // Video, stretch that bad boy.
        {
//            cout<<"Video2" <<threadIndex << endl;
            // Copy data into a contiguous buffer we can mess with
            auto copyResult = av_image_copy_to_buffer(data[threadIndex].data(), frameBufferSize, frame->data, frame->linesize, static_cast<AVPixelFormat>(frame->format), frame->width, frame->height, 1);

//            cout<<"Video2.5" <<threadIndex<<inputVideoInfo.pixelFormat<<frame->width<<frame->height << endl;
            // Set up thread to perform the stretchy stuff
            if (inputVideoInfo.pixelFormat == AVPixelFormat::AV_PIX_FMT_YUV420P || inputVideoInfo.pixelFormat == AVPixelFormat::AV_PIX_FMT_YUVJ420P ||inputVideoInfo.pixelFormat == AV_PIX_FMT_YUVJ422P)
                threads[threadIndex] = thread(ProcessFrameYuv, frame->width, frame->height, ref(data[threadIndex]), ref(derperviewedData[threadIndex]));
            else
                threads[threadIndex] = thread(ProcessFrame, frame->width, frame->height, ref(data[threadIndex]), ref(derperviewedData[threadIndex]));
            threadIndex ++;

//            cout<<"Video3" <<threadIndex << endl;
            // If we've got all of our threads, then join the lot and write them to the output
            if (threadIndex >= totalThreads)
            {
//                cout<<"Video4" <<threadIndex << endl;
                for (int i = 0; i < totalThreads; i ++)
                {
//                    cout<<"Video4.5" <<threadIndex<<i<<threads[i].joinable() << endl;
                    threads[i].join();
//                    cout<<"Video4.6" <<threadIndex<<i<<threads[i].joinable() << endl;
                }

                for (int i = 0; i < totalThreads; i++)
                {
//                    cout<<"Video5" <<threadIndex<<i << endl;

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
                        cout << " " << ceil(static_cast<float>(frameCount) * 100 / inputVideoInfo.totalFrames) << "% ";
                        cout.flush();
                    }
                    else if (frameCount % 5 == 0)
                    {
                        cout << ".";
                        cout.flush();
                    }
                }

                threadIndex = 0;
            }
        }

//        cout<<"Video6" <<threadIndex << endl;

        frame = input.GetNextFrame();
    }
//    cout<<"Video7" <<threadIndex << endl;

    // Clear left-over frames out of the thread buffer
    for (int i = 0; i < threadIndex; i++)
        threads[i].join();

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
            cout << " " << ceil(static_cast<float>(frameCount) * 100 / inputVideoInfo.totalFrames) << "% ";
        else if (frameCount % 5 == 0)
            cout << ".";
    }

    output.Flush();

    cout << endl;
    cout << "Encoded packet count: " << encodedPacketCount << endl;
    cout << "Frames read: " << frameCount << endl;

    cout << "--------------------------------------------------------------------" << endl;

    return 0;
}

void SuppressLibAvOutput(void *careface, int whatevs, const char *pfff, va_list sigh)
{
    // STFU
}

