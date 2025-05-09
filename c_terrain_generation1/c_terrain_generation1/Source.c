
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
#define NOISE_WIDTH 256
#define NOISE_HEIGHT 256
#define NOISE_DEPTH 256

#define GRASS_DENSITY 800*3 // 800 * 3

#include "shader_helper.h"
GLuint noiseTexture;
double noise[NOISE_WIDTH][NOISE_HEIGHT][NOISE_DEPTH];
mat4 perspective = GLM_MAT4_IDENTITY_INIT;
mat4 view = GLM_MAT4_IDENTITY_INIT;
mat4 model1 = GLM_MAT4_IDENTITY_INIT;
mat4 model2 = GLM_MAT4_IDENTITY_INIT;
mat4 model3 = GLM_MAT4_IDENTITY_INIT;
mat4 model4 = GLM_MAT4_IDENTITY_INIT;


mat4 mvp = GLM_MAT4_IDENTITY_INIT;
mat4 mvp1 = GLM_MAT4_IDENTITY_INIT;
mat4 mvp2 = GLM_MAT4_IDENTITY_INIT;

double grass_positions[GRASS_DENSITY];

int width = 800.0f, height = 600.0f;
GLuint cubemapShader;
GLuint waterShader;

GLuint cubemaptexture;
GLuint vao[5];
GLuint refractTextureId;
GLuint reflectTextureId;

GLuint instancedObjectsShader;

GLuint refractFrameBuffer;
GLuint reflectFrameBuffer;

GLuint dirtTexture;
GLuint grassTexture;
GLuint physicalGrassTexture;
float depthLookup = 0.0f;

GLuint groundtexture;

float* g_vertices;

bool lockcamerain = true;
float prevTime = 0;

GLuint loadTexture1(const char* texImagePath) {
    GLuint textureID = SOIL_load_OGL_texture(texImagePath,
        SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_INVERT_Y);
    if (textureID == 0) printf("could not find texture file %s", texImagePath);
    return textureID;
}
void loadTexture(const char* name) {
    GLuint temp = loadTexture1(name);
    
    glBindTexture(GL_TEXTURE_2D, temp);
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

GLuint loadTerrainTexture(const char* name) {
    GLuint temp = loadTexture1(name);
    glBindTexture(GL_TEXTURE_2D, temp);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glGenerateMipmap(GL_TEXTURE_2D);

    if (glewIsSupported("GL_EXT_texture_filter_anisotropic")) {
        GLfloat anisoSetting = 0.0f;
        glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &anisoSetting);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, anisoSetting);
    }
    return temp;
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
    return ((island_info[i * 1024 + j] / 155)) * 8;
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

float* generateVerticiesNoHeight(int dim, float width, float height)
{
    float* vertices = (float*)malloc(sizeof(float) * dim * dim * 3);

    int i = 0; int j = 0;

    int index = 0;
    for (i = 0; i < dim; i++)
    {
        for (j = 0; j < dim; j++)
        {
            vertices[index++] = j * width * 1 * 3;
            vertices[index++] = 0;
            vertices[index++] = i * height * 1 * 3;
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

    /*
        "res\\right.jpg",
        "res\\left.jpg",
        "res\\top.jpg",
        "res\\bottom.jpg",
        "res\\front.jpg",
        "res\\back.jpg",
    */
    // cubemap
    GLuint m_texture = SOIL_load_OGL_cubemap
    (
        "res\\bluecloud_ft.jpg",
        "res\\bluecloud_bk.jpg",
        "res\\bluecloud_up.jpg",
        "res\\bluecloud_dn.jpg",
        "res\\bluecloud_rt.jpg",
        "res\\bluecloud_lf.jpg",
        SOIL_LOAD_RGB,
        SOIL_CREATE_NEW_ID,
        SOIL_FLAG_MIPMAPS
    );
    return m_texture;
}

void generateNoise() {
    srand(time(0));
    for (int x = 0; x < NOISE_WIDTH; x++) {
        for (int y = 0; y < NOISE_HEIGHT; y++) {
            for (int z = 0; z < NOISE_DEPTH; z++) {
                noise[x][y][z] = (double)rand() / (RAND_MAX + 1.0); // computes a double in [0..1]
            }
        }
    }
}

double smoothNoise(double zoom, double x1, double y1, double z1) {
    // fraction of x1, y1, and z1 (percentage from current block to next block, for this texel)
    double fractX = x1 - (int)x1;
    double fractY = y1 - (int)y1;
    double fractZ = z1 - (int)z1;
    // indices for neighboring values with wrapping at the ends
    double x2 = x1 - 1; if (x2 < 0) x2 = round(NOISE_WIDTH / zoom) - 1.0;
    double y2 = y1 - 1; if (y2 < 0) y2 = round(NOISE_HEIGHT / zoom) - 1.0;
    double z2 = z1 - 1; if (z2 < 0) z2 = round(NOISE_DEPTH / zoom) - 1.0;
    // smooth the noise by interpolating the greyscale intensity along all three axes
    double value = 0.0;
    value += fractX * fractY * fractZ * noise[(int)x1][(int)y1][(int)z1];
    value += (1 - fractX) * fractY * fractZ * noise[(int)x2][(int)y1][(int)z1];
    value += fractX * (1 - fractY) * fractZ * noise[(int)x1][(int)y2][(int)z1];
    value += (1 - fractX) * (1 - fractY) * fractZ * noise[(int)x2][(int)y2][(int)z1];
    value += fractX * fractY * (1 - fractZ) * noise[(int)x1][(int)y1][(int)z2];
    value += (1 - fractX) * fractY * (1 - fractZ) * noise[(int)x2][(int)y1][(int)z2];
    value += fractX * (1 - fractY) * (1 - fractZ) * noise[(int)x1][(int)y2][(int)z2];
    value += (1 - fractX) * (1 - fractY) * (1 - fractZ) * noise[(int)x2][(int)y2][(int)z2];
    return value;
}

double turbulence(double x, double y, double z, double maxZoom) {
    double sum = 0.0, zoom = maxZoom;
    sum = sin((1.0 / 512.0) * (8 * 3.14159) * (x + z-4*y)) + 1 * 8.0;
    while (zoom >= 0.9) {
        sum = sum + smoothNoise(zoom, x / zoom, y / zoom, z / zoom) * zoom;
        zoom = zoom / 2.0;
    }
    sum = 128.0 * sum / maxZoom;
    return sum;
}

void fillDataArray(GLubyte * data) {
    double maxZoom = 32.0;
    for (int i = 0; i < NOISE_WIDTH; i++) {
        for (int j = 0; j < NOISE_HEIGHT; j++) {
            for (int k = 0; k < NOISE_DEPTH; k++) {
                data[i * (NOISE_WIDTH * NOISE_HEIGHT * 4) + j * (NOISE_HEIGHT * 4) + k * 4 + 0] =
                    (GLubyte)turbulence(i, j, k, maxZoom);
                data[i * (NOISE_WIDTH * NOISE_HEIGHT * 4) + j * (NOISE_HEIGHT * 4) + k * 4 + 1] =
                    (GLubyte)turbulence(i, j, k, maxZoom);
                data[i * (NOISE_WIDTH * NOISE_HEIGHT * 4) + j * (NOISE_HEIGHT * 4) + k * 4 + 2] =
                    (GLubyte)turbulence(i, j, k, maxZoom);
                data[i * (NOISE_WIDTH * NOISE_HEIGHT * 4) + j * (NOISE_HEIGHT * 4) + k * 4 + 3] =
                    (GLubyte)255;
            }
        }
    }
}
int load3DTexture() {
    GLuint textureID;
    GLubyte* data = (GLubyte*)malloc(256 * 256 * 256 * 4);
    fillDataArray(data);
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_3D, textureID);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA8, 256, 256, 256, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
    glTexSubImage3D(GL_TEXTURE_3D, 0, 0, 0, 0, 256, 256, 256,
        GL_RGBA, GL_UNSIGNED_BYTE, data);
    free(data);
    return textureID;
}
// setup VBO/VAO shit
GLuint init() {
    GLuint shaderProgram = createShader("shaders\\terrain_shader.vert", "shaders\\terrain_shader.frag");
    cubemapShader = createShader("shaders\\skybox_shader.vert", "shaders\\skybox_shader.frag");
    waterShader = createShader("shaders\\water_shader.vert", "shaders\\water_shader.frag");
    instancedObjectsShader = createShader("shaders\\instancedTerrainObject.vert", "shaders\\instancedTerrainObject.frag");

    cubemaptexture = loadCubemap();
    generateNoise();
    noiseTexture = load3DTexture();


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
    GLfloat* normals = generateNormals(CHUNK_DIMENSION, 1, 1);
    GLfloat* textureCoordinates = generateTC();

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

    GLuint vbo[10];

    glGenVertexArrays(5, vao);
    glGenBuffers(10, vbo);

    // cubemap vertex setup

    GLuint EBO[3];
    glGenBuffers(3, EBO);

    // terrain vertex setup

    glBindVertexArray(vao[0]);
    // set up indices
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO[0]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLuint) * CHUNK_DIMENSION * CHUNK_DIMENSION * 2 * 3, indices, GL_STATIC_DRAW);
    free(indices);

    // load vertices
    glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * CHUNK_DIMENSION * CHUNK_DIMENSION * 3, vertices, GL_STATIC_DRAW);
    free(vertices);
    glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);
    GLuint vertexPosition = glGetAttribLocation(shaderProgram, "aPos");
    glVertexAttribPointer(vertexPosition, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
    glEnableVertexAttribArray(vertexPosition);

    // load normals
    glBindBuffer(GL_ARRAY_BUFFER, vbo[1]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * CHUNK_DIMENSION * CHUNK_DIMENSION * 3, normals, GL_STATIC_DRAW);
    free(normals);
    glBindBuffer(GL_ARRAY_BUFFER, vbo[1]);
    GLuint normalsPosition = glGetAttribLocation(shaderProgram, "aNorm");
    glVertexAttribPointer(normalsPosition, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
    glEnableVertexAttribArray(normalsPosition);

    glActiveTexture(GL_TEXTURE4);
    dirtTexture = loadTerrainTexture("res\\dirt.png");
    glActiveTexture(GL_TEXTURE5);
    grassTexture = loadTerrainTexture("res\\image.jpg");

    glActiveTexture(GL_TEXTURE8);
    physicalGrassTexture = loadTerrainTexture("res\\grass.png");
    // load texture coordiantes
    glBindBuffer(GL_ARRAY_BUFFER, vbo[2]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * CHUNK_DIMENSION * CHUNK_DIMENSION * 2, textureCoordinates, GL_STATIC_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, vbo[2]);
    GLuint textureCoordinatePosition = glGetAttribLocation(shaderProgram, "tc");
    glVertexAttribPointer(textureCoordinatePosition, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);
    glEnableVertexAttribArray(textureCoordinatePosition);
    free(textureCoordinates);
    glBindVertexArray(vao[1]);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO[1]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(in), in, GL_STATIC_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, vbo[3]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vc), vc, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, vbo[3]);
    GLuint VP = glGetAttribLocation(cubemapShader, "aPos");
    glVertexAttribPointer(VP, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
    glEnableVertexAttribArray(VP);

    glBindVertexArray(vao[2]);

    GLfloat* PLANE_POSITIONS = generateVerticiesNoHeight(CHUNK_DIMENSION, 1, 1);
    GLuint* PLANE_INDICES = generateFaces(CHUNK_DIMENSION);
    GLfloat* PLANE_NORMALS = generateNormals(CHUNK_DIMENSION, 1, 1);
    GLfloat* PLANE_TEXCOORDS = generateTC();

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO[2]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLuint) * CHUNK_DIMENSION * CHUNK_DIMENSION * 2 * 3, PLANE_INDICES, GL_STATIC_DRAW);
    free(PLANE_INDICES);

    glBindBuffer(GL_ARRAY_BUFFER, vbo[4]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * CHUNK_DIMENSION * CHUNK_DIMENSION * 3, PLANE_POSITIONS, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, vbo[4]);
    GLuint vpos = glGetAttribLocation(waterShader, "aPos");
    glVertexAttribPointer(vpos, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
    glEnableVertexAttribArray(vpos);
    free(PLANE_POSITIONS);

    glBindBuffer(GL_ARRAY_BUFFER, vbo[5]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * CHUNK_DIMENSION * CHUNK_DIMENSION * 2, PLANE_TEXCOORDS, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, vbo[5]);
    GLuint tcpos = glGetAttribLocation(waterShader, "tc");
    glVertexAttribPointer(tcpos, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);
    glEnableVertexAttribArray(tcpos);
    free(PLANE_TEXCOORDS);

    glBindBuffer(GL_ARRAY_BUFFER, vbo[6]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * CHUNK_DIMENSION * CHUNK_DIMENSION * 3, PLANE_NORMALS, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, vbo[6]);
    GLuint normPos = glGetAttribLocation(waterShader, "aNorm");
    glVertexAttribPointer(normPos, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
    glEnableVertexAttribArray(normPos);
    free(PLANE_NORMALS);

    glBindVertexArray(vao[3]);

    float GRASS_POSITIONS[18] = {
        -128.0f, 0.0f, -128.0f, -128.0f, 0.0f, 128.0f, 128.0f, 0.0f, -128.0f,
        128.0f, 0.0f, -128.0f, -128.0f, 0.0f, 128.0f, 128.0f, 0.0f, 128.0f
    };

    float GRASS_NORMALS[18] = {
    0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f,0.0f, 1.0f, 0.0f,
    0.0f, 1.0f, 0.0f,0.0f, 1.0f, 0.0f,0.0f, 1.0f, 0.0f
    };

    float GRASS_TEXCOORDS[12] = {
    0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f
    };

    glBindBuffer(GL_ARRAY_BUFFER, vbo[7]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GRASS_NORMALS), GRASS_NORMALS, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, vbo[7]);
    GLuint grassNormPos = glGetAttribLocation(instancedObjectsShader, "aNorm");
    glVertexAttribPointer(grassNormPos, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
    glEnableVertexAttribArray(grassNormPos);

    glBindBuffer(GL_ARRAY_BUFFER, vbo[8]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GRASS_POSITIONS), GRASS_POSITIONS, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, vbo[8]);
    GLuint grassvpos = glGetAttribLocation(instancedObjectsShader, "aPos");
    glVertexAttribPointer(grassvpos, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
    glEnableVertexAttribArray(grassvpos);

    glBindBuffer(GL_ARRAY_BUFFER, vbo[9]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GRASS_TEXCOORDS), GRASS_TEXCOORDS, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, vbo[9]);
    GLuint grasstcpos = glGetAttribLocation(instancedObjectsShader, "tc");
    glVertexAttribPointer(grasstcpos, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);
    glEnableVertexAttribArray(grasstcpos);

    glmc_perspective(glm_rad(height * 0.1f), (float)width / (float)height, 0.1f, 500.0f, &perspective);
    vec3 m1_translate = { -50, -5, 0.0 };
    vec3 m1_scale = { 2, 2, 2 };
    glmc_translate(model1, m1_translate);


    vec3 m2_scale = { 5, 5, 5 };
    //vec3 m2_translate = { 0, 0, 5};
    glmc_scale(model2, m2_scale);

    //vec3 m3_scale = { 5, 5, 5 };
    vec3 m3_trans = { -500, -3.5, -500 };
    glmc_translate(model3, m3_trans);

    vec3 m4_trans = { 0, 5, 0 };
    glmc_scale(model4, m2_scale);
    glmc_translate(model4, m4_trans);

    //glmc_rotate(model3, 90, GLM_YUP);

    return shaderProgram;
}

void createRefractFrameBuffers(GLFWwindow * window) {
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
}

void createReflectFrameBuffers(GLFWwindow* window) {
    glfwGetFramebufferSize(window, &width, &height);
    reflectFrameBuffer = 5;
    glGenFramebuffers(1, &reflectFrameBuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, reflectFrameBuffer);

    glGenTextures(1, &reflectTextureId); // this is for the color buffer
    glBindTexture(GL_TEXTURE_2D, reflectTextureId);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, reflectTextureId, 0);
    glDrawBuffer(GL_COLOR_ATTACHMENT0);

    GLuint depthBufferTextureId;

    glGenTextures(1, &depthBufferTextureId); // this is for the depth buffer
    glBindTexture(GL_TEXTURE_2D, depthBufferTextureId);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, width, height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthBufferTextureId, 0);
}

void reshape(GLFWwindow* window, int w, int h)
{
    width = w;
    height = h;
    glViewport(0, 0, (GLsizei)w, (GLsizei)h);
    glDeleteFramebuffers(1, &reflectFrameBuffer);
    glDeleteFramebuffers(1, &refractFrameBuffer);

    createReflectFrameBuffers(window);
    createRefractFrameBuffers(window);

}       

// draw
void display(GLuint shaderProgram, float deltaTime) {
    glmc_perspective(glm_rad(height * 0.1f), (float)width / (float)height, 0.1f, 50100.0f, &perspective);

    move_events(deltaTime);
    updateCameraVectors();
    getViewMatrix(&view);

    glViewport(0, 0, width, height);
    glClearColor(1.0, 1.0, 1.0, 1.0);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    // Draw The Sky first
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
    glUniform3fv(glGetUniformLocation(cubemapShader, "eyePos"), 1, cameraPos);
    glUniformMatrix4fv(glGetUniformLocation(cubemapShader, "model"), 1, GL_FALSE, model2[0]);
    glUniformMatrix4fv(glGetUniformLocation(cubemapShader, "view"), 1, GL_FALSE, view[0]);
    glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
    


    // FACE THE CAMERA UP
    // take a screenshot of the sky and paste it as a texture onto the water object
    // this will be the reflect portion of the water
  
    glBindFramebuffer(GL_FRAMEBUFFER, reflectFrameBuffer);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glViewport(0, 0, width, height);

    PITCH = -PITCH;
    move_events(deltaTime);
    updateCameraVectors();
    getViewMatrix(&view);

    glDisable(GL_DEPTH_TEST);
    vec3 translate_under_the_water_facing_up = { cameraPos[0], (-3.5-cameraPos[1]), cameraPos[2] };
    glmc_translate(view, translate_under_the_water_facing_up);
    //glmc_rotate(view, -PITCH, GLM_XUP);
    glUseProgram(cubemapShader);
    glBindTexture(GL_TEXTURE_CUBE_MAP, cubemaptexture);
    glBindVertexArray(vao[1]);
    view[3][0] = 0;
    view[3][1] = 0;
    view[3][2] = 0;
    glm_mat4_mulN((mat4 * []) { &perspective, & view, & model2 }, 3, mvp2);
    glUniformMatrix4fv(glGetUniformLocation(cubemapShader, "mvp"), 1, GL_FALSE, mvp2[0]);
    glUniform3fv(glGetUniformLocation(cubemapShader, "eyePos"), 1, cameraPos);
    glUniformMatrix4fv(glGetUniformLocation(cubemapShader, "model"), 1, GL_FALSE, model2[0]);
    glUniformMatrix4fv(glGetUniformLocation(cubemapShader, "view"), 1, GL_FALSE, view[0]);
    glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
    // DREW SKY TO FRAMEBUFFER Texture

    // RENDER THE REAL TERRAIN NOW
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glCullFace(GL_BACK);
    glDepthMask(GL_TRUE);
    glEnable(GL_DEPTH_TEST);

    PITCH = -PITCH;
    move_events(deltaTime);
    updateCameraVectors();
    getViewMatrix(&view);

    // DRAWING THE TERRAIN
    glUseProgram(shaderProgram);
    glBindVertexArray(vao[0]);

    glActiveTexture(GL_TEXTURE4);
    glBindTexture(GL_TEXTURE_2D, dirtTexture);
    glActiveTexture(GL_TEXTURE5);
    glBindTexture(GL_TEXTURE_2D, grassTexture);

    glm_mat4_mulN((mat4* []) { &perspective, & view, & model1 }, 3, mvp);
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "mvp"), 1, GL_FALSE, mvp[0]);
    glUniform3fv(glGetUniformLocation(shaderProgram, "eyePos"), 1, cameraPos);
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, model1[0]);
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, view[0]);
    glDrawElements(GL_TRIANGLES, CHUNK_DIMENSION* CHUNK_DIMENSION * 2 * 3, GL_UNSIGNED_INT, 0);
    glUniform1i(glGetUniformLocation(waterShader, "underwater"), 0);

    //// refract
    move_events(deltaTime);
    updateCameraVectors();
    getViewMatrix(&view);
    glViewport(0, 0, width, height);

    glBindFramebuffer(GL_FRAMEBUFFER, refractFrameBuffer);
    glClear(GL_DEPTH_BUFFER_BIT);
    glClear(GL_COLOR_BUFFER_BIT);
    //// now render the floor (and other items below the surface) to the refraction buffer
    glUseProgram(shaderProgram);
    glBindVertexArray(vao[0]);


    vec3 vecsss = { cameraPos[0], (cameraPos[1]), cameraPos[2] };
    glActiveTexture(GL_TEXTURE4);
    glBindTexture(GL_TEXTURE_2D, dirtTexture);
    glActiveTexture(GL_TEXTURE5);
    glBindTexture(GL_TEXTURE_2D, grassTexture);

    glm_mat4_mulN((mat4* []) { &perspective, & view, & model1 }, 3, mvp);
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "mvp"), 1, GL_FALSE, mvp[0]);
    glUniform3fv(glGetUniformLocation(shaderProgram, "eyePos"), 1, cameraPos);
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, model1[0]);
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, view[0]);
    glDrawElements(GL_TRIANGLES, CHUNK_DIMENSION* CHUNK_DIMENSION * 2 * 3, GL_UNSIGNED_INT, 0);
    glUniform1f(glGetUniformLocation(waterShader, "depthOffset"), depthLookup);
    glUniform1i(glGetUniformLocation(waterShader, "underwater"), 1);

    move_events(deltaTime);
    updateCameraVectors();
    getViewMatrix(&view);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glUseProgram(waterShader);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, reflectTextureId);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, refractTextureId);
    glActiveTexture(GL_TEXTURE7);
    glBindTexture(GL_TEXTURE_3D, noiseTexture);
    glDisable(GL_CULL_FACE);
    glBindVertexArray(vao[2]);

    float currentTime = glfwGetTime();
    depthLookup += (currentTime - prevTime) * 0.04f;
    prevTime = currentTime;

    glUniform1f(glGetUniformLocation(waterShader, "depthOffset"), depthLookup);
    glm_mat4_mulN((mat4 * []) { &perspective, & view, & model3 }, 3, mvp);
    glUniformMatrix4fv(glGetUniformLocation(waterShader, "mvp"), 1, GL_FALSE, mvp[0]);
    glUniformMatrix4fv(glGetUniformLocation(waterShader, "model"), 1, GL_FALSE, model3[0]);
    glUniformMatrix4fv(glGetUniformLocation(waterShader, "view"), 1, GL_FALSE, view[0]);
    glUniform3fv(glGetUniformLocation(waterShader, "eyePos"), 1, cameraPos);

    glDrawElements(GL_TRIANGLES, CHUNK_DIMENSION* CHUNK_DIMENSION * 2 * 3, GL_UNSIGNED_INT, 0);

    glUseProgram(instancedObjectsShader);
    glBindVertexArray(vao[3]);

    glActiveTexture(GL_TEXTURE8);
    glBindTexture(GL_TEXTURE_2D, physicalGrassTexture);

    glm_mat4_mulN((mat4* []) { &perspective, & view, & model4 }, 3, mvp);
    glUniformMatrix4fv(glGetUniformLocation(instancedObjectsShader, "model"), 1, GL_FALSE, model4[0]);
    glUniformMatrix4fv(glGetUniformLocation(instancedObjectsShader, "view"), 1, GL_FALSE, view[0]);
    glUniform3fv(glGetUniformLocation(instancedObjectsShader, "eyePos"), 1, cameraPos);
    glDrawArrays(GL_TRIANGLES, 0, 18);

    glEnable(GL_CULL_FACE);



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

    float t, t_old = 0;
    float dt = glfwGetTime();
    /* Main loop */
    createReflectFrameBuffers(window);
    createRefractFrameBuffers(window);

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
