#version 430 core
out vec4 FragColor;

layout(binding=0) uniform sampler2D samp1;

in vec2 ftc;
in vec4 color;
void main()
{
		FragColor = mix(texture(samp1, ftc), color, 0.6);
}