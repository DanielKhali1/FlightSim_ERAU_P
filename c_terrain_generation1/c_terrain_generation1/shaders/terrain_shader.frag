#version 430 core
out vec4 FragColor;

layout(binding=0) uniform sampler2D samp1;

in vec3 norm;
in vec3 fragPos;

uniform vec3 eyePos;

in vec3 vertEyeSpacePos;
in vec2 ftc;
in vec4 color;
void main()
{

	vec4 fogColor = vec4(0.5, 0.6, 0.7, 1.0); // bluish gray
	float fogStart = 5;
	float fogEnd = 20;
	// the distance from the camera to the vertex in eye space is simply the length of a
	// vector to that vertex, because the camera is at (0,0,0) in eye space.
	float dist = length(fragPos-eyePos);
	float fogFactor = clamp(((fogEnd - dist) / (fogEnd - fogStart)), 0.0, 1.0);


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
	vec4 mainColor = vec4(0.5, 0.5, 0.5, 1.0); 


	FragColor = mix(fogColor, vec4((ambient + diffuse), 1.0) * texture(samp1, ftc), fogFactor);
}