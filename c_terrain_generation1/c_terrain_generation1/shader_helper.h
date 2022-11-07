#pragma once

int createShader(const char* vertexShaderSource, const char* fragmentShaderSource);
void checkStatus(unsigned int shaderIndex, char* infoLog, int isLinking);
