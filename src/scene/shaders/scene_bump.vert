#version 330

layout(location = 0) in vec3 vertexPosition;
layout(location = 1) in vec3 vertexTangent;
layout(location = 2) in vec3 vertexBitangent;
layout(location = 3) in vec3 vertexNormal;
layout(location = 4) in vec2 vertexTexCoord;

uniform mat4 mvp;
uniform mat4 model;
uniform mat4 normalMatrix;
uniform mat4 lightSpaceMatrix;

out vec3 fragWorldPos;
out vec3 fragTangent;
out vec3 fragBitangent;
out vec3 fragNormal;
out vec2 fragTexCoord;
out vec4 fragShadowCoord;

void main()
{
    vec4 worldPos = model * vec4(vertexPosition, 1.0);
    fragWorldPos = worldPos.xyz;
    fragTangent = mat3(normalMatrix) * vertexTangent;
    fragBitangent = mat3(normalMatrix) * vertexBitangent;
    fragNormal = mat3(normalMatrix) * vertexNormal;
    fragTexCoord = vertexTexCoord;
    fragShadowCoord = lightSpaceMatrix * worldPos;
    gl_Position = mvp * vec4(vertexPosition, 1.0);
}
