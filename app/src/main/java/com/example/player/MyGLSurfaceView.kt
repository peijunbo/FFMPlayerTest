package com.example.player

import android.content.Context
import android.opengl.GLSurfaceView
import android.util.AttributeSet
import android.util.Log
import javax.microedition.khronos.egl.EGLConfig
import javax.microedition.khronos.opengles.GL10

class MyGLSurfaceView(context: Context?, attrs: AttributeSet? = null) :
    GLSurfaceView(context, attrs) {
    private val mGLRender: MyGLRender

    init {
        setEGLContextClientVersion(3)
        mGLRender = MyGLRender()
        setRenderer(mGLRender)
        renderMode = RENDERMODE_CONTINUOUSLY
    }
    fun init(url: String) {
        mGLRender.native_Init(url, 0, 0, null)
    }
    fun play() {
        mGLRender.native_Play(0);
    }
    companion object {
        private const val TAG = "MyGLSurfaceView"

        //gl render type
        const val VIDEO_GL_RENDER = 0
        const val AUDIO_GL_RENDER = 1
        const val VR_3D_GL_RENDER = 2

        class MyGLRender : Renderer {
            override fun onSurfaceCreated(gl: GL10, config: EGLConfig) {
                Log.d(TAG, "onSurfaceCreated: ")
                native_OnSurfaceCreated(VIDEO_GL_RENDER)
            }

            override fun onSurfaceChanged(gl: GL10, width: Int, height: Int) {
                Log.d(
                    TAG,
                    "onSurfaceChanged() called with: gl = [$gl], width = [$width], height = [$height]"
                )
                native_OnSurfaceChanged(VIDEO_GL_RENDER, width, height)
            }

            override fun onDrawFrame(gl: GL10) {
                Log.d(TAG, "onDrawFrame() called with: gl = [$gl]")
                native_OnDrawFrame(VIDEO_GL_RENDER)
            }

            external fun native_Init(
                url: String,
                playerType: Int,
                renderType: Int,
                surface: Any?
            ): Long

            //for GL render
            private external fun native_OnSurfaceCreated(renderType: Int)
            private external fun native_OnSurfaceChanged(renderType: Int, width: Int, height: Int)
            private external fun native_OnDrawFrame(renderType: Int)
            external fun native_Play(playerHandle: Long)
            companion object {
                init {
                    System.loadLibrary("ffmpeg-player")
                }
            }
        }
    }
}