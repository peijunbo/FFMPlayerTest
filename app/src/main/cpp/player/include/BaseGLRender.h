//
// Created on 2023/5/4.
//

#ifndef PLAYER_BASEGLRENDER_H
#define PLAYER_BASEGLRENDER_H
//gl render type
#define VIDEO_GL_RENDER     0
#define AUDIO_GL_RENDER     1
#define VR_3D_GL_RENDER     2
#define MATH_PI 3.1415926535897932384626433832802
#define TEXTURE_NUM 3
struct TransformMatrix {
    int degree;
    int mirror;
    float translateX;
    float translateY;
    float scaleX;
    float scaleY;
    int angleX;
    int angleY;

    TransformMatrix():
            translateX(0),
            translateY(0),
            scaleX(1.0),
            scaleY(1.0),
            degree(0),
            mirror(0),
            angleX(0),
            angleY(0)
    {

    }
    void Reset()
    {
        translateX = 0;
        translateY = 0;
        scaleX = 1.0;
        scaleY = 1.0;
        degree = 0;
        mirror = 0;

    }
};

class BaseGLRender {
public:
    virtual ~BaseGLRender(){}
    // 对应GLSurfaceView的三个方法
    virtual void OnSurfaceCreated() = 0;
    virtual void OnSurfaceChanged(int w, int h) = 0;
    virtual void OnDrawFrame() = 0;

    virtual void UpdateMVPMatrix(int angleX, int angleY, float scaleX, float scaleY) = 0;

    virtual void UpdateMVPMatrix(TransformMatrix * pTransformMatrix) {}

    virtual void SetTouchLoc(float touchX, float touchY) = 0;
};

#endif //PLAYER_BASEGLRENDER_H
