#pragma once

#include <string>

extern "C" {

#include <libavutil/rational.h>

struct AVFormatContext;
struct AVCodecContext;
struct AVFrame;

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

    AVFrame* next();


private:
    void clear();

private:
    AVFormatContext* ic = nullptr;
    AVCodecContext* dec = nullptr;
};

}
