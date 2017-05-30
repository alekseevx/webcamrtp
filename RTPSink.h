#pragma once

#include <string>

extern "C"
{

#include <libavutil/rational.h>

struct AVFormatContext;
struct AVCodecContext;
struct AVFrame;
struct AVFilterContext;
struct AVFilterGraph;
struct SwsContext;
}

namespace webcamrtp
{

class RTPSink final
{
public:
    RTPSink(
        int width,
        int height,
        int pixFmt,
        AVRational framerate,
        const std::string& codecName,
        const std::string& reciverUrl);
    ~RTPSink();

    void put(AVFrame* frame);

private:
    void clear();

private:
    AVFormatContext* oc = nullptr;
    AVCodecContext* enc = nullptr;

    AVFilterContext* buffersink = nullptr;
    AVFilterContext* buffersrc = nullptr;
    AVFilterGraph* filterGraph = nullptr;
    SwsContext* sws = nullptr;
};

}
