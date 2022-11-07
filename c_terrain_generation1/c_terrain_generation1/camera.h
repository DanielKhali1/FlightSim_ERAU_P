#pragma once
#include <cglm/cglm.h>   /* for inline */
#include <cglm/call.h>   

float YAW;
float PITCH;
float SPEED;
float SENSITIVITY;
float ZOOM;

vec3 cameraFront;
vec3 cameraUp;
vec3 cameraTarget;
vec3 cameraDirection;

vec3 up;
vec3 cameraRight;

vec3 cameraPos;
bool mv[6];
float firstMouse;
float lastX;
float lastY;



void init_camera(GLFWwindow* window);
void camera_mouse_callback(double xoffset, double yoffset);
void updateCameraVectors();
void getViewMatrix(mat4 * dest);
void move_events(float deltaTime);
void cameraKeyMovement(GLFWwindow* window, int key, int scancode, int action, int mods);
