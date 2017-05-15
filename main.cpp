#include <stdlib.h>

extern "C" {
#include <libavutil/frame.h>
}

#include "Webcam.h"


int main()
{
    webcamrtp::Webcam webcam("Philips SPC230NC Webcam");
    while (true)
    {
        AVFrame* frame = webcam.next();
        av_frame_free(&frame);
    }

    return EXIT_SUCCESS;
}
