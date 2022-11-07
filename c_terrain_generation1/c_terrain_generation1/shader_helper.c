#pragma once
#include <glad/gl.h>
#include <stdio.h>
#include <stdlib.h>
//#include <string.h>
#include "shader_helper.h"

char* pullCstrFromFile(char* source, char * str) {
	
	FILE* file;

	file = fopen(source, "r");

	if (file == NULL) return NULL;

	fseek(file, 0, SEEK_END);

	int length = ftell(file);
	fseek(file, 0, SEEK_SET);
	char c;
	char* sttr = (char *)malloc(sizeof(char) * (length + 1));
	int i = 0;
	while ((c = fgetc(file)) != EOF)
	{
		sttr[i] = c;
		i++;
	}
	sttr[i] = '\0';
	fclose(file);

	str = sttr;
}

int createShader(const char* vertexShaderSource, const char* fragmentShaderSource)
{
	char* vert;
	char* frag;

	const GLchar* vertProg = pullCstrFromFile((char *)vertexShaderSource, &vert);
	const GLchar* fragProg = pullCstrFromFile((char *)fragmentShaderSource, &frag);

	char infoLog[512];

	printf(" VERTEX SHADER \n");
	GLuint vertexShaderIndex = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertexShaderIndex, 1, &vertProg, NULL);
	free(vertProg);
	glCompileShader(vertexShaderIndex);
	checkStatus(vertexShaderIndex, infoLog, 0);

	printf("FRAGMENT SHADER \n");
	GLuint fragmentShaderIndex = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragmentShaderIndex, 1, &fragProg, NULL);
	free(fragProg);
	glCompileShader(fragmentShaderIndex);
	checkStatus(fragmentShaderIndex, infoLog, 0);

	printf("COMBINED SHADER PROGRAM\n");
	GLuint shaderProgram = glCreateProgram();
	glAttachShader(shaderProgram, vertexShaderIndex);
	glAttachShader(shaderProgram, fragmentShaderIndex);
	glLinkProgram(shaderProgram);
	checkStatus(shaderProgram, infoLog, 1);

	glDeleteProgram(vertexShaderIndex);
	glDeleteProgram(fragmentShaderIndex);
	return shaderProgram;
}


void checkStatus(unsigned int shaderIndex, char* infoLog, int isLinking)
{
	int success = 1;
	glGetProgramiv(shaderIndex, GL_COMPILE_STATUS, &success);

	if (!success) {
		glGetProgramInfoLog(shaderIndex, 512, NULL, infoLog);
		printf("ERROR::PROGRAM::COMPILATION\n%s", infoLog);
	}
	else
	{
		printf("SUCCESS::PROGRAM::COMPILATION\n");
	}
	if (isLinking) {
		glGetProgramiv(shaderIndex, GL_LINK_STATUS, &success);
		if (!success)
		{
			glGetProgramInfoLog(shaderIndex, 512, NULL, infoLog);
			printf("ERROR::PROGRAM::LINKING\n%s", infoLog);
		}
		else
		{
			printf("SUCCESS::PROGRAM::LINKING\n");
		}
	}
	glGetError();

}
