#version 430 core
out vec4 FragColor;

layout(binding=8) uniform sampler2D grassTexture;
layout(binding=9) uniform sampler2D barkTexture;

in vec3 fragPos;
uniform vec3 eyePos;
uniform float depthOffset;
uniform mat4 view;
in vec2 ftc;
in vec4 glp;
in vec3 norm;

void main()
{
	//vec4 fogColor = vec4(189.0/255.0, 87.0/255.0, 64.0/255.0, 1.0); // bluish gray
	vec4 fogColor = vec4(0.7, 0.8, 0.9, 1.0); // bluish gray
	
	float fogStart = 5;
	float fogEnd = 100;
	// the distance from the camera to the vertex in eye space is simply the length of a
	// vector to that vertex, because the camera is at (0,0,0) in eye space.
	float dist = length(fragPos-eyePos);
	float fogFactor = clamp(((fogEnd - dist) / (fogEnd - fogStart)), 0.0, 1.0);

	vec3 lightPos = vec3(10, 50, 10);
	vec3 lightColor = vec3(0.9, 0.9, 1);

	float ambientStrength = 0.4; // 0.4
	vec3 ambient = ambientStrength * lightColor;
	vec3 lightDir = normalize(  lightPos  - fragPos  ); 
	float diff = max (dot(norm, lightDir), 0.0);
	vec3 diffuse = diff * lightColor *.5;

	float specularStrength = 1;
    vec3 viewDir = normalize(eyePos - fragPos);
    vec3 reflectDir = reflect(-lightDir, norm);  
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
    vec3 specular = specularStrength * spec * lightColor;

	vec4 mixColor;

	FragColor = mix( fogColor, vec4((mixColor.xyz * .8*(ambient + diffuse) + 0.5 * specular), 1.0), fogFactor);
}