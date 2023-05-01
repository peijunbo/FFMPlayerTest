#include <cstring>
#include <string>
#include <assert.h>
#include "jni.h"
#include "LogUtils.h"
#include "android/log.h"
#include "android/native_window_jni.h"
#include "android/native_window.h"
#include "SLES/OpenSLES.h"
#include "SLES/OpenSLES_Android.h"
#include "audio_frame.h"
#include "SLES/OpenSLES_Platform.h"
#include <queue>
#include "thread"
extern "C" {
#include "libavutil/opt.h"
#include "libavutil/time.h"
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
#include "libavutil/imgutils.h"
#include "libswresample/swresample.h"
}


// 音频编码采样率
static const int AUDIO_DST_SAMPLE_RATE = 44100;
// 音频编码通道数
static const int AUDIO_DST_CHANNEL_COUNTS = 2;
// 音频编码声道格式
static const uint64_t AUDIO_DST_CHANNEL_LAYOUT = AV_CH_LAYOUT_STEREO;
// 音频编码比特率
static const int AUDIO_DST_BIT_RATE = 327000;
// ACC音频一帧采样数
static const int ACC_NB_SAMPLES = 1024;
// 采样格式
static const AVSampleFormat DST_SAMPLE_FORMAT = AV_SAMPLE_FMT_S16;
// 音频帧队列
static std::queue<AudioFrame *> audioFrameQueue;

static SLObjectItf engineObject;
static SLEngineItf engineEngine;
static SLEnvironmentalReverbItf outputMixEnvironmentalReverb = NULL;
// 播放器
static SLPlayItf audioPlayerPlay;
static SLVolumeItf audioPlayerVolume;
// 混音器
static SLObjectItf outputMixObj;
static const SLEnvironmentalReverbSettings reverbSettings = SL_I3DL2_ENVIRONMENT_PRESET_STONECORRIDOR;

extern "C"
JNIEXPORT void JNICALL
Java_com_example_player_MainActivity_startPlayer(JNIEnv *env, jobject thiz, jobject surface,
                                                 jstring url) {
    const char *curl = env->GetStringUTFChars(url, nullptr);
    AVFormatContext *pAvFormatContext = avformat_alloc_context();
    //打开输入流
    if (avformat_open_input(&pAvFormatContext, curl, nullptr, nullptr) != 0) {
        LOGE("open fail");
        return;
    }
    //获取视频流
    if (avformat_find_stream_info(pAvFormatContext, nullptr) < 0) {
        LOGE("find info fail");
    }
    int streamIndex = -1;
//4.获取音视频流索引
    for (int i = 0; i < pAvFormatContext->nb_streams; i++) {
        if (pAvFormatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            streamIndex = i;
            break;
        }
    }

    if (streamIndex == -1) {
        LOGE("fail to find index");
        return;
    }

//5.获取解码器参数
    AVCodecParameters *codecParameters = pAvFormatContext->streams[streamIndex]->codecpar;

//6.根据 codec_id 获取解码器
    const AVCodec *pCodec = avcodec_find_decoder(codecParameters->codec_id);
    if (pCodec == nullptr) {
        LOGE("avcodec_find_decoder fail.");
        return;
    }

//7.创建解码器上下文
    AVCodecContext *pAvCodecContext = avcodec_alloc_context3(pCodec);
    if (avcodec_parameters_to_context(pAvCodecContext, codecParameters) != 0) {
        LOGE("avcodec_parameters_to_context fail.");
        return;
    }

//8.打开解码器
    int result = avcodec_open2(pAvCodecContext, pCodec, NULL);
    if (result < 0) {
        LOGE("avcodec_open2 fail. result=%d", result);
        return;
    }


//9.创建存储编码数据和解码数据的结构体
    AVPacket *pAvPacket = av_packet_alloc(); //创建 AVPacket 存放编码数据
    AVFrame *pAvFrame = av_frame_alloc(); //创建 AVFrame 存放解码后的数据


    //1. 分配存储 RGB 图像的 buffer
    int m_VideoWidth = pAvCodecContext->width;
    int m_VideoHeight = pAvCodecContext->height;
    AVFrame *m_RGBAFrame = av_frame_alloc();

    // R4 初始化 Native Window 用于播放视频
    ANativeWindow *native_window = ANativeWindow_fromSurface(env, surface);
    if (native_window == nullptr) {
        LOGE("Player Error : Can not create native window");
        return;
    }
    // 通过设置宽高限制缓冲区中的像素数量，而非屏幕的物理显示尺寸。
    // 如果缓冲区与物理屏幕的显示尺寸不相符，则实际显示可能会是拉伸，或者被压缩的图像
    result = ANativeWindow_setBuffersGeometry(native_window, m_VideoWidth, m_VideoHeight * 2,
                                              WINDOW_FORMAT_RGBA_8888);
    if (result < 0) {
        LOGE("Player Error : Can not set native window buffer");
        ANativeWindow_release(native_window);
        return;
    }



    //计算 Buffer 的大小
    int bufferSize = av_image_get_buffer_size(AV_PIX_FMT_RGBA, m_VideoWidth, m_VideoHeight, 1);
    //为 m_RGBAFrame 分配空间
    auto *m_FrameBuffer = (uint8_t *) av_malloc(bufferSize * sizeof(uint8_t));
    av_image_fill_arrays(m_RGBAFrame->data, m_RGBAFrame->linesize, m_FrameBuffer, AV_PIX_FMT_RGBA,
                         m_VideoWidth, m_VideoHeight, 1);

    //获取转换的上下文
    SwsContext *m_SwsContext = sws_getContext(m_VideoWidth, m_VideoHeight, pAvCodecContext->pix_fmt,
                                              m_VideoWidth, m_VideoHeight, AV_PIX_FMT_RGBA,
                                              SWS_FAST_BILINEAR, NULL, NULL, NULL);


//10.解码循环
    while (av_read_frame(pAvFormatContext, pAvPacket) >= 0) { //读取帧
        if (pAvPacket->stream_index == streamIndex) {
            if (avcodec_send_packet(pAvCodecContext, pAvPacket) != 0) { //视频解码
                return;
            }
            while (avcodec_receive_frame(pAvCodecContext, pAvFrame) == 0) {
                //获取到 pAvFrame 解码数据，在这里进行格式转换，然后进行渲染，下一节介绍 ANativeWindow 渲染过程
                //3. 格式转换
                sws_scale(m_SwsContext, pAvFrame->data, pAvFrame->linesize, 0, m_VideoHeight,
                          m_RGBAFrame->data, m_RGBAFrame->linesize);
                // 定义绘图缓冲区
                ANativeWindow_Buffer m_NativeWindowBuffer;

                //锁定当前 Window ，获取屏幕缓冲区 Buffer 的指针
                ANativeWindow_lock(native_window, &m_NativeWindowBuffer, nullptr);
                uint8_t *dstBuffer = static_cast<uint8_t *>(m_NativeWindowBuffer.bits);

                int srcLineSize = m_RGBAFrame->linesize[0];//输入图的步长（一行像素有多少字节）
                int dstLineSize = m_NativeWindowBuffer.stride * 4;//RGBA 缓冲区步长

                for (int i = 0; i < m_VideoHeight; ++i) {
                    //一行一行地拷贝图像数据
                    memcpy(dstBuffer + i * dstLineSize, m_FrameBuffer + i * srcLineSize,
                           srcLineSize);
                }
                //解锁当前 Window ，渲染缓冲区数据
                ANativeWindow_unlockAndPost(native_window);

            }
        }
        av_packet_unref(pAvPacket); //释放 pAvPacket 引用，防止内存泄漏
    }


    //11.释放资源，解码完成
    if (native_window)
        ANativeWindow_release(native_window);
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
    if (pAvFrame != nullptr) {
        av_frame_free(&pAvFrame);
        pAvFrame = nullptr;
    }

    if (pAvPacket != nullptr) {
        av_packet_free(&pAvPacket);
        pAvPacket = nullptr;
    }

    if (pAvCodecContext != nullptr) {
        avcodec_close(pAvCodecContext);
        avcodec_free_context(&pAvCodecContext);
        pAvCodecContext = nullptr;
        pCodec = nullptr;
    }

    if (pAvFormatContext != nullptr) {
        avformat_close_input(&pAvFormatContext);
        avformat_free_context(pAvFormatContext);
        pAvFormatContext = nullptr;
    }
}


//播放器的 callback
void AudioPlayerCallback(SLAndroidSimpleBufferQueueItf pBufferQueueItf, void *context) {
    AudioFrame *audioFrame = audioFrameQueue.front();
    LOGD("aaa");
    if (audioFrame != nullptr) {
        SLresult result = (*pBufferQueueItf)->Enqueue(pBufferQueueItf, audioFrame->data,
                                                      (SLuint32) audioFrame->dataSize);
        if (result == SL_RESULT_SUCCESS) {
            LOGD("success");
            audioFrameQueue.pop();
            delete audioFrame;
        }
    }
}
void audioTest(JNIEnv
               *env,
               jobject thiz,
               const char* curl) {
    AVFormatContext *pAvFormatContext = avformat_alloc_context();
    if (
            avformat_open_input(&pAvFormatContext, curl,
                                nullptr, nullptr) != 0) {
        LOGE("open fail");
        return;
    }
    if (
            avformat_find_stream_info(pAvFormatContext,
                                      nullptr) < 0) {
        LOGE("find info fail");
        return;
    }

    int streamIndex = -1;
//4.获取音视频流索引
    for (
            int i = 0;
            i < pAvFormatContext->
                    nb_streams;
            i++) {
        LOGI("codec type%d", pAvFormatContext->streams[i]->codecpar->codec_type);
        if (pAvFormatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            streamIndex = i;
            break;
        }
    }

    if (streamIndex == -1) {
        LOGE("cannot find stream");
        return;
    }

//5.获取解码器参数
    AVCodecParameters *codecParameters = pAvFormatContext->streams[streamIndex]->codecpar;

//6.根据 codec_id 获取解码器
    const AVCodec *pCodec = avcodec_find_decoder(codecParameters->codec_id);
    if (pCodec == nullptr) {
        LOGE("avcodec_find_decoder fail.");
        return;
    }

//7.创建解码器上下文
    AVCodecContext *pAvCodecContext = avcodec_alloc_context3(pCodec);
    if (
            avcodec_parameters_to_context(pAvCodecContext, codecParameters
            ) != 0) {
        LOGE("avcodec_parameters_to_context fail.");
        return;
    }


    if (
            avcodec_open2(pAvCodecContext, pCodec,
                          nullptr) < 0) {
        LOGE("open codec fail");
        return;
    }
//音频重采样
    SwrContext *pSwrContext = swr_alloc();
    av_opt_set_int(pSwrContext,
                   "in_channel_layout", pAvCodecContext->channel_layout, 0);
    av_opt_set_int(pSwrContext,
                   "out_channel_layout", AUDIO_DST_CHANNEL_LAYOUT, 0);
    av_opt_set_int(pSwrContext,
                   "in_sample_rate", pAvCodecContext->sample_rate, 0);
    av_opt_set_int(pSwrContext,
                   "out_sample_rate", AUDIO_DST_SAMPLE_RATE, 0);
    av_opt_set_sample_fmt(pSwrContext,
                          "in_sample_fmt", pAvCodecContext->sample_fmt, 0);
    av_opt_set_sample_fmt(pSwrContext,
                          "out_sample_fmt", DST_SAMPLE_FORMAT, 0);

    swr_init(pSwrContext);


//2. 申请输出 Buffer
    int nbSamples = (int) av_rescale_rnd(ACC_NB_SAMPLES, AUDIO_DST_SAMPLE_RATE,
                                         pAvCodecContext->sample_rate, AV_ROUND_UP);
    int DstFrameDataSize = av_samples_get_buffer_size(nullptr, AUDIO_DST_CHANNEL_COUNTS, nbSamples,
                                                      DST_SAMPLE_FORMAT, 1);
    auto *audioOutBuffer = (uint8_t *) malloc(DstFrameDataSize);


    AVPacket *pAvPacket = av_packet_alloc(); //创建 AVPacket 存放编码数据
    AVFrame *pAvFrame = av_frame_alloc(); //创建 AVFrame 存放解码后的数据

//10.解码循环
    while (
            av_read_frame(pAvFormatContext, pAvPacket
            ) >= 0) { //读取帧
        if (pAvPacket->stream_index == streamIndex) {
            if (
                    avcodec_send_packet(pAvCodecContext, pAvPacket
                    ) == AVERROR_EOF) {
//解码结束
                return;
            }
            while (
                    avcodec_receive_frame(pAvCodecContext, pAvFrame
                    ) == 0) {
//获取到 pAvFrame 解码数据，在这里进行格式转换
//3. 重采样，frame 为解码帧
                int convert_result = swr_convert(
                        pSwrContext,
                        &audioOutBuffer, DstFrameDataSize / 2,
                        (const uint8_t **) pAvFrame->data, pAvFrame->nb_samples);
                if (convert_result > 0) {
//play
                    auto *audioFrame = new AudioFrame(audioOutBuffer, DstFrameDataSize);
                    audioFrameQueue.
                            push(audioFrame);
                }
            }
        }
        av_packet_unref(pAvPacket); //释放 pAvPacket 引用，防止内存泄漏
    }


    SLresult result;

// 创建引擎对象
    result = slCreateEngine(&engineObject, 0, nullptr, 0, nullptr, nullptr);
    assert(SL_RESULT_SUCCESS == result);

// 实例化
    result = (*engineObject)->Realize(engineObject, SL_BOOLEAN_FALSE);
    assert(SL_RESULT_SUCCESS == result);


// 获取引擎对象接口
    result = (*engineObject)->GetInterface(engineObject, SL_IID_ENGINE, &engineEngine);
    assert(SL_RESULT_SUCCESS == result);

//创建混音器
    const SLInterfaceID mids[1] = {SL_IID_ENVIRONMENTALREVERB};
    const SLboolean mreq[1] = {SL_BOOLEAN_FALSE};

    result = (*engineEngine)->CreateOutputMix(engineEngine, &outputMixObj, 1, mids, mreq);
    if (result != SL_RESULT_SUCCESS) {
        LOGE("CreateOutputMix fail. result=%d", result);
        return;
    }
    result = (*outputMixObj)->Realize(outputMixObj, SL_BOOLEAN_FALSE);
    if (result != SL_RESULT_SUCCESS) {
        LOGE("OpenSLRender::CreateOutputMixer CreateOutputMix fail. result=%d", result);
        return;
    }

    result = (*outputMixObj)->GetInterface(outputMixObj, SL_IID_ENVIRONMENTALREVERB,
                                           &outputMixEnvironmentalReverb);
    if (SL_RESULT_SUCCESS == result) {
        result = (*outputMixEnvironmentalReverb)->SetEnvironmentalReverbProperties(
                outputMixEnvironmentalReverb, &reverbSettings);
    }
//创建播放器
    SLDataLocator_AndroidSimpleBufferQueue android_queue = {SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE,
                                                            2};

    SLDataFormat_PCM pcm = {
            SL_DATAFORMAT_PCM,//format type
            (SLuint32) 2,//channel count
            SL_SAMPLINGRATE_48,//48000hz
            SL_PCMSAMPLEFORMAT_FIXED_16,// bits per sample
            SL_PCMSAMPLEFORMAT_FIXED_16,// container size
            SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT,// channel mask
            SL_BYTEORDER_LITTLEENDIAN // endianness
    };
    SLDataSource slDataSource = {&android_queue, &pcm};
    SLDataLocator_OutputMix outputMix = {SL_DATALOCATOR_OUTPUTMIX, outputMixObj};
    SLDataSink slDataSink = {&outputMix, nullptr};

    const SLInterfaceID ids[3] = {SL_IID_BUFFERQUEUE, SL_IID_EFFECTSEND, SL_IID_VOLUME};
    const SLboolean req[3] = {SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE};
//正式创建播放器
    SLObjectItf audioPlayerObj;
    if (
            (*engineEngine)->
                    CreateAudioPlayer(engineEngine, &audioPlayerObj, &slDataSource,
                                      &slDataSink,
                                      3, ids, req) != SL_RESULT_SUCCESS) {
        LOGE("OpenSLRender::CreateAudioPlayer CreateAudioPlayer fail. result=%d", result);
        return;
    }

    if ((*audioPlayerObj)->
            Realize(audioPlayerObj, SL_BOOLEAN_FALSE) != SL_RESULT_SUCCESS) {
        LOGE("OpenSLRender::CreateAudioPlayer Realize fail. result=%d", result);
        return;
    }
    result = (*audioPlayerObj)->GetInterface(audioPlayerObj, SL_IID_PLAY, &audioPlayerPlay);
    if (result != SL_RESULT_SUCCESS) {
        LOGE("OpenSLRender::CreateAudioPlayer GetInterface fail. result=%d", result);
        return;
    }
    result = (*audioPlayerObj)->GetInterface(audioPlayerObj, SL_IID_BUFFERQUEUE,
                                             &bufferQueue);
    if (result != SL_RESULT_SUCCESS) {
        LOGE("OpenSLRender::CreateAudioPlayer GetInterface fail. result=%d", result);
        return;
    }
    result = (*audioPlayerObj)->GetInterface(audioPlayerObj, SL_IID_VOLUME, &audioPlayerVolume);
    if (result != SL_RESULT_SUCCESS) {
        LOGE("OpenSLRender::CreateAudioPlayer GetInterface fail. result=%d", result);
        return;
    }

//注册回调
    result = (*bufferQueue)->RegisterCallback(bufferQueue, AudioPlayerCallback, nullptr);
    if (result != SL_RESULT_SUCCESS) {
        LOGE("OpenSLRender::CreateAudioPlayer RegisterCallback fail. result=%d", result);
        return;
    }

//设置播放状态
    result = (*audioPlayerPlay)->SetPlayState(audioPlayerPlay, SL_PLAYSTATE_PLAYING);
    if (result != SL_RESULT_SUCCESS) {
        LOGE(" SetPlayState  error");
        return;
    }

    AudioPlayerCallback(bufferQueue,
                        nullptr);

//4. 释放资源
// 释放引擎对象的资源
// (*engineObject)->Destroy(engineObject);
    if (audioOutBuffer) {
        free(audioOutBuffer);
        audioOutBuffer = nullptr;
    }

    if (pSwrContext) {
        swr_free(&pSwrContext);
        pSwrContext = nullptr;
    }
}

extern "C"
JNIEXPORT void JNICALL
Java_com_example_player_MainActivity_startAudio(JNIEnv *env, jobject thiz, jstring url) {
    const char *curl = env->GetStringUTFChars(url, nullptr);
    new std::thread(audioTest, env, thiz, curl);

}
