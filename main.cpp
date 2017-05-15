#include <stdlib.h>
#include <exception>
#include <iostream>

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavdevice/avdevice.h>
#include <libavformat/avformat.h>
#include <libavutil/frame.h>
#include <libavutil/log.h>
}

#include "Webcam.h"
#include "RTPSink.h"


static AVFrame *alloc_picture(int width, int height)
{
    AVFrame *picture;
    int ret;

    picture = av_frame_alloc();
    if (!picture)
        return NULL;

    picture->format = AV_PIX_FMT_YUV420P;
    picture->width  = width;
    picture->height = height;

    /* allocate the buffers for the frame data */
    ret = av_frame_get_buffer(picture, 32);
    if (ret < 0) {
        fprintf(stderr, "Could not allocate frame data.\n");
        exit(1);
    }

    return picture;
}


int main()
{
    try
    {
//        av_log_set_level(AV_LOG_TRACE);

        av_register_all();
        avdevice_register_all();
        avformat_network_init();

        webcamrtp::Webcam webcam("Philips SPC230NC Webcam");
        webcamrtp::RTPSink sink(
                    webcam.width(),
                    webcam.height(),
                    webcam.fps(),
                    "libvpx",
                    "rtp://127.0.0.1:9090");
        while (AVFrame* frame = webcam.get())
            sink.put(frame);
    }
    catch (const std::exception& exc)
    {
        std::cerr << "Exception: " << exc.what() << std::endl;
        return EXIT_FAILURE;
    }
    catch (...)
    {
        std::cerr << "Caught unknown exception" << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
