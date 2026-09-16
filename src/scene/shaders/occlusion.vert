#version 330

layout(location = 0) in vec3 vertexPosition;

uniform mat4 mvp;
uniform mat4 model;
uniform vec3 cameraPos;

out float viewDist;

void main()
{
    vec3 worldPos = (model * vec4(vertexPosition, 1.0)).xyz;
    viewDist = length(worldPos - cameraPos);
    gl_Position = mvp * vec4(vertexPosition, 1.0);
}
