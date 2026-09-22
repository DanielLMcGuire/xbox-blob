#version 330

in vec3 fragEnvRefl;
in vec3 fragBlobRefl;
in vec3 fragMoodRefl;
in vec3 fragToEye;

uniform float shading;
uniform float blobIntensity;
uniform vec3 blobSpecColor;
uniform samplerCube envMap;

out vec4 finalColor;

const vec3 BLOB_HIGHLIGHT_TINT = vec3(1.2, 1.5, 1.2);

float lobe(vec3 reflected, vec3 toEye)
{
    float d = clamp(dot(normalize(reflected), toEye), 0.0, 1.0);
    d *= d;
    d *= d;
    d *= d;
    d *= d;
    return d;
}

void main()
{
    vec3 toEye = normalize(fragToEye);

    float blob = lobe(fragBlobRefl, toEye);
    float mood = lobe(fragMoodRefl, toEye);

    vec3 highlight = min(blob * BLOB_HIGHLIGHT_TINT, 1.0);
    highlight = min(highlight + mood, 1.0);

    vec3 color = min(highlight * blobSpecColor + texture(envMap, fragEnvRefl).rgb, 1.0);

    finalColor = vec4(color * clamp(blobIntensity, 0.0, 1.0), shading);
}
