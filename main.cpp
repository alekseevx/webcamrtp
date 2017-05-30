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



int main()
{
    try
    {
//        av_log_set_level(AV_LOG_TRACE);
        av_log_set_level(AV_LOG_ERROR);

        av_register_all();
        avdevice_register_all();
        avformat_network_init();

        webcamrtp::Webcam webcam("Philips SPC230NC Webcam");
        webcamrtp::RTPSink sink(
                    webcam.width(),
                    webcam.height(),
                    webcam.pixFmt(),
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
