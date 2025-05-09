#include <linmath.h>
#include <SOIL2.h>
#include <cglm/cglm.h>   /* for inline */
#include <cglm/call.h>   /* for library call (this also includes cglm.h) */
#include <GLFW/glfw3.h>
#include "camera.h"

void init_camera(GLFWwindow* window) {


	YAW = 90.0f;
	PITCH = 0.0f;
	SPEED = 2.5f;
	SENSITIVITY = 0.1f;
	ZOOM = 45.0f;
	firstMouse = true;

	//setting up initial vectors
	cameraFront[1] = -1.0f;
	cameraUp[1] = 1.0f;
	cameraTarget;
	vec3 temp;
	glm_vec3_sub(&cameraPos, &cameraTarget, &temp);
	glm_vec3_normalize(&temp, &cameraDirection);

	up[1] = 1.0f;
	glm_vec3_crossn(&up, &cameraDirection, &cameraRight);

	cameraPos[2] = 5.0f;


	int HEIGHT;
	int WIDTH;
	glfwGetWindowSize(window, &WIDTH, &HEIGHT);
	firstMouse = true;
	lastX = WIDTH / 2.0f, lastY = HEIGHT / 2.0f;

	for (int i = 0; i < sizeof(mv); i++) {
		mv[i] = false;
	}
}

void camera_mouse_callback(double xoffset, double yoffset) {

	xoffset *= SENSITIVITY;
	yoffset *= SENSITIVITY;

	YAW += xoffset;
	PITCH += yoffset;

	// make sure that when pitch is out of bounds, screen doesn't get flipped
	if (PITCH > 89.0f)
		PITCH = 89.0f;
	if (PITCH < -89.0f)
		PITCH = -89.0f;

	updateCameraVectors();
}

void updateCameraVectors()
{
	// calculate the new Front vector
	vec3 front = GLM_VEC3_ONE_INIT;
	front[0] = cos(glm_rad(YAW)) * cos(glm_rad(PITCH));
	front[1] = sin(glm_rad(PITCH));
	front[2] = sin(glm_rad(YAW)) * cos(glm_rad(PITCH));
	glmc_vec3_normalize_to(&front, &cameraFront);
	
	// also re-calculate the Right and Up vector
	glm_vec3_crossn(&cameraFront, &up, &cameraRight);
	glm_vec3_crossn(&cameraRight, &cameraFront, &cameraUp);
}

void getViewMatrix(mat4 * dest) {
	vec3 temp;
	glm_vec3_add(&cameraPos, &cameraFront, &temp);
	glm_lookat(&cameraPos, &temp, &cameraUp, dest);
}

void getViewMatrixLookingAtOrigin(mat4 * dest) {

	vec3 temp;
	vec3 zerovec = GLM_VEC3_ZERO_INIT;
	glm_vec3_sub(&zerovec, &cameraPos, temp);
	glm_lookat(cameraPos, temp, cameraUp, dest);
}

void cameraKeyMovement(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (key == GLFW_KEY_A) { if (action == GLFW_PRESS) { mv[0] = true; } else if (action == GLFW_RELEASE) { mv[0] = false; } }
	if (key == GLFW_KEY_D) { if (action == GLFW_PRESS) { mv[1] = true; } else if (action == GLFW_RELEASE) { mv[1] = false; } }
	if (key == GLFW_KEY_LEFT_CONTROL) { if (action == GLFW_PRESS) { mv[2] = true; } else if (action == GLFW_RELEASE) { mv[2] = false; } }
	if (key == GLFW_KEY_SPACE) { if (action == GLFW_PRESS) { mv[3] = true; } else if (action == GLFW_RELEASE) { mv[3] = false; } }
	if (key == GLFW_KEY_W) { if (action == GLFW_PRESS) { mv[4] = true; } else if (action == GLFW_RELEASE) { mv[4] = false; } }
	if (key == GLFW_KEY_S) { if (action == GLFW_PRESS) { mv[5] = true; } else if (action == GLFW_RELEASE) { mv[5] = false; } }
	if (key == GLFW_KEY_RIGHT_SHIFT || key == GLFW_KEY_LEFT_SHIFT) {
		if (action == GLFW_PRESS) {
			SPEED *= 4;
		}
		else if (action == GLFW_RELEASE) {
			SPEED /= 4;
		}
	}
}

void move_events(float deltaTime) {

	float velocity = SPEED * deltaTime;

	if (mv[0]) { int i = 0; for (int i = 0; i < 3; i++) cameraPos[i] -= cameraRight[i] * velocity;}
	// right
	if (mv[1]) { int i = 0; for (int i = 0; i < 3; i++) cameraPos[i] += cameraRight[i] * velocity; }
	// down
	if (mv[2]) cameraPos[1] -=  velocity;
	// up
	if (mv[3]) { cameraPos[1] += velocity; }
	// forward
	if (mv[5]) { int i = 0; for (int i = 0; i < 3; i++) cameraPos[i] -= cameraFront[i] * velocity; }
	//backward
	if (mv[4]) { int i = 0; for (int i = 0; i < 3; i++) cameraPos[i] += cameraFront[i] * velocity; }
}