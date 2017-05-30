#include <stdexcept>
#include <iostream>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavfilter/avfilter.h>
#include <libavutil/time.h>
#include <libswscale/swscale.h>
}

#include "RTPSink.h"


namespace webcamrtp
{
#if 1
RTPSink::RTPSink(int width,
    int height,
    int pixFmt,
    AVRational framerate,
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

    this->enc->time_base = av_inv_q(framerate);
    this->enc->width = width;
    this->enc->height = height;
    this->enc->pix_fmt = codec->pix_fmts[0];
    this->enc->framerate = framerate;
    AVDictionary* encOpts = nullptr;
    av_dict_set(&encOpts, "deadline", "realtime", 0);
    av_dict_set(&encOpts, "crf", "30", 0);
    av_dict_set(&encOpts, "g", "60", 0);
    av_dict_set(&encOpts, "b", "1024K", 0);

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

    this->sws = sws_getContext(width, height, (AVPixelFormat)pixFmt,
                               width, height, this->enc->pix_fmt,
                               SWS_BICUBIC, nullptr, nullptr, nullptr);

//    char sdpBuf[64*1024];
//    av_sdp_create(&this->oc, 1, sdpBuf, sizeof(sdpBuf));
//    std::clog << "SDP\n" << sdpBuf << std::endl;
}
#else
RTPSink::RTPSink(int width,
    int height,
    int pixFmt,
    const std::string& codecName,
    const std::string& reciverUrl)
{
    AVCodec* codec = avcodec_find_encoder_by_name("pcm_mulaw");
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

    this->enc->time_base = {1, 44100};
    this->enc->sample_rate = 44100;
    this->enc->sample_fmt = static_cast<AVSampleFormat>(pixFmt);
    this->enc->channels = 2;
    AVDictionary* encOpts = nullptr;
//    av_dict_set(&encOpts, "ar", "8000", 0);
//    av_dict_set(&encOpts, "ac", "1", 0);

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

    char sdpBuf[64*1024];
    av_sdp_create(&this->oc, 1, sdpBuf, sizeof(sdpBuf));
    std::clog << "SDP\n" << sdpBuf << std::endl;
}
#endif
RTPSink::~RTPSink()
{
    this->clear();
}

void RTPSink::put(AVFrame* frame)
{
    static int64_t start;
    static int64_t counter = 0;

    if (counter == 0)
        start = av_gettime_relative();

    if (++counter % 300)
    {
        int64_t curTime = av_gettime_relative();
        std::cout << "fps=" << counter/((curTime - start)/double(AV_TIME_BASE))
                  << std::endl;
    }
    if (this->sws != nullptr)
    {
        AVFrame* frame2 = av_frame_alloc();
        frame2->width = frame->width;
        frame2->height = frame->height;
        frame2->format = this->enc->pix_fmt;
        av_frame_get_buffer(frame2, 32);
        av_frame_copy_props(frame2, frame);
        sws_scale(this->sws, frame->data, frame->linesize, 0, this->enc->height,
                  frame2->data, frame2->linesize);
        av_frame_free(&frame);

        frame = frame2;
    }

    static int64_t pts = av_gettime_relative();
    frame->pts = ++pts;
//    frame->pts = av_rescale_q(frame->pts, {1, AV_TIME_BASE},
//                              this->enc->time_base);
    std::cout << frame->pts << " "
              << frame->pkt_pts << " "
              << av_gettime_relative() << " "
              << std::endl;

    int res = avcodec_send_frame(this->enc, frame);
    av_frame_free(&frame);
    if (res != 0)
        throw std::runtime_error("avcodec_receive_frame failed");

    while (true)
    {
        AVPacket pkg;
        av_init_packet(&pkg);

        res = avcodec_receive_packet(this->enc, &pkg);
        if (res != 0)
            break;

        AVStream* stream = this->oc->streams[0];
        av_packet_rescale_ts(&pkg, this->enc->time_base, stream->time_base);

        pkg.stream_index = 0;
        res = av_interleaved_write_frame(this->oc, &pkg);
        if (res != 0)
            throw std::runtime_error("av_interleaved_write_frame failed");
        av_packet_unref(&pkg);
    }
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
