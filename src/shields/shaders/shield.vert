#version 330

layout(location = 0) in vec3 vertexPosition;
layout(location = 1) in vec3 vertexNormal;

uniform mat4 mvp;
uniform mat4 model;
uniform vec3 eyePos;
uniform vec3 blobLightPos;
uniform vec3 moodLightPos;

out vec3 fragEnvRefl;
out vec3 fragBlobRefl;
out vec3 fragMoodRefl;
out vec3 fragToEye;

vec3 reflectRay(vec3 incident, vec3 n)
{
    return incident - 2.0 * dot(incident, n) * n;
}

void main()
{
    vec4 worldPos = model * vec4(vertexPosition, 1.0);
    vec3 n = normalize(mat3(model) * vertexNormal);

    fragEnvRefl = reflectRay(worldPos.xyz - eyePos, n);
    fragBlobRefl = reflectRay(worldPos.xyz - blobLightPos, n);
    fragMoodRefl = reflectRay(worldPos.xyz - moodLightPos, n);
    fragToEye = eyePos - worldPos.xyz;

    gl_Position = mvp * worldPos;
}
