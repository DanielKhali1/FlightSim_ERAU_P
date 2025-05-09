#version 430 core
out vec4 FragColor;

layout(binding=4) uniform sampler2D samp1;
layout(binding=5) uniform sampler2D samp2;
layout(binding=7) uniform sampler3D noiseTex;

in vec3 norm;
in vec3 fragPos;
uniform float depthOffset;

uniform vec3 eyePos;
uniform int underwater;

in vec3 vertEyeSpacePos;
in vec2 ftc;
in vec4 color;

float getCausticValue(float x, float y, float z)
{ 
	float w = 2; // frequency of caustic curved bands
	float strength = 1.0;
	float PI = 3.14159;
	float noise = texture(noiseTex, vec3(x*w, y, z*w)).r;
	return pow((1.0-abs(sin(noise*2*PI))), strength);
}

void main()
{



//	vec4 fogColor = vec4(189.0/255.0, 87.0/255.0, 64.0/255.0, 1.0); // bluish gray
	vec4 fogColor = vec4(0.3, 0.4, 0.5, 1.0);
		vec4 fogColor2 = vec4(0.1, 0.1, 0.2, 1.0);


	float fogStart = 3;
	float fogEnd = 80;

	float fogStart2 = 3;
	float fogEnd2 = 10;

	float cStart = 1;
	float cEnd = 3;


	// the distance from the camera to the vertex in eye space is simply the length of a
	// vector to that vertex, because the camera is at (0,0,0) in eye space.
	float dist = length(fragPos-eyePos);
	float fogFactor = clamp(((fogEnd - dist) / (fogEnd - fogStart)), 0.0, 1.0);
		float fogFactor2 = clamp(((fogEnd2 - dist) / (fogEnd2 - fogStart2)), 0.0, 1.0);

    float causticFactor2 = clamp(((cEnd - dist) / (cEnd - cStart)), 0.0, 1.0);


	vec3 lightPos = vec3(10, 50, 10);
	vec3 lightColor = vec3(0.9, 0.9, 1);

	float ambientStrength = 0.2;
	vec3 ambient = ambientStrength * lightColor;

	vec3 normal = normalize(norm);
	vec3 lightDir = normalize(  lightPos  - fragPos  ); 
	float diff = max (dot(normal , lightDir), 0.0);
	vec3 diffuse = diff * lightColor;

	float specularStrength = 0.5;
    vec3 viewDir = normalize(eyePos - fragPos);
    vec3 reflectDir = reflect(-lightDir, normal);  
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 256);
    vec3 specular = specularStrength * spec * lightColor;
	vec4 mainColor = vec4(0.5, 0.5, 0.5, 1.0); 
	vec4 color;
	if(eyePos.y < -3.5) {
		float causticColor = getCausticValue(ftc.s, depthOffset, ftc.t);
		float colorR = clamp(color.x + causticColor, 0.0, 1.0);
		float colorG = clamp(color.y + causticColor, 0.0, 1.0);
		float colorB = clamp(color.z + causticColor, 0.0, 1.0);
		color = vec4(colorR, colorG, colorB, 1.0);
	}

	float blend = 1/(1 + pow(2.7182, -(fragPos.y+2))); 
	vec4 t = mix( texture(samp1, ftc), texture(samp2, ftc), blend);
	if(eyePos.y < -3.5) {
		FragColor = mix(fogColor2, mix(vec4((ambient + diffuse), 1.0) * t,vec4(0.5, 0.5, 1.0, 1.0), 0.2), fogFactor2) +  mix(vec4(0,0,0,0),color,causticFactor2)*.3;
	}
	else {
			FragColor = mix(fogColor, vec4((ambient + diffuse+ specular), 1.0) * t , fogFactor);

	}
}