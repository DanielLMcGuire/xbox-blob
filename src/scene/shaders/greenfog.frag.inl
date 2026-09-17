R"GLSL(#version 330

in vec2 fragTexCoord0;
in vec2 fragPlasmaUV[3];

uniform sampler2D intensityMap;
uniform sampler2D plasmaMap0;
uniform sampler2D plasmaMap1;
uniform sampler2D plasmaMap2;
uniform bool useIntensityMap;
uniform vec3 intensityTint;
uniform vec3 glowColor;

out vec4 finalColor;

float samplePlasma(vec4 texColor)
{
    return (texColor.a < 1.0) ? texColor.a : texColor.r;
}

void main()
{
    float p0 = samplePlasma(texture(plasmaMap0, fragPlasmaUV[0]));
    float p1 = samplePlasma(texture(plasmaMap1, fragPlasmaUV[1]));
    float p2 = samplePlasma(texture(plasmaMap2, fragPlasmaUV[2]));
    
    vec3 plasmaSum = vec3(p0 + p1 + p2) * intensityTint;
    vec3 occ = useIntensityMap ? texture(intensityMap, fragTexCoord0).rgb : vec3(1.0);
    vec3 color = (plasmaSum * occ) + glowColor;

    finalColor = vec4(color, 1.0);
}
)GLSL"