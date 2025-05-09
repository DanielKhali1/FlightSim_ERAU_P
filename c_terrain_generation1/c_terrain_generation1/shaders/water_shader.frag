#version 430 core
out vec4 FragColor;

layout(binding=2) uniform sampler2D refractTex;
layout(binding=1) uniform sampler2D reflectTex;
layout(binding=7) uniform sampler3D noiseTex;

in vec3 fragPos;
uniform vec3 eyePos;
uniform float depthOffset;
uniform mat4 view;
in vec2 ftc;
in vec4 glp;
in vec3 norm;
//out vec4 color;
vec3 estimateWaveNormal(float offset, float mapScale, float hScale)
{ 
	// estimate the normal using the wave height values stored in the noise texture.
	// Do this by looking up three height values at the specified offset distance around this fragment.
	// incoming parameters are scale factors for nearness of neighbors, size of noise map relative to scene, and height

	float h1 = (texture(noiseTex, vec3(((ftc.s))*mapScale, depthOffset, ((ftc.t)+offset)*mapScale))).r * hScale;
	float h2 = (texture(noiseTex, vec3(((ftc.s)-offset)*mapScale, depthOffset, ((ftc.t)-offset)*mapScale))).r * hScale;
	float h3 = (texture(noiseTex, vec3(((ftc.s)+offset)*mapScale, depthOffset, ((ftc.t)-offset)*mapScale))).r * hScale;
	// build two vectors using the three neighboring height values. The cross product is the estimated normal.
	vec3 v1 = vec3(0, h1, -1); // neighboring height value #1
	vec3 v2 = vec3(-1, h2, 1); // neighboring height value #2
	vec3 v3 = vec3(1, h3, 1); // neighboring height value #3
	vec3 v4 = v2-v1; // first vector orthogonal to desired normal
	vec3 v5 = v3-v1; // second vector orthogonal to desired normal
	vec3 normEst = normalize(cross(v4,v5));
	return normEst;
}
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
	vec3 estNorm = estimateWaveNormal(0.00008, 45.0, 8);
	vec3 lightDir = normalize(  lightPos  - fragPos  ); 
	float diff = max (dot(estNorm, lightDir), 0.0);
	vec3 diffuse = diff * lightColor *.5;

	float specularStrength = 1;
    vec3 viewDir = normalize(eyePos - fragPos);
    vec3 reflectDir = reflect(-lightDir, estNorm);  
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
    vec3 specular = specularStrength * spec * lightColor;

	vec3 Nfres = normalize(estNorm);
	float cosFres = dot(viewDir,Nfres);
	float fresnel = acos(cosFres);
	fresnel = pow(clamp(fresnel-0.3, 0.0, 1.0), 3);

	vec4 refractColor = texture(refractTex, (vec2(glp.x,glp.y))/(2.0*glp.w)+0.5);
	vec4 reflectColor = texture(reflectTex, (vec2(glp.x,-glp.y))/(2.0*glp.w)+0.5);
	vec4 mixColor;

	if(eyePos.y > -3.5) {
		mixColor = mix( refractColor ,reflectColor, fresnel);
	} else {
		mixColor = mix(refractColor , reflectColor, 0.2);

	}
	FragColor = mix( fogColor, vec4((mixColor.xyz * .8*(ambient + diffuse) + 0.5 * specular), 1.0), fogFactor);
}