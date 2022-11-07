
#if defined(_MSC_VER)
// Make MS math.h define M_PI
#define _USE_MATH_DEFINES
#endif
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <glad/gl.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <linmath.h>
#include <SOIL2.h>
#include <cglm/cglm.h>   /* for inline */
#include <cglm/call.h>   /* for library call (this also includes cglm.h) */
#include "camera.h"
#include "game_state.h"


#define CHUNK_DIMENSION 1024
#define GL_TEXTURE_MAX_ANISOTROPY_EXT 0x84FE
#define GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT 0x84FF

#include "shader_helper.h"
mat4 perspective = GLM_MAT4_IDENTITY_INIT;
mat4 view = GLM_MAT4_IDENTITY_INIT;
mat4 model1 = GLM_MAT4_IDENTITY_INIT;
mat4 model2 = GLM_MAT4_IDENTITY_INIT;
mat4 model3 = GLM_MAT4_IDENTITY_INIT;

mat4 mvp = GLM_MAT4_IDENTITY_INIT;
int width = 800.0f, height = 600.0f;
GLuint cubemapShader;
GLuint waterShader;

GLuint cubemaptexture;
GLuint vao[3];

GLuint refractFrameBuffer;
GLuint reflectFrameBuffer;
GLuint refractTextureId;
GLuint reflectTextureId;

float* g_vertices;

bool lockcamerain = true;

GLuint loadTexture1(const char* texImagePath) {
    GLuint textureID = SOIL_load_OGL_texture(texImagePath,
        SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_INVERT_Y);
    if (textureID == 0) printf("could not find texture file %s", texImagePath);
    return textureID;
}
void loadTexture(const char* name) {
    GLuint bricktext = loadTexture1(name);
    glBindTexture(GL_TEXTURE_2D, bricktext);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glGenerateMipmap(GL_TEXTURE_2D);

    if (glewIsSupported("GL_EXT_texture_filter_anisotropic")) {
        GLfloat anisoSetting = 0.0f;
        glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &anisoSetting);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, anisoSetting);
    }
}

void loadTerrainTexture(const char* name) {
    GLuint bricktext = loadTexture1(name);
    glBindTexture(GL_TEXTURE_2D, bricktext);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glGenerateMipmap(GL_TEXTURE_2D);

    if (glewIsSupported("GL_EXT_texture_filter_anisotropic")) {
        GLfloat anisoSetting = 0.0f;
        glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &anisoSetting);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, anisoSetting);
    }
}

float **  PerlinNoise2D(int nOctaves, float fBias, int nWidth, int nHeight, float * fSeed)
{
    //float fOutput[][] = new float[nHeight][nWidth];
    float ** perlinNoise = (float**)malloc(sizeof(float*) * nHeight);
    int i = 0;
    for (i = 0; i < nHeight; i++) {
        perlinNoise[i] = (float*)malloc(sizeof(float) * nWidth);
    }

    //Used 1D Perlin Noise
    int x = 0; int y = 0; int o = 0;
    for (x = 0; x < nWidth; x++)
    {
        for (y = 0; y < nHeight; y++)
        {
            float fNoise = 0.0f;
            float fScaleAcc = 0.0f;
            float fScale = 1.0f;

            for (o = 0; o < nOctaves; o++)
            {
                int nPitch = (int)(((long)nWidth) >> o);
                int nSampleX1 = (x / nPitch) * nPitch;
                int nSampleY1 = (y / nPitch) * nPitch;

                int nSampleX2 = (nSampleX1 + nPitch) % nWidth;
                int nSampleY2 = (nSampleY1 + nPitch) % nWidth;

                float fBlendX = (float)(x - nSampleX1) / (float)nPitch;
                float fBlendY = (float)(y - nSampleY1) / (float)nPitch;

                float fSampleT = (1.0f - fBlendX) * fSeed[nSampleY1 * nWidth + nSampleX1] + fBlendX * fSeed[nSampleY1 * nWidth + nSampleX2];
                float fSampleB = (1.0f - fBlendX) * fSeed[nSampleY2 * nWidth + nSampleX1] + fBlendX * fSeed[nSampleY2 * nWidth + nSampleX2];

                fScaleAcc += fScale;
                fNoise += (fBlendY * (fSampleB - fSampleT) + fSampleT) * fScale;
                fScale = fScale / fBias;

            }
            perlinNoise[x][y] = fNoise / fScaleAcc;

        }
    }
    return perlinNoise;
}

float grabHeightVal(int i, int j) {
    if(j < 0 || j >= 1024 || i < 0 || i >= 1024)
        return 0;
    return ((island_info[i * 1024 + j] / 155)) * 2;
}

float* generateNormals(int dim, float width, float height)
{
    float* normals = (float*)malloc(sizeof(float) * dim * dim * 3);

    int i = 0; int j = 0;

    int index = 0;
    for (i = 0; i < dim; i++)
    {
        for (j = 0; j < dim; j++)
        {
            float heightL = grabHeightVal(i, j+1);
            float heightR = grabHeightVal(i, j-1);
            float heightD = grabHeightVal(i+1, j);
            float heightU = grabHeightVal(i-1, j);
            normals[index++] = heightR- heightL;
            normals[index++] = 2;
            normals[index++] = heightU- heightD;
        }
    }
    return normals;
}


float * generateVerticies(int dim, float width, float height)
{
    float * vertices = (float*)malloc(sizeof(float) * dim * dim * 3);

    int i = 0; int j = 0;

    int index = 0;
    for ( i = 0; i < dim; i++)
    {
        for (j = 0; j < dim; j++)
        {
            vertices[index++] = j * width*0.1*2;
            vertices[index++] = grabHeightVal(i, j);
            vertices[index++] = i * height * 0.1*2;
        }
    }
    return vertices;
}

unsigned int  * generateFaces(int dim)
{
    unsigned int* faces = (unsigned int*)malloc(sizeof(unsigned int) * dim * dim * 2 * 3);

    int j = 0;
    int** vertices = (unsigned int**) malloc(sizeof(unsigned int*) * dim);
    int i = 0;
    for (i = 0; i < dim; i++) {
        vertices[i] = (unsigned int*)malloc(sizeof(unsigned int) * dim);
    }

    //upside down triangles  |\|\|\| for each row
    //rightside up triangles |\|\|\| for each row
    int index = 0;

    for (i = 0; i < dim; i++)
        for (int j = 0; j < dim; j++)
            vertices[i][j] = index++;

    index = 0;
    for (i = 1; i < dim; i++)
    {
        for (j = 1; j < dim; j++)
        {
            //upside down traingle
            faces[index++] = vertices[i - 1][j - 1];
            faces[index++] = vertices[i][j - 1];
            faces[index++] = vertices[i - 1][j];

            //right side up triangle
            faces[index++] = vertices[i][j - 1];
            faces[index++] = vertices[i][j];
            faces[index++] = vertices[i - 1][j];
        }
    }

    for (i = 0; i < dim; i++) 
        free(vertices[i]);

    free(vertices);


    return faces;
}

GLfloat* generateTC()
{
    GLfloat* tc = (GLfloat*)malloc(sizeof(GLfloat*) * 2 * CHUNK_DIMENSION * CHUNK_DIMENSION);
    int index = 0;
    int i = 0; int j = 0;
    for (i = 0; i < CHUNK_DIMENSION; i++)
    {
        for (j = 0; j < CHUNK_DIMENSION; j++)
        {
            tc[index++] = i *0.1;
            tc[index++] = j * 0.1;
        }
    }
    return tc;
}

unsigned int loadCubemap()
{
    // cubemap
    GLuint m_texture = SOIL_load_OGL_cubemap
    (
        "res\\right.jpg",
        "res\\left.jpg",
        "res\\top.jpg",
        "res\\bottom.jpg",
        "res\\front.jpg",
        "res\\back.jpg",
        SOIL_LOAD_RGB,
        SOIL_CREATE_NEW_ID,
        SOIL_FLAG_MIPMAPS
    );
    return m_texture;
}


// setup VBO/VAO shit
GLuint init () {
    GLuint shaderProgram = createShader("shaders\\terrain_shader.vert", "shaders\\terrain_shader.frag");
    cubemapShader = createShader("shaders\\skybox_shader.vert", "shaders\\skybox_shader.frag");
    waterShader = createShader("shaders\\water_shader.vert", "shaders\\water_shader.frag");

    cubemaptexture = loadCubemap();

    GLfloat vc[] = {
        // front
        -1.0, -1.0,  1.0,
         1.0, -1.0,  1.0,
         1.0,  1.0,  1.0,
        -1.0,  1.0,  1.0,
        // back
        -1.0, -1.0, -1.0,
         1.0, -1.0, -1.0,
         1.0,  1.0, -1.0,
        -1.0,  1.0, -1.0
    };

    GLuint in[] = {
        // front
        0, 1, 2,
        2, 3, 0,
        // right
        1, 5, 6,
        6, 2, 1,
        // back
        7, 6, 5,
        5, 4, 7,
        // left
        4, 0, 3,
        3, 7, 4,
        // bottom
        4, 5, 1,
        1, 0, 4,
        // top
        3, 2, 6,
        6, 7, 3
    };


    GLfloat* vertices = generateVerticies(CHUNK_DIMENSION, 1, 1);
    GLuint* indices = generateFaces(CHUNK_DIMENSION);
    GLfloat* normals = generateNormals(CHUNK_DIMENSION,1 ,1);
    GLfloat* textureCoordinates = generateTC();

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

    GLuint vbo[7];

    glGenVertexArrays(3, vao);
    glGenBuffers(7, vbo);

    // cubemap vertex setup

    GLuint EBO[2];
    glGenBuffers(2, EBO);

    glBindVertexArray(vao[1]);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO[1]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(in), in, GL_STATIC_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vc), vc, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);
    GLuint VP = glGetAttribLocation(cubemapShader, "aPos");
    glVertexAttribPointer(VP, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
    glEnableVertexAttribArray(VP);

    // terrain vertex setup

    glBindVertexArray(vao[0]);
    // set up indices
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO[0]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLuint) * CHUNK_DIMENSION * CHUNK_DIMENSION * 2 * 3, indices, GL_STATIC_DRAW);
    free(indices);

    // load vertices
    glBindBuffer(GL_ARRAY_BUFFER, vbo[1]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * CHUNK_DIMENSION * CHUNK_DIMENSION * 3, vertices, GL_STATIC_DRAW);
    free(vertices);
    glBindBuffer(GL_ARRAY_BUFFER, vbo[1]);
    GLuint vertexPosition = glGetAttribLocation(shaderProgram, "aPos");
    glVertexAttribPointer(vertexPosition, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
    glEnableVertexAttribArray(vertexPosition);

    // load normals
    glBindBuffer(GL_ARRAY_BUFFER, vbo[2]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * CHUNK_DIMENSION * CHUNK_DIMENSION * 3, normals, GL_STATIC_DRAW);
    free(normals);
    glBindBuffer(GL_ARRAY_BUFFER, vbo[2]);
    GLuint normalsPosition = glGetAttribLocation(shaderProgram, "aNorm");
    glVertexAttribPointer(normalsPosition, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
    glEnableVertexAttribArray(normalsPosition);

    glActiveTexture(GL_TEXTURE0);
    loadTerrainTexture("res\\dirt.png");
    // load texture coordiantes
    glBindBuffer(GL_ARRAY_BUFFER, vbo[3]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * CHUNK_DIMENSION * CHUNK_DIMENSION * 2, textureCoordinates, GL_STATIC_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, vbo[3]);
    GLuint textureCoordinatePosition = glGetAttribLocation(shaderProgram, "tc");
    glVertexAttribPointer(textureCoordinatePosition, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);
    glEnableVertexAttribArray(textureCoordinatePosition);
    free(textureCoordinates);

    glBindVertexArray(vao[2]);
    float PLANE_POSITIONS[18] = {
        -128.0f, 0.0f, -128.0f, -128.0f, 0.0f, 128.0f, 128.0f, 0.0f, -128.0f,
        128.0f, 0.0f, -128.0f, -128.0f, 0.0f, 128.0f, 128.0f, 0.0f, 128.0f
    };

    float PLANE_NORMALS[18] = {
        0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f,0.0f, 1.0f, 0.0f,
        0.0f, 1.0f, 0.0f,0.0f, 1.0f, 0.0f,0.0f, 1.0f, 0.0f
    };
    float PLANE_TEXCOORDS[12] = {
    0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f
    };
    glBindBuffer(GL_ARRAY_BUFFER, vbo[4]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(PLANE_POSITIONS), PLANE_POSITIONS, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, vbo[4]);
    GLuint vpos = glGetAttribLocation(shaderProgram, "aPos");
    glVertexAttribPointer(vpos, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
    glEnableVertexAttribArray(vpos);

    glBindBuffer(GL_ARRAY_BUFFER, vbo[5]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(PLANE_TEXCOORDS), PLANE_POSITIONS, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, vbo[5]);
    GLuint tcpos = glGetAttribLocation(shaderProgram, "ftc");
    glVertexAttribPointer(tcpos, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);
    glEnableVertexAttribArray(tcpos);

    glBindBuffer(GL_ARRAY_BUFFER, vbo[6]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(PLANE_NORMALS), PLANE_NORMALS, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, vbo[6]);
    GLuint normPos = glGetAttribLocation(shaderProgram, "aNorm");
    glVertexAttribPointer(normPos, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
    glEnableVertexAttribArray(normPos);

    glmc_perspective(glm_rad(height*0.1f), (float)width / (float) height, 0.1f, 100.0f, &perspective);
    vec3 m1_translate = { -50, -5, 0.0 };
    glmc_translate(model1, m1_translate);

    vec3 m2_scale = {50, 50, 50 };
    //vec3 m2_translate = { 0, 0, 5};
    glmc_scale(model2, m2_scale);


    vec3 m3_trans = { 0, -3.5, 0 };
    glmc_translate(model3, m3_trans);
    glmc_scale(model3, m2_scale);

    return shaderProgram;
}

void createFrameBuffers(GLFWwindow * window) {
    GLuint bufferId[1];
    glGenBuffers(1, bufferId);
    glfwGetFramebufferSize(window, &width, &height);

    glGenFramebuffers(1, bufferId);
    refractFrameBuffer = bufferId[0];
    glBindFramebuffer(GL_FRAMEBUFFER, refractFrameBuffer);
    glGenTextures(1, bufferId); // this is for the color buffer
    refractTextureId = bufferId[0];
    glBindTexture(GL_TEXTURE_2D, refractTextureId);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, refractTextureId, 0);
    glDrawBuffer(GL_COLOR_ATTACHMENT0);
    glGenTextures(1, bufferId); // this is for the depth buffer
    glBindTexture(GL_TEXTURE_2D, bufferId[0]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, width, height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, bufferId[0], 0);
    // initialize reflection framebuffer
    glGenFramebuffers(1, bufferId);
    reflectFrameBuffer = bufferId[0];
    glBindFramebuffer(GL_FRAMEBUFFER, refractFrameBuffer);
    glGenTextures(1, bufferId); // this is for the color buffer
    reflectTextureId = bufferId[0];
    glBindTexture(GL_TEXTURE_2D, reflectTextureId);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, reflectTextureId, 0);
    glDrawBuffer(GL_COLOR_ATTACHMENT0);
    glGenTextures(1, bufferId); // this is for the depth buffer
    glBindTexture(GL_TEXTURE_2D, bufferId[0]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, width, height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, bufferId[0], 0);
}

void reshape(GLFWwindow* window, int w, int h)
{
    glViewport(0, 0, (GLsizei)w, (GLsizei)h);
}       


// draw
void display(GLuint shaderProgram, float deltaTime) {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glmc_perspective(glm_rad(height * 0.1f), (float)width / (float)height, 0.1f, 100.0f, &perspective);

    move_events(deltaTime);
    updateCameraVectors();
    getViewMatrix(&view);

    glViewport(0, 0, width, height);
    glClearColor(1.0, 1.0, 1.0, 1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDepthMask(GL_FALSE);
    glCullFace(GL_FRONT);
    glUseProgram(cubemapShader);
    glBindTexture(GL_TEXTURE_CUBE_MAP, cubemaptexture);
    glBindVertexArray(vao[1]);
    view[3][0] = 0;
    view[3][1] = 0;
    view[3][2] = 0;
    glm_mat4_mulN((mat4 * []) { &perspective, & view, & model2 }, 3, mvp);
    glUniformMatrix4fv(glGetUniformLocation(cubemapShader, "mvp"), 1, GL_FALSE, mvp[0]);
    glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);

    // FACE THE CAMERA UP
    // take a screenshot of the sky and paste it as a texture onto the water object
    // this will be the reflect portion of the water

    glBindFramebuffer(GL_FRAMEBUFFER, reflectFrameBuffer);

    mat4 view_below = GLM_MAT4_IDENTITY_INIT;
    vec3 translate_under_the_water_facing_up = {0.0f, -(-3.5-cameraPos[1]), 0.0f};
    glmc_translate(view_below, translate_under_the_water_facing_up);
    glmc_rotate(view_below, -PITCH, GLM_XUP);
    glClear(GL_DEPTH_BUFFER_BIT);
    glClear(GL_COLOR_BUFFER_BIT);
    glDepthMask(GL_FALSE);

    glUseProgram(cubemapShader);
    glBindTexture(GL_TEXTURE_CUBE_MAP, cubemaptexture);
    glBindVertexArray(vao[1]);
    glm_mat4_mulN((mat4 * []) { &perspective, & view_below, & model2 }, 3, mvp);
    glUniformMatrix4fv(glGetUniformLocation(cubemapShader, "mvp"), 1, GL_FALSE, mvp[0]);
    glActiveTexture(GL_TEXTURE1);
    glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
    // DREW SKY TO FRAMEBUFFER
    glCullFace(GL_BACK);

    // refract
    glBindFramebuffer(GL_FRAMEBUFFER, reflectFrameBuffer);
    mat4 view_above = GLM_MAT4_IDENTITY_INIT;
    vec3 translate_above_the_water_facing_down = { 0.0f, -(cameraPos[1]), 0.0f };
    glmc_translate(view_above, translate_above_the_water_facing_down);
    glmc_rotate(view_above, -PITCH, GLM_XUP);
    glBindBuffer(GL_FRAMEBUFFER, refractFrameBuffer);
    glClear(GL_DEPTH_BUFFER_BIT);
    glClear(GL_COLOR_BUFFER_BIT);
    // now render the checkerboard floor (and other items below the surface) to the refraction buffer
    glUseProgram(shaderProgram);
    glBindVertexArray(vao[0]);

    glm_mat4_mulN((mat4* []) { &perspective, & view, & model1 }, 3, mvp);
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "mvp"), 1, GL_FALSE, mvp[0]);
    glUniform3fv(glGetUniformLocation(shaderProgram, "eyePos"), 1, cameraPos);
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, model1[0]);
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, view[0]);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glActiveTexture(GL_TEXTURE2);

    glDrawArrays(GL_TRIANGLES, 0, 6);


    glDepthMask(GL_TRUE);
    glEnable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glEnable(GL_DEPTH_TEST);



    
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glClear(GL_DEPTH_BUFFER_BIT);
    glClear(GL_COLOR_BUFFER_BIT);

    move_events(deltaTime);
    updateCameraVectors();
    getViewMatrix(&view);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, reflectTextureId);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, refractTextureId);

    glUseProgram(waterShader);
    glBindVertexArray(vao[2]);
    glm_mat4_mulN((mat4 * []) { &perspective, & view, & model3 }, 3, mvp);
    glUniformMatrix4fv(glGetUniformLocation(waterShader, "mvp"), 1, GL_FALSE, mvp[0]);
    glUniformMatrix4fv(glGetUniformLocation(waterShader, "model"), 1, GL_FALSE, model3[0]);
    glUniformMatrix4fv(glGetUniformLocation(waterShader, "view"), 1, GL_FALSE, view[0]);

    glDrawArrays(GL_TRIANGLES, 0, 6);
    glEnable(GL_CULL_FACE);

    glCullFace(GL_BACK);
    glUseProgram(shaderProgram);
    glBindVertexArray(vao[0]);

    glFrontFace(GL_CW);

    glm_mat4_mulN((mat4 * []) { &perspective, & view, & model1 }, 3, mvp);
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "mvp"), 1, GL_FALSE, mvp[0]);
    glUniform3fv(glGetUniformLocation(shaderProgram, "eyePos"), 1, cameraPos);
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, model1[0]);
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, view[0]);
    glDrawElements(GL_TRIANGLES, CHUNK_DIMENSION* CHUNK_DIMENSION * 2 * 3, GL_UNSIGNED_INT, 0);

}

void window_size_callback(GLFWwindow* window, int w, int h)
{
    width = w;
    height = h;
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_ESCAPE) { if (action == GLFW_PRESS) { lockcamerain = !lockcamerain; } }


    if (!lockcamerain) {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
    }
    else {

        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
    }
    cameraKeyMovement(window, key, scancode, action, mods);
}

void mouse_callback(GLFWwindow* window, double xposz, double yposz) {

    float xpos = (float)(xposz);
    float ypos = (float)(yposz);
    if (lockcamerain) {

        if (firstMouse)
        {
            lastX = xpos;
            lastY = ypos;
            firstMouse = false;
        }
        float xoffset = xpos - lastX;
        float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top

        lastX = xpos;
        lastY = ypos;

        camera_mouse_callback(xoffset, yoffset);

        glfwSetCursorPos(window, width / 2.0f, height / 2.0f);
        lastX = width / 2.0f;
        lastY = height / 2.0f;
    }
};

void window_execution() {
    GLFWwindow* window;

    if (!glfwInit())
        exit(EXIT_FAILURE);

    window = glfwCreateWindow(width, height, "Terrain Demo", NULL, NULL);
    if (!window)
    {
        glfwTerminate();
        exit(EXIT_FAILURE);
    }

    glfwSetWindowAspectRatio(window, 1, 1);

    glfwSetFramebufferSizeCallback(window, reshape);
    glfwSetWindowSizeCallback(window, window_size_callback);

    glfwSetKeyCallback(window, key_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwMakeContextCurrent(window);
    gladLoadGL(glfwGetProcAddress);
    glfwSwapInterval(1);

    glfwGetFramebufferSize(window, &width, &height);
    init_camera(window);

    glfwSetTime(0.0);
    GLuint shaderProgram = init();
    printf("%d", shaderProgram);

    float t, t_old = 0;
    float dt = glfwGetTime();
    /* Main loop */
    createFrameBuffers(window);

    while (true)
    {
        glUseProgram(shaderProgram);

        /* Timing */
        t = glfwGetTime();
        dt = t - t_old;
        t_old = t;

        /* Draw one frame */
        display(shaderProgram, dt);

        /* Swap buffers */
        glfwSwapBuffers(window);
        glfwPollEvents();

        /* Check if we are still running */
        if (glfwWindowShouldClose(window))
            break;

    }

    glfwTerminate();
    exit(EXIT_SUCCESS);
}

int main(void)
{
    window_execution();
    return 0;
}
