#pragma once

#include <string>

extern "C"
{

#include <libavutil/rational.h>

struct AVFormatContext;
struct AVCodecContext;
struct AVFrame;
struct SwsContext;

}


namespace webcamrtp
{

class Webcam final
{
public:
    explicit Webcam(const std::string& webcamId);
    ~Webcam();

    AVRational fps() const;
    int width() const;
    int height() const;

    AVFrame* get();


private:
    void clear();
    AVFrame* convertColorSpace(AVFrame* frame);

private:
    AVFormatContext* ic = nullptr;
    AVCodecContext* dec = nullptr;
    SwsContext* sws = nullptr;
};

}
