R"GLSL(#version 330

    in vec3 vertexBasePos;
    in vec4 vertexDynamic;

    uniform mat4 mvp;
    uniform vec3 eyePos;
    uniform vec3 scaling;
    uniform vec3 ooScaling;
    uniform vec3 center;

    out vec3 fragNormal;
    out vec3 fragEyeDir;

    void main()
    {
        vec3 ellipseNormal = normalize(vertexBasePos * ooScaling);
        vec3 basePos = vertexBasePos * scaling + center;
        vec3 worldPos = basePos + ellipseNormal * vertexDynamic.w;

        fragNormal = normalize(vertexDynamic.xyz * ooScaling);
        fragEyeDir = normalize(eyePos - worldPos);
        gl_Position = mvp * vec4(worldPos, 1.0);
    }
)GLSL"