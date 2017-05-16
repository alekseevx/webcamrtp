#pragma once

#include <string>

extern "C"
{

#include <libavutil/rational.h>

struct AVFormatContext;
struct AVCodecContext;
struct AVFrame;

}

namespace webcamrtp
{

class RTPSink final
{
public:
    RTPSink(
        int width,
        int height,
        const std::string& codecName,
        const std::string& reciverUrl);
    ~RTPSink();

    void put(AVFrame* frame);

private:
    void clear();

private:
    AVFormatContext* oc = nullptr;
    AVCodecContext* enc = nullptr;
};

}
