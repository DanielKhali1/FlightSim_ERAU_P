
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


#define CHUNK_DIMENSION 1000
#define GL_TEXTURE_MAX_ANISOTROPY_EXT 0x84FE
#define GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT 0x84FF

#include "shader_helper.h"
mat4 perspective = GLM_MAT4_IDENTITY_INIT;
mat4 view = GLM_MAT4_IDENTITY_INIT;
mat4 model1 = GLM_MAT4_IDENTITY_INIT;
mat4 model2 = GLM_MAT4_IDENTITY_INIT;
mat4 mvp = GLM_MAT4_IDENTITY_INIT;
int width = 800.0f, height = 600.0f;
GLuint cubemapShader;
GLuint cubemaptexture;
GLuint vao[2];

float* g_vertices;

bool lockcamerain = true    ;

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

float * generateVerticies(int dim, float width, float height)
{
    float * vertices = (float*)malloc(sizeof(float) * dim * dim * 3);

    int i = 0; int j = 0;
    float * fSeed = (float *) malloc(sizeof(float) * dim * dim);

    for (i = 0; i < dim * dim; i++)
        fSeed[i] = -2 + ((float)rand() / (float)RAND_MAX) * 4;

    float** perlinNoise = PerlinNoise2D(8, 1.5, dim, dim, fSeed);

    int index = 0;
    for ( i = 0; i < dim; i++)
    {
        for (j = 0; j < dim; j++)
        {
            vertices[index++] = j * width*0.1;
            vertices[index++] = perlinNoise[i][j] * 10;
            vertices[index++] = i * height * 0.1;
        }
    }
    free(fSeed);
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
        "right.jpg",
        "left.jpg",
        "top.jpg",
        "bottom.jpg",
        "front.jpg",
        "back.jpg",
        SOIL_LOAD_RGB,
        SOIL_CREATE_NEW_ID,
        SOIL_FLAG_MIPMAPS
    );
    return m_texture;
}


GLfloat calculateNormal(int x, int z) {

}

// setup VBO/VAO shit
GLuint init () {
    GLuint shaderProgram = createShader("terrain_shader.vert", "terrain_shader.frag");
    cubemapShader = createShader("skybox_shader.vert", "skybox_shader.frag");
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
    GLfloat* textureCoordinates = generateTC();

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

    GLuint vbo[3];

    glGenVertexArrays(2, vao);
    glGenBuffers(3, vbo);

    // cubemap vertex setup

    GLuint EBO[2];
    glGenBuffers(2, EBO);

    glBindVertexArray(vao[1]);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO[1]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(in), in, GL_STATIC_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, vbo[2]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vc), vc, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, vbo[2]);
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
    glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * CHUNK_DIMENSION * CHUNK_DIMENSION * 3, vertices, GL_STATIC_DRAW);
    free(vertices);
    glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);
    GLuint vertexPosition = glGetAttribLocation(shaderProgram, "aPos");
    glVertexAttribPointer(vertexPosition, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
    glEnableVertexAttribArray(vertexPosition);

    glActiveTexture(GL_TEXTURE0);
    loadTerrainTexture("image.jpg");
    // load texture coordiantes
    glBindBuffer(GL_ARRAY_BUFFER, vbo[1]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * CHUNK_DIMENSION * CHUNK_DIMENSION * 2, textureCoordinates, GL_STATIC_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, vbo[1]);
    GLuint textureCoordinatePosition = glGetAttribLocation(shaderProgram, "tc");
    glVertexAttribPointer(textureCoordinatePosition, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);
    glEnableVertexAttribArray(textureCoordinatePosition);
    free(textureCoordinates);



    glmc_perspective(glm_rad(height*0.1f), (float)width / (float) height, 0.1f, 100.0f, &perspective);
    vec3 m1_translate = { -20.0, 10.0, -20.0 };
    glmc_translate(model1, m1_translate);

    vec3 m2_scale = { 50, 50, 50 };
    glmc_scale(model2, m2_scale);

    return shaderProgram;
}


void reshape(GLFWwindow* window, int w, int h)
{
    glViewport(0, 0, (GLsizei)w, (GLsizei)h);
}       




// draw
void display(GLuint shaderProgram, float deltaTime) {
    glmc_perspective(glm_rad(height * 0.1f), (float)width / (float)height, 0.1f, 100.0f, &perspective);

    move_events(deltaTime);
    updateCameraVectors();
    getViewMatrix(&view);

    glViewport(0, 0, width, height);
    glClearColor(1.0, 1.0, 1.0, 1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    glCullFace(GL_BACK);
    glUseProgram(shaderProgram);
    glBindVertexArray(vao[0]);
    glm_mat4_mulN((mat4 * []) { &perspective, & view, & model1 }, 3, mvp);
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "mvp"), 1, GL_FALSE, mvp[0]);
    glDrawElements(GL_TRIANGLES, CHUNK_DIMENSION* CHUNK_DIMENSION * 2 * 3, GL_UNSIGNED_INT, 0);

    glDepthMask(GL_FALSE);
    glCullFace(GL_FRONT); 
    glUseProgram(cubemapShader);
    glBindTexture(GL_TEXTURE_CUBE_MAP, cubemaptexture);
    glBindVertexArray(vao[1]);
    //vec3 moveToCamera = { cameraPos[0]+20,cameraPos[1]+10, cameraPos[2] };
    
    //glm_translate(model2, moveToCamera);
    view[3][0] = 0;
    view[3][1] = 0;
    view[3][2] = 0;
    glm_mat4_mulN((mat4* []) { &perspective, & view, & model2 }, 3, mvp);
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "mvp"), 1, GL_FALSE, mvp[0]);
    glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
    glDepthMask(GL_TRUE);



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
