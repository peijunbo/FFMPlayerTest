#include "VideoGLRender.h"
#include "VideoDecoder.h"
#include <jni.h>
VideoDecoder* videoDecoder;

extern "C"
JNIEXPORT jlong JNICALL
Java_com_example_player_MyGLSurfaceView_00024Companion_00024MyGLRender_native_1Init(JNIEnv *env,
                                                                                    jobject thiz,
                                                                                    jstring url,
                                                                                    jint player_type,
                                                                                    jint render_type,
                                                                                    jobject surface) {
    LOGE();
    const char* curl = env->GetStringUTFChars(url, nullptr);
    videoDecoder = new VideoDecoder(curl);
    videoDecoder->SetVideoRender(VideoGLRender::GetInstance());
    return 0;
}


extern "C"
JNIEXPORT void JNICALL
Java_com_example_player_MyGLSurfaceView_00024Companion_00024MyGLRender_native_1OnSurfaceCreated(
        JNIEnv *env, jobject thiz, jint render_type) {
    LOGE();
    VideoGLRender::GetInstance()->OnSurfaceCreated();
}


extern "C"
JNIEXPORT void JNICALL
Java_com_example_player_MyGLSurfaceView_00024Companion_00024MyGLRender_native_1OnSurfaceChanged(
        JNIEnv *env, jobject thiz, jint render_type, jint width, jint height) {
    LOGE();
    VideoGLRender::GetInstance()->OnSurfaceChanged(width, height);
}


extern "C"
JNIEXPORT void JNICALL
Java_com_example_player_MyGLSurfaceView_00024Companion_00024MyGLRender_native_1OnDrawFrame(
        JNIEnv *env, jobject thiz, jint render_type) {
    VideoGLRender::GetInstance()->OnDrawFrame();
}
extern "C"
JNIEXPORT void JNICALL
Java_com_example_player_MyGLSurfaceView_00024Companion_00024MyGLRender_native_1Play(JNIEnv *env,
                                                                                    jobject thiz,
                                                                                    jlong player_handle) {
    if (videoDecoder != nullptr) {
        videoDecoder->Start();
    }
}