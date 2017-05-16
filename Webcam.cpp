#include <stdexcept>

extern "C" {

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libswscale/swscale.h>

}

#include "Webcam.h"


namespace webcamrtp
{

Webcam::Webcam(const std::string& webcamId)
{
    AVInputFormat* format = av_find_input_format("dshow");
    if (format == nullptr)
    {
        this->clear();
        throw std::runtime_error("Can't find AVInputFormat for dshow");
    }

    AVDictionary* formatOpts = nullptr;
    av_dict_set(&formatOpts, "fflags", "nobuffer", 0);
    av_dict_set(&formatOpts, "rtbufsize", "100M", 0);
    av_dict_set(&formatOpts, "framerate", "30", 0);
    av_dict_set(&formatOpts, "pixel_format", "yuv420p", 0);

    std::string webcamUrl = "video=" + webcamId;
    int res = avformat_open_input(&this->ic, webcamUrl.c_str(), format, &formatOpts);
    if (res != 0)
    {
        this->clear();
        throw std::runtime_error("Can't open " + webcamId);
    }

    if (ic->nb_streams < 1)
    {
        this->clear();
        throw std::runtime_error(webcamId + ": No streams");
    }

    res = avformat_find_stream_info(this->ic, nullptr);
    if (res != 0)
    {
        this->clear();
        throw std::runtime_error("avformat_find_stream_info failed");
    }

    AVCodecParameters* codecpar = ic->streams[0]->codecpar;
    if (codecpar->codec_type != AVMEDIA_TYPE_VIDEO)
    {
        this->clear();
        throw std::runtime_error("Stream has invalid codec type");
    }

    AVCodec* codec = avcodec_find_decoder(codecpar->codec_id);
    if (codec == nullptr)
    {
        this->clear();
        throw std::runtime_error("Can't find codec");
    }

    this->dec = avcodec_alloc_context3(codec);
    if (this->dec == nullptr)
    {
        this->clear();
        throw std::runtime_error("avcodec_alloc_context3 failed");
    }

    res = avcodec_parameters_to_context(this->dec, codecpar);
    if (res != 0)
    {
        this->clear();
        throw std::runtime_error("avcodec_parameters_to_context failed");
    }

    res = avcodec_open2(this->dec, nullptr, nullptr);
    if (res != 0)
    {
        this->clear();
        throw std::runtime_error("avcodec_open2 failed");
    }

    av_dump_format(this->ic, 0, this->ic->filename, 0);
}

Webcam::~Webcam()
{
    this->clear();
}

AVRational Webcam::fps() const
{
    return av_guess_frame_rate(this->ic, this->ic->streams[0], 0);
}

int Webcam::width() const
{
    return this->dec->width;
}

int Webcam::height() const
{
    return  this->dec->height;
}

AVFrame* Webcam::get()
{
    while (true)
    {
        AVPacket pkg;
        av_init_packet(&pkg);
        this->receivePackage(&pkg);

        if (AVFrame* frame = decoding(&pkg))
            return frame;
    }
}

void Webcam::receivePackage(AVPacket* pkg)
{
    int res = av_read_frame(this->ic, pkg);
    if (res != 0)
        throw std::runtime_error("av_read_frame failed");
}

AVFrame* Webcam::decoding(AVPacket* pkg)
{
    int res = avcodec_send_packet(this->dec, pkg);
    av_packet_unref(pkg);

    if (res != 0)
        throw std::runtime_error("avcodec_send_packet failed");

    AVFrame* frame = av_frame_alloc();
    if (frame == nullptr)
        throw std::runtime_error("av_frame_alloc failed");

    res = avcodec_receive_frame(this->dec, frame);
    if (res == 0)
    {
        frame->pts = av_rescale_q(frame->pkt_dts,
                                  this->ic->streams[0]->time_base, {1, AV_TIME_BASE});
        return frame;
    }

    av_frame_free(&frame);
    if (res == AVERROR(EAGAIN))
        return nullptr;

    throw std::runtime_error("avcodec_receive_frame failed");
}

void Webcam::clear()
{
    if (this->dec != nullptr)
        avcodec_free_context(&dec);

    if (this->ic != nullptr)
        avformat_close_input(&ic);
}


}
