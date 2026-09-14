#version 300 es

in vec3 vertexBasePos;

uniform mat4 mvp;
uniform vec3 eyePos;
uniform vec3 center;
uniform vec3 scaleDir;
uniform float scalePerp;
uniform vec3 scaleDirPMP;

out vec3 fragNormal;
out vec3 fragEyeDir;

void main()
{
    fragNormal = vertexBasePos;

    float d = dot(vertexBasePos, scaleDir);
    vec3 worldPos = scalePerp * vertexBasePos + d * scaleDirPMP + center;

    fragEyeDir = normalize(eyePos - worldPos);

    gl_Position = mvp * vec4(worldPos, 1.0);
}