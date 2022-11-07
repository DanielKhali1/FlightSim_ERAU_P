#version 430 core

layout (location = 1) in vec3 aPos;
layout (location = 2) in vec2 tc;


uniform mat4 mvp;

out vec2 ftc;
out vec4 color;

void main()
{
   gl_Position = mvp * vec4(aPos, 1.0) ;
   float height_color = 1/(1 + pow(2.7182, -aPos.y*.2))*1.5;
   color = vec4(0.1,height_color, height_color, 1.0);
   ftc = tc;
} 