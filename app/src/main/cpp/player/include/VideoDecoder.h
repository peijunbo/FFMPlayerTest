//
// Created by 裴俊博 on 2023/5/4.
//

#ifndef PLAYER_VIDEODECODER_H
#define PLAYER_VIDEODECODER_H
extern "C" {
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
#include <libavcodec/jni.h>
#include "libavutil/imgutils.h"
}
#include "VideoRender.h"
#include "DecoderBase.h"
#include "ImageDef.h"
class VideoDecoder : public DecoderBase {

public:

    VideoDecoder(const char *url){
        Init(url, AVMEDIA_TYPE_VIDEO);
    }

    virtual ~VideoDecoder(){
        UnInit();
    }


    int GetVideoWidth()
    {
        return m_VideoWidth;
    }
    int GetVideoHeight()
    {
        return m_VideoHeight;
    }

    void SetVideoRender(VideoRender *videoRender)
    {
        m_VideoRender = videoRender;
    }

    void SetDecodeToRGBA(bool decodeToRGBA) {
        m_DecodeToRGBA = decodeToRGBA;
    }
private:
    virtual void OnDecoderReady();
    virtual void OnDecoderDone();
    virtual void OnFrameAvailable(AVFrame *frame);

    const AVPixelFormat DST_PIXEL_FORMAT = AV_PIX_FMT_RGBA;
    bool m_DecodeToRGBA = false;
    int m_VideoWidth = 0;
    int m_VideoHeight = 0;

    int m_RenderWidth = 0;
    int m_RenderHeight = 0;

    AVFrame *m_RGBAFrame = nullptr;
    uint8_t *m_FrameBuffer = nullptr;

    VideoRender *m_VideoRender = nullptr;
    SwsContext *m_SwsContext = nullptr;
    SwsContext *m_RGBASwsContext = nullptr;
    // SingleVideoRecorder *m_pVideoRecorder = nullptr;
};

#endif //PLAYER_VIDEODECODER_H
