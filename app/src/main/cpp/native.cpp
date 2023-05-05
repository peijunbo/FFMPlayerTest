#include "CubeGLRender.h"
#include "VideoDecoder.h"
#include <jni.h>

static VideoDecoder *videoDecoder;
CubeGLRender *videoGlRender = new CubeGLRender;


extern "C"
JNIEXPORT jlong JNICALL
Java_com_example_player_MyGLSurfaceView_00024MyGLRender_native_1Init(JNIEnv *env,
                                                                     jobject thiz,
                                                                     jstring url,
                                                                     jint player_type,
                                                                     jint render_type,
                                                                     jobject surface) {
    LOGE("player init id %d", render_type);
    const char *curl = env->GetStringUTFChars(url, nullptr);
    videoDecoder = new VideoDecoder(curl);
    videoDecoder->SetDecodeToRGBA(true);
    videoDecoder->SetVideoRender(videoGlRender);
    return 0;
}


extern "C"
JNIEXPORT void JNICALL
Java_com_example_player_MyGLSurfaceView_00024MyGLRender_native_1OnSurfaceCreated(
        JNIEnv *env, jobject thiz, jint render_type) {
    LOGE();

    videoGlRender->OnSurfaceCreated();
}


extern "C"
JNIEXPORT void JNICALL
Java_com_example_player_MyGLSurfaceView_00024MyGLRender_native_1OnSurfaceChanged(
        JNIEnv *env, jobject thiz, jint render_type, jint width, jint height) {
    LOGE();
    videoGlRender->OnSurfaceChanged(width, height);
}


extern "C"
JNIEXPORT void JNICALL
Java_com_example_player_MyGLSurfaceView_00024MyGLRender_native_1OnDrawFrame(
        JNIEnv *env, jobject thiz, jint render_type) {
    videoGlRender->OnDrawFrame();
}
extern "C"
JNIEXPORT void JNICALL
Java_com_example_player_MyGLSurfaceView_00024MyGLRender_native_1Play(JNIEnv *env,
                                                                     jobject thiz,
                                                                     jint player_handle) {
    LOGD("play playerid %d", player_handle);
    if (videoDecoder != nullptr) {
        videoDecoder->Start();
        LOGD("1 start");
    }
}

extern "C"
JNIEXPORT void JNICALL
Java_com_example_player_MyGLSurfaceView_00024MyGLRender_native_1SeekTo(JNIEnv *env, jobject thiz,
                                                                       jint destination) {

}