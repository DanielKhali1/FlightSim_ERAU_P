#version 430 core

layout (location = 1) in vec3 aPos;
layout (location = 2) in vec2 tc;
layout (location = 3) in vec3 aNorm;

uniform mat4 mvp;
uniform mat4 model;
uniform mat4 view;

out vec2 ftc;
out vec3 fragPos;

out vec4 glp;

//out vec4 color;

void main()
{
	glp = mvp * vec4(aPos, 1.0);
   gl_Position = mvp * vec4(aPos, 1.0) ;
   ftc = tc;
} 