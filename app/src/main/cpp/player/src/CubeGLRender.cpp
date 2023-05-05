//
// Created on 2023/5/4.
//
#include "CubeGLRender.h"
#include "GLUtils.h"
#include "gtc/matrix_transform.hpp"
extern "C" {
#include "libavutil/time.h"
}
CubeGLRender* CubeGLRender::s_Instance = nullptr;
std::mutex CubeGLRender::m_Mutex;

static char vCubeShaderStr[] =
        "#version 300 es\n"
        "layout(location = 0) in vec3 a_position;\n"
        "layout(location = 1) in vec2 a_texCoord;\n"
        "uniform mat4 projection;\n"
        "uniform mat4 view;\n"
        "uniform mat4 model;\n"
        "out vec2 v_texCoord;\n"
        "void main()\n"
        "{\n"
        "    gl_Position = projection * view * model * vec4(a_position, 1.0);\n"
        "    v_texCoord = a_texCoord;\n"
        "}";

static char fShaderStr[] =
        "#version 300 es\n"
        "precision highp float;\n"
        "in vec2 v_texCoord;\n"
        "layout(location = 0) out vec4 outColor;\n"
        "uniform sampler2D s_texture0;\n"
        "uniform sampler2D s_texture1;\n"
        "uniform sampler2D s_texture2;\n"
        "uniform int u_nImgType;// 1:RGBA, 2:NV21, 3:NV12, 4:I420\n"
        "\n"
        "void main()\n"
        "{\n"
        "\n"
        "    if(u_nImgType == 1) //RGBA\n"
        "    {\n"
        "        outColor = texture(s_texture0, v_texCoord);\n"
        "    }\n"
        "    else if(u_nImgType == 2) //NV21\n"
        "    {\n"
        "        vec3 yuv;\n"
        "        yuv.x = texture(s_texture0, v_texCoord).r;\n"
        "        yuv.y = texture(s_texture1, v_texCoord).a - 0.5;\n"
        "        yuv.z = texture(s_texture1, v_texCoord).r - 0.5;\n"
        "        highp vec3 rgb = mat3(1.0,       1.0,     1.0,\n"
        "        0.0, \t-0.344, \t1.770,\n"
        "        1.403,  -0.714,     0.0) * yuv;\n"
        "        outColor = vec4(rgb, 1.0);\n"
        "\n"
        "    }\n"
        "    else if(u_nImgType == 3) //NV12\n"
        "    {\n"
        "        vec3 yuv;\n"
        "        yuv.x = texture(s_texture0, v_texCoord).r;\n"
        "        yuv.y = texture(s_texture1, v_texCoord).r - 0.5;\n"
        "        yuv.z = texture(s_texture1, v_texCoord).a - 0.5;\n"
        "        highp vec3 rgb = mat3(1.0,       1.0,     1.0,\n"
        "        0.0, \t-0.344, \t1.770,\n"
        "        1.403,  -0.714,     0.0) * yuv;\n"
        "        outColor = vec4(rgb, 1.0);\n"
        "    }\n"
        "    else if(u_nImgType == 4) //I420\n"
        "    {\n"
        "        vec3 yuv;\n"
        "        yuv.x = texture(s_texture0, v_texCoord).r;\n"
        "        yuv.y = texture(s_texture1, v_texCoord).r - 0.5;\n"
        "        yuv.z = texture(s_texture2, v_texCoord).r - 0.5;\n"
        "        highp vec3 rgb = mat3(1.0,       1.0,     1.0,\n"
        "                              0.0, \t-0.344, \t1.770,\n"
        "                              1.403,  -0.714,     0.0) * yuv;\n"
        "        outColor = vec4(rgb, 1.0);\n"
        "    }\n"
        "    else\n"
        "    {\n"
        "        outColor = vec4(1.0);\n"
        "    }\n"
        "}";

//
//GLfloat verticesCoords[] = {
//        -1.0f,  1.0f, 0.0f,  // Position 0
//        -1.0f, -1.0f, 0.0f,  // Position 1
//        1.0f,  -1.0f, 0.0f,  // Position 2
//        1.0f,   1.0f, 0.0f,  // Position 3
//};

GLfloat vertices[] = {
        -0.5f, -0.5f, -0.5f,  0.0f, 0.0f,
        0.5f, -0.5f, -0.5f,  1.0f, 0.0f,
        0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
        0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
        -0.5f,  0.5f, -0.5f,  0.0f, 1.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, 0.0f,

        -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
        0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
        0.5f,  0.5f,  0.5f,  1.0f, 1.0f,
        0.5f,  0.5f,  0.5f,  1.0f, 1.0f,
        -0.5f,  0.5f,  0.5f,  0.0f, 1.0f,
        -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,

        -0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
        -0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
        -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
        -0.5f,  0.5f,  0.5f,  1.0f, 0.0f,

        0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
        0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
        0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
        0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
        0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
        0.5f,  0.5f,  0.5f,  1.0f, 0.0f,

        -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
        0.5f, -0.5f, -0.5f,  1.0f, 1.0f,
        0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
        0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
        -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,

        -0.5f,  0.5f, -0.5f,  0.0f, 1.0f,
        0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
        0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
        0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
        -0.5f,  0.5f,  0.5f,  0.0f, 0.0f,
        -0.5f,  0.5f, -0.5f,  0.0f, 1.0f
};

//GLfloat textureCoords[] = {
//        0.0f,  0.0f,        // TexCoord 0
//        0.0f,  1.0f,        // TexCoord 1
//        1.0f,  1.0f,        // TexCoord 2
//        1.0f,  0.0f         // TexCoord 3
//};

//GLushort indices[] = { 0, 1, 2, 0, 2, 3 };

CubeGLRender::CubeGLRender():VideoRender(VIDEO_RENDER_OPENGL) {

}

CubeGLRender::~CubeGLRender() {
    NativeImageUtil::FreeNativeImage(&m_RenderImage);

}

void CubeGLRender::Init(int videoWidth, int videoHeight, int *dstSize) {
    if(dstSize != nullptr) {
        dstSize[0] = videoWidth / 2;
        dstSize[1] = videoHeight / 2;
    }
    m_FrameIndex = 0;
    UpdateMVPMatrix(0, 0, 1.0f, 1.0f);
}

void CubeGLRender::RenderVideoFrame(NativeImage *pImage) {
    if(pImage == nullptr || pImage->ppPlane[0] == nullptr)
        return;
    std::unique_lock<std::mutex> lock(m_Mutex);
    if (pImage->width != m_RenderImage.width || pImage->height != m_RenderImage.height) {
        if (m_RenderImage.ppPlane[0] != nullptr) {
            NativeImageUtil::FreeNativeImage(&m_RenderImage);
        }
        memset(&m_RenderImage, 0, sizeof(NativeImage));
        m_RenderImage.format = pImage->format;
        m_RenderImage.width = pImage->width;
        m_RenderImage.height = pImage->height;
        NativeImageUtil::AllocNativeImage(&m_RenderImage);
    }

    NativeImageUtil::CopyNativeImage(pImage, &m_RenderImage);
    //NativeImageUtil::DumpNativeImage(&m_RenderImage, "/sdcard", "camera");
}

void CubeGLRender::UnInit() {

}

void CubeGLRender::UpdateMVPMatrix(int angleX, int angleY, float scaleX, float scaleY)
{
    angleX = angleX % 360;
    angleY = angleY % 360;

    //转化为弧度角
    float radiansX = static_cast<float>(MATH_PI / 180.0f * angleX);
    float radiansY = static_cast<float>(MATH_PI / 180.0f * angleY);
    // Projection matrix
    glm::mat4 Projection = glm::ortho(-1.0f, 1.0f, -1.0f, 1.0f, 0.1f, 100.0f);
    //glm::mat4 Projection = glm::frustum(-ratio, ratio, -1.0f, 1.0f, 4.0f, 100.0f);
    //glm::mat4 Projection = glm::perspective(45.0f,ratio, 0.1f,100.f);
    // View matrix
    glm::mat4 View = glm::lookAt(
            glm::vec3(0, 0, 4), // Camera is at (0,0,1), in World Space
            glm::vec3(0, 0, 0), // and looks at the origin
            glm::vec3(0, 1, 0)  // Head is up (set to 0,-1,0 to look upside-down)
    );

    // Model matrix
    glm::mat4 Model = glm::mat4(1.0f);
    Model = glm::scale(Model, glm::vec3(scaleX, scaleY, 1.0f));
    Model = glm::rotate(Model, radiansX, glm::vec3(1.0f, 0.0f, 0.0f));
    Model = glm::rotate(Model, radiansY, glm::vec3(0.0f, 1.0f, 0.0f));
    Model = glm::translate(Model, glm::vec3(0.0f, 0.0f, 0.0f));

    m_MVPMatrix = Projection * View * Model;

}

void CubeGLRender::UpdateMVPMatrix(TransformMatrix *pTransformMatrix) {
//    //BaseGLRender::UpdateMVPMatrix(pTransformMatrix);
//    float fFactorX = 1.0f;
//    float fFactorY = 1.0f;
//
//    if (pTransformMatrix->mirror == 1) {
//        fFactorX = -1.0f;
//    } else if (pTransformMatrix->mirror == 2) {
//        fFactorY = -1.0f;
//    }
//
//    float fRotate = MATH_PI * pTransformMatrix->degree * 1.0f / 180;
//    if (pTransformMatrix->mirror == 0) {
//        if (pTransformMatrix->degree == 270) {
//            fRotate = MATH_PI * 0.5;
//        } else if (pTransformMatrix->degree == 180) {
//            fRotate = MATH_PI;
//        } else if (pTransformMatrix->degree == 90) {
//            fRotate = MATH_PI * 1.5;
//        }
//    } else if (pTransformMatrix->mirror == 1) {
//        if (pTransformMatrix->degree == 90) {
//            fRotate = MATH_PI * 0.5;
//        } else if (pTransformMatrix->degree == 180) {
//            fRotate = MATH_PI;
//        } else if (pTransformMatrix->degree == 270) {
//            fRotate = MATH_PI * 1.5;
//        }
//    }
//
//    glm::mat4 Projection = glm::ortho(-1.0f, 1.0f, -1.0f, 1.0f, 0.0f, 1.0f);
//    glm::mat4 View = glm::lookAt(
//            glm::vec3(0, 0, 1), // Camera is at (0,0,1), in World Space
//            glm::vec3(0, 0, 0), // and looks at the origin
//            glm::vec3(0, 1, 0) // Head is up (set to 0,-1,0 to look upside-down)
//    );
//
//    // Model matrix : an identity matrix (model will be at the origin)
//    glm::mat4 Model = glm::mat4(1.0f);
//    Model = glm::scale(Model, glm::vec3(fFactorX * pTransformMatrix->scaleX,
//                                        fFactorY * pTransformMatrix->scaleY, 1.0f));
//    Model = glm::rotate(Model, fRotate, glm::vec3(0.0f, 0.0f, 1.0f));
//    Model = glm::translate(Model,
//                           glm::vec3(pTransformMatrix->translateX, pTransformMatrix->translateY, 0.0f));
//
//         pTransformMatrix->translateX, pTransformMatrix->translateY,
//         fFactorX * pTransformMatrix->scaleX, fFactorY * pTransformMatrix->scaleY);
//
//    m_MVPMatrix = Projection * View * Model;
}

void CubeGLRender::OnSurfaceCreated() {

    m_ProgramObj = GLUtils::CreateProgram(vCubeShaderStr, fShaderStr);
    if (!m_ProgramObj)
    {
        LOGE("CubeGLRender::OnSurfaceCreated create program fail");
        return;
    }
    // 开启深度测试
    glEnable(GL_DEPTH_TEST);

    glGenTextures(TEXTURE_NUM, m_TextureIds);
    for (int i = 0; i < TEXTURE_NUM ; ++i) {
        glActiveTexture(GL_TEXTURE0 + i);
        glBindTexture(GL_TEXTURE_2D, m_TextureIds[i]);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glBindTexture(GL_TEXTURE_2D, GL_NONE);
    }

//    // Generate VBO Ids and load the VBOs with data
//    glGenBuffers(3, m_VboIds);
//    glBindBuffer(GL_ARRAY_BUFFER, m_VboIds[0]);
//    glBufferData(GL_ARRAY_BUFFER, sizeof(verticesCoords), verticesCoords, GL_STATIC_DRAW);
//
//    glBindBuffer(GL_ARRAY_BUFFER, m_VboIds[1]);
//    glBufferData(GL_ARRAY_BUFFER, sizeof(textureCoords), textureCoords, GL_STATIC_DRAW);
//
//    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_VboIds[2]);
//    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);


    // Generate VAO Id
    glGenVertexArrays(1, &m_VaoId);
    glBindVertexArray(m_VaoId);
    //因为将定点数组改为了一个，所以只分配1个
    glGenBuffers(1, m_VboIds);
    glBindBuffer(GL_ARRAY_BUFFER, m_VboIds[0]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    // position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // texture coord attribute
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

//    glBindBuffer(GL_ARRAY_BUFFER, m_VboIds[0]);
//    glEnableVertexAttribArray(0);
//    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (const void *)0);
//    glBindBuffer(GL_ARRAY_BUFFER, GL_NONE);
//
//    glBindBuffer(GL_ARRAY_BUFFER, m_VboIds[1]);
//    glEnableVertexAttribArray(1);
//    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), (const void *)0);
//    glBindBuffer(GL_ARRAY_BUFFER, GL_NONE);
//
//    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_VboIds[2]);
//
    glBindVertexArray(GL_NONE);

    m_TouchXY = vec2(0.5f, 0.5f);
}

void CubeGLRender::OnSurfaceChanged(int w, int h) {
    LOGE("CubeGLRender::OnSurfaceChanged [w, h]=[%d, %d]", w, h);
    m_ScreenSize.x = w;
    m_ScreenSize.y = h;
    glViewport(0, 0, w, h);
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
}

void CubeGLRender::OnDrawFrame() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    if(m_ProgramObj == GL_NONE|| m_RenderImage.ppPlane[0] == nullptr) return;
    m_FrameIndex++;
//    if(m_FrameIndex == 2)
//        NativeImageUtil::DumpNativeImage(&m_RenderImage, "/sdcard", "2222");

    // upload image data
    std::unique_lock<std::mutex> lock(m_Mutex);
    switch (m_RenderImage.format)
    {
        case IMAGE_FORMAT_RGBA:
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, m_TextureIds[0]);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, m_RenderImage.width, m_RenderImage.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, m_RenderImage.ppPlane[0]);
            glBindTexture(GL_TEXTURE_2D, GL_NONE);
            break;
        case IMAGE_FORMAT_NV21:
        case IMAGE_FORMAT_NV12:
            //upload Y plane data
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, m_TextureIds[0]);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, m_RenderImage.width,
                         m_RenderImage.height, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE,
                         m_RenderImage.ppPlane[0]);
            glBindTexture(GL_TEXTURE_2D, GL_NONE);

            //update UV plane data
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, m_TextureIds[1]);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE_ALPHA, m_RenderImage.width >> 1,
                         m_RenderImage.height >> 1, 0, GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE,
                         m_RenderImage.ppPlane[1]);
            glBindTexture(GL_TEXTURE_2D, GL_NONE);
            break;
        case IMAGE_FORMAT_I420:
            //upload Y plane data
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, m_TextureIds[0]);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, m_RenderImage.width,
                         m_RenderImage.height, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE,
                         m_RenderImage.ppPlane[0]);
            glBindTexture(GL_TEXTURE_2D, GL_NONE);

            //update U plane data
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, m_TextureIds[1]);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, m_RenderImage.width >> 1,
                         m_RenderImage.height >> 1, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE,
                         m_RenderImage.ppPlane[1]);
            glBindTexture(GL_TEXTURE_2D, GL_NONE);

            //update V plane data
            glActiveTexture(GL_TEXTURE2);
            glBindTexture(GL_TEXTURE_2D, m_TextureIds[2]);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, m_RenderImage.width >> 1,
                         m_RenderImage.height >> 1, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE,
                         m_RenderImage.ppPlane[2]);
            glBindTexture(GL_TEXTURE_2D, GL_NONE);
            break;
        default:
            break;
    }
    lock.unlock();


    // Use the program object
    glUseProgram (m_ProgramObj);

    glBindVertexArray(m_VaoId);

    glm::mat4 view          = glm::mat4(1.0f); // make sure to initialize matrix to identity matrix first
    glm::mat4 projection    = glm::mat4(1.0f);
    glm::mat4 proj = glm::perspective(glm::radians(45.0f), (float)m_ScreenSize.x/(float)m_ScreenSize.y, 0.1f, 100.0f);
    // 注意，我们将矩阵向我们要进行移动场景的反方向移动。
    view = glm::translate(view, glm::vec3(0.0f, 0.0f, -3.0f));
    // 自定义旋转矩阵
    mat4 model = mat4(1.0f);

    float angle = (float)(av_gettime() / 10000 % 360) * radians(1.0f);
    model = rotate(model, angle, vec3(0.5f, 1.0f, 0.0f));
    GLUtils::setMat4(m_ProgramObj, "projection", proj);
    GLUtils::setMat4(m_ProgramObj, "view", view);
    GLUtils::setMat4(m_ProgramObj, "model", model);
    LOGD("pvm matrix angel%f srcw%f, srch%f", angle, m_ScreenSize.x, m_ScreenSize.y);
    for (int i = 0; i < TEXTURE_NUM; ++i) {
        glActiveTexture(GL_TEXTURE0 + i);
        glBindTexture(GL_TEXTURE_2D, m_TextureIds[i]);
        char samplerName[64] = {0};
        sprintf(samplerName, "s_texture%d", i);
        GLUtils::setInt(m_ProgramObj, samplerName, i);
    }
    GLUtils::setInt(m_ProgramObj, "u_nImgType", m_RenderImage.format);

    glDrawArrays(GL_TRIANGLES, 0, 36);

}

CubeGLRender *CubeGLRender::GetInstance() {
    if(s_Instance == nullptr)
    {
        std::lock_guard<std::mutex> lock(m_Mutex);
        s_Instance = new CubeGLRender();
    }
    return s_Instance;
}

void CubeGLRender::ReleaseInstance() {
    if(s_Instance != nullptr)
    {
        std::lock_guard<std::mutex> lock(m_Mutex);
        delete s_Instance;
        s_Instance = nullptr;

    }
}


