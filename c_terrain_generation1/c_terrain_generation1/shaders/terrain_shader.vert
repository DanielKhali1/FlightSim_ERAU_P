#version 430 core

layout (location = 1) in vec3 aPos;
layout (location = 3) in vec3 aNorm;
layout (location = 2) in vec2 tc;

uniform mat4 mvp;
uniform mat4 model;
uniform mat4 view;

out vec2 ftc;
out vec3 norm;
out vec3 fragPos;
out vec3 vertEyeSpacePos;
//out vec4 color;

void main()
{
	vertEyeSpacePos = (view* vec4(aPos,1.0)).xyz;
	fragPos = vec3(model * vec4(aPos, 1.0));
	norm = mat3(transpose(inverse(model))) * aNorm;

   gl_Position = mvp * vec4(aPos, 1.0) ;
   //float height_color = 1/(1 + pow(2.7182, -aPos.y*.2))*1.5;
   //color = vec4(0.1,height_color, height_color, 1.0);
   ftc = tc;
} 