//
// Created on 2023/5/4.
//

#ifndef PLAYER_CUBEGLRENDER_H
#define PLAYER_CUBEGLRENDER_H
#include <thread>
#include "ImageDef.h"
#include "VideoRender.h"
#include "GLES3/gl3.h"
#include "glm.hpp"
#include "BaseGLRender.h"
using namespace glm;
class CubeGLRender: public VideoRender, public BaseGLRender{
public:
    CubeGLRender();
    virtual ~CubeGLRender();
    virtual void Init(int videoWidth, int videoHeight, int *dstSize);
    virtual void RenderVideoFrame(NativeImage *pImage);
    virtual void UnInit();

    virtual void OnSurfaceCreated();
    virtual void OnSurfaceChanged(int w, int h);
    virtual void OnDrawFrame();

    static CubeGLRender *GetInstance();
    static void ReleaseInstance();

    virtual void UpdateMVPMatrix(int angleX, int angleY, float scaleX, float scaleY);
    virtual void UpdateMVPMatrix(TransformMatrix * pTransformMatrix);
    virtual void SetTouchLoc(float touchX, float touchY) {
        m_TouchXY.x = touchX / m_ScreenSize.x;
        m_TouchXY.y = touchY / m_ScreenSize.y;
    }

private:
    static std::mutex m_Mutex;
    static CubeGLRender* s_Instance;
    GLuint m_ProgramObj = GL_NONE;
    GLuint m_AnotherProgramObj = GL_NONE;
    GLuint m_TextureIds[TEXTURE_NUM]{};
    GLuint m_VaoId{};
    GLuint m_VboIds[3]{};
    NativeImage m_RenderImage;
    glm::mat4 m_MVPMatrix{};

    int m_FrameIndex{};
    vec2 m_TouchXY{};
    vec2 m_ScreenSize{};
};
#endif //PLAYER_CUBEGLRENDER_H
