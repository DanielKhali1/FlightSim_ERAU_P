#version 430 core
out vec4 FragColor;

layout(binding=1) uniform sampler2D refractTex;
layout(binding=2) uniform sampler2D reflectTex;

in vec3 fragPos;
uniform vec3 eyePos;
in vec2 ftc;
in vec4 glp;
in vec3 norm;


void main()
{

	vec3 lightPos = vec3(2, 10, 0);
	vec3 lightColor = vec3(0.9, 0.9, 1);

	float ambientStrength = 0.5;
	vec3 ambient = ambientStrength * lightColor;

	vec3 normal = normalize(norm);
	vec3 lightDir = normalize(  lightPos  - fragPos  ); 
	float diff = max (dot(normal , lightDir), 0.0);
	vec3 diffuse = diff * lightColor;

	float specularStrength = 0.5;
    vec3 viewDir = normalize(eyePos - fragPos);
    vec3 reflectDir = reflect(-lightDir, norm);  
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 256);
    vec3 specular = specularStrength * spec * lightColor;

	vec4 refractColor = texture(refractTex, (vec2(glp.x,glp.y))/(2.0*glp.w)+0.5);
	vec4 reflectColor = texture(reflectTex, (vec2(glp.x,-glp.y))/(2.0*glp.w)+0.5);
	vec4 mixColor = (0.2 * refractColor) + (1.0 * reflectColor);
	FragColor = vec4((mixColor.xyz * (ambient + diffuse) + 0.75 * specular), 1.0);
}