#include "VideoDecoder.h"

void VideoDecoder::OnDecoderReady() {
    LOGE("VideoDecoder::OnDecoderReady");

    m_VideoWidth = GetCodecContext()->width;
    m_VideoHeight = GetCodecContext()->height;

    if (m_MsgContext && m_MsgCallback)
        m_MsgCallback(m_MsgContext, MSG_DECODER_READY, 0);

    if (m_VideoRender != nullptr) {
        int dstSize[2] = {0};
        m_VideoRender->Init(m_VideoWidth, m_VideoHeight, dstSize);
        m_RenderWidth = dstSize[0];
        m_RenderHeight = dstSize[1];

        if (m_VideoRender->GetRenderType() == VIDEO_RENDER_ANWINDOW) {
            int fps = 25;
            long videoBitRate = m_RenderWidth * m_RenderHeight * fps * 0.2;
//            m_pVideoRecorder = new SingleVideoRecorder("/sdcard/learnffmpeg_output.mp4", m_RenderWidth, m_RenderHeight, videoBitRate, fps);
//            m_pVideoRecorder->StartRecord();
        }

        m_RGBAFrame = av_frame_alloc();
        int bufferSize = av_image_get_buffer_size(DST_PIXEL_FORMAT, m_RenderWidth, m_RenderHeight,
                                                  1);
        m_FrameBuffer = (uint8_t *) av_malloc(bufferSize * sizeof(uint8_t));
        av_image_fill_arrays(m_RGBAFrame->data, m_RGBAFrame->linesize,
                             m_FrameBuffer, DST_PIXEL_FORMAT, m_RenderWidth, m_RenderHeight, 1);

        m_SwsContext = sws_getContext(m_VideoWidth, m_VideoHeight, GetCodecContext()->pix_fmt,
                                      m_RenderWidth, m_RenderHeight, DST_PIXEL_FORMAT,
                                      SWS_FAST_BILINEAR, NULL, NULL, NULL);
        m_RGBASwsContext = sws_getContext(m_VideoWidth, m_VideoHeight, GetCodecContext()->pix_fmt,
                                          m_VideoWidth, m_VideoHeight, AV_PIX_FMT_RGBA,
                                          SWS_FAST_BILINEAR, NULL, NULL, NULL);

    } else {
        LOGE("VideoDecoder::OnDecoderReady m_VideoRender == null");
    }
}

void VideoDecoder::OnDecoderDone() {
    LOGE("VideoDecoder::OnDecoderDone");

    if (m_MsgContext && m_MsgCallback)
        m_MsgCallback(m_MsgContext, MSG_DECODER_DONE, 0);

    if (m_VideoRender)
        m_VideoRender->UnInit();

    if (m_RGBAFrame != nullptr) {
        av_frame_free(&m_RGBAFrame);
        m_RGBAFrame = nullptr;
    }

    if (m_FrameBuffer != nullptr) {
        free(m_FrameBuffer);
        m_FrameBuffer = nullptr;
    }

    if (m_SwsContext != nullptr) {
        sws_freeContext(m_SwsContext);
        m_SwsContext = nullptr;
    }
    if (m_RGBASwsContext != nullptr) {
        sws_freeContext(m_RGBASwsContext);
        m_SwsContext = nullptr;
    }
//    if(m_pVideoRecorder != nullptr) {
//        m_pVideoRecorder->StopRecord();
//        delete m_pVideoRecorder;
//        m_pVideoRecorder = nullptr;
//    }

}

void VideoDecoder::OnFrameAvailable(AVFrame *frame) {
    LOGE("VideoDecoder::OnFrameAvailable frame=%p", frame);
    if (m_VideoRender != nullptr && frame != nullptr) {
        NativeImage image;
        LOGE("VideoDecoder::OnFrameAvailable frame[w,h]=[%d, %d],format=%d,[line0,line1,line2]=[%d, %d, %d]",
             frame->width, frame->height, GetCodecContext()->pix_fmt, frame->linesize[0],
             frame->linesize[1], frame->linesize[2]);
        if (m_VideoRender->GetRenderType() == VIDEO_RENDER_ANWINDOW) {
            sws_scale(m_SwsContext, frame->data, frame->linesize, 0,
                      m_VideoHeight, m_RGBAFrame->data, m_RGBAFrame->linesize);

            image.format = IMAGE_FORMAT_RGBA;
            image.width = m_RenderWidth;
            image.height = m_RenderHeight;
            image.ppPlane[0] = m_RGBAFrame->data[0];
            image.pLineSize[0] = image.width * 4;
        } else if (m_DecodeToRGBA) {
            //无论格式，均转为RGBA格式
            // 初始化长度
            int buffer_size = av_image_get_buffer_size(AV_PIX_FMT_RGBA, frame->width, frame->height, 1);
            auto *out_buffer = (uint8_t *) av_malloc(buffer_size);
            int dst_linesize[1] = {frame->width * 4};
            sws_scale(m_RGBASwsContext, frame->data, frame->linesize, 0,
                      frame->height, &out_buffer, dst_linesize);
            image.format = IMAGE_FORMAT_RGBA;
            image.width = frame->width;
            image.height = frame->height;
            image.pLineSize[0] = dst_linesize[0];
            image.ppPlane[0] = out_buffer;
        } else if (GetCodecContext()->pix_fmt == AV_PIX_FMT_YUV420P ||
                   GetCodecContext()->pix_fmt == AV_PIX_FMT_YUVJ420P) {
            image.format = IMAGE_FORMAT_I420;
            image.width = frame->width;
            image.height = frame->height;
            image.pLineSize[0] = frame->linesize[0];
            image.pLineSize[1] = frame->linesize[1];
            image.pLineSize[2] = frame->linesize[2];
            image.ppPlane[0] = frame->data[0];
            image.ppPlane[1] = frame->data[1];
            image.ppPlane[2] = frame->data[2];
            if (frame->data[0] && frame->data[1] && !frame->data[2] &&
                frame->linesize[0] == frame->linesize[1] && frame->linesize[2] == 0) {
                // on some android device, output of h264 mediacodec decoder is NV12 兼容某些设备可能出现的格式不匹配问题
                image.format = IMAGE_FORMAT_NV12;
            }
        } else if (GetCodecContext()->pix_fmt == AV_PIX_FMT_NV12) {
            image.format = IMAGE_FORMAT_NV12;
            image.width = frame->width;
            image.height = frame->height;
            image.pLineSize[0] = frame->linesize[0];
            image.pLineSize[1] = frame->linesize[1];
            image.ppPlane[0] = frame->data[0];
            image.ppPlane[1] = frame->data[1];
        } else if (GetCodecContext()->pix_fmt == AV_PIX_FMT_NV21) {
            image.format = IMAGE_FORMAT_NV21;
            image.width = frame->width;
            image.height = frame->height;
            image.pLineSize[0] = frame->linesize[0];
            image.pLineSize[1] = frame->linesize[1];
            image.ppPlane[0] = frame->data[0];
            image.ppPlane[1] = frame->data[1];
        } else if (GetCodecContext()->pix_fmt == AV_PIX_FMT_RGBA) {
            image.format = IMAGE_FORMAT_RGBA;
            image.width = frame->width;
            image.height = frame->height;
            image.pLineSize[0] = frame->linesize[0];
            image.ppPlane[0] = frame->data[0];
        } else {
            sws_scale(m_SwsContext, frame->data, frame->linesize, 0,
                      m_VideoHeight, m_RGBAFrame->data, m_RGBAFrame->linesize);
            image.format = IMAGE_FORMAT_RGBA;
            image.width = m_RenderWidth;
            image.height = m_RenderHeight;
            image.ppPlane[0] = m_RGBAFrame->data[0];
            image.pLineSize[0] = image.width * 4;
        }

        m_VideoRender->RenderVideoFrame(&image);

//        if(m_pVideoRecorder != nullptr) {
//            m_pVideoRecorder->OnFrame2Encode(&image);
//        }
    }

    if (m_MsgContext && m_MsgCallback)
        m_MsgCallback(m_MsgContext, MSG_REQUEST_RENDER, 0);
}