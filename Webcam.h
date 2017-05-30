#pragma once

#include <string>

extern "C"
{

#include <libavutil/rational.h>

struct AVFormatContext;
struct AVCodecContext;
struct AVFrame;
struct SwsContext;
struct AVPacket;

}


namespace webcamrtp
{

class Webcam final
{
public:
    explicit Webcam(const std::string& webcamId);
    ~Webcam();

    AVRational fps() const;
    int pixFmt() const;
    int width() const;
    int height() const;

    AVFrame* get();


private:
    void clear();

    void receivePackage(AVPacket* pkg);
    AVFrame* decoding(AVPacket* pkg);

private:
    AVFormatContext* ic = nullptr;
    AVCodecContext* dec = nullptr;
};

}
