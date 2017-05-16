#include <stdexcept>
#include <iostream>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/time.h>
}

#include "RTPSink.h"


namespace webcamrtp
{

RTPSink::RTPSink(int width,
    int height,
    const std::string& codecName,
    const std::string& reciverUrl)
{
    AVCodec* codec = avcodec_find_encoder_by_name(codecName.c_str());
    if (codec == nullptr)
    {
        this->clear();
        throw std::runtime_error("Can't find codec " + codecName);
    }

    this->enc = avcodec_alloc_context3(codec);
    if (this->enc == nullptr)
    {
        this->clear();
        throw std::runtime_error("avcodec_alloc_context3 failed");
    }

    this->enc->time_base = {1, AV_TIME_BASE};
    this->enc->width = width;
    this->enc->height = height;
    this->enc->pix_fmt = AV_PIX_FMT_YUV420P;
    AVDictionary* encOpts = nullptr;
    av_dict_set(&encOpts, "b", "512K", 0);
    av_dict_set(&encOpts, "deadline", "realtime", 0);
    av_dict_set(&encOpts, "crf", "30", 0);
    av_dict_set(&encOpts, "g", "60", 0);

    int res = avcodec_open2(this->enc, codec, &encOpts);
    if (res != 0)
    {
        this->clear();
        throw std::runtime_error("avcodec_open2 failed");
    }

    res = avformat_alloc_output_context2(&this->oc, nullptr, "rtp", reciverUrl.c_str());
    if (res != 0)
    {
        this->clear();
        throw std::runtime_error("avformat_alloc_output_context2 failed");
    }

    AVStream* stream = avformat_new_stream(this->oc, this->enc->codec);
    if (stream == nullptr)
    {
        this->clear();
        throw std::runtime_error("avformat_new_stream failed");
    }

    res = avcodec_parameters_from_context(stream->codecpar, this->enc);
    if (res != 0)
    {
        this->clear();
        throw std::runtime_error("avcodec_parameters_from_context failed");
    }

    res = avio_open(&this->oc->pb, this->oc->filename, AVIO_FLAG_WRITE);
    if (res != 0)
    {
        this->clear();
        throw std::runtime_error("avio_open failed");
    }

    res = avformat_write_header(this->oc, nullptr);
    if (res != 0)
    {
        this->clear();
        throw std::runtime_error("avformat_init_output failed");
    }
}

RTPSink::~RTPSink()
{
    this->clear();
}

void RTPSink::put(AVFrame* frame)
{
    frame->pts = av_gettime_relative();

    int res = avcodec_send_frame(this->enc, frame);
    av_frame_free(&frame);
    if (res != 0)
        throw std::runtime_error("avcodec_receive_frame failed");

    AVPacket pkg;
    av_init_packet(&pkg);

    res = avcodec_receive_packet(this->enc, &pkg);
    if (res == 0)
    {
        AVStream* stream = this->oc->streams[0];
        av_packet_rescale_ts(&pkg, this->enc->time_base, stream->time_base);

        pkg.stream_index = 0;
        res = av_interleaved_write_frame(this->oc, &pkg);
        if (res != 0)
            throw std::runtime_error("av_interleaved_write_frame failed");
    }
    av_packet_unref(&pkg);
}

void RTPSink::clear()
{
    if (this->enc != nullptr)
        avcodec_free_context(&this->enc);

    if (this->oc != nullptr)
    {
        av_write_trailer(this->oc);

        if (this->oc->pb != nullptr)
            avio_closep(&this->oc->pb);

        avformat_free_context(this->oc);
        this->oc = nullptr;
    }
}

}
