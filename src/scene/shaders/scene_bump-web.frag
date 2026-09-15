#version 300 es
precision mediump float;

in vec3 fragWorldPos;
in vec3 fragTangent;
in vec3 fragBitangent;
in vec3 fragNormal;
in vec2 fragTexCoord;
in vec4 fragShadowCoord;

uniform vec3 eyePos;
uniform vec3 lightPos;
uniform vec3 diffuseColor;
uniform vec3 specularColor;
uniform vec3 ambientColor;
uniform vec3 attenuation;
uniform sampler2D normalMap;
uniform sampler2D shadowMap;
uniform bool useShadow;

out vec4 finalColor;

void main()
{
    vec3 T = normalize(fragTangent);
    vec3 B = normalize(fragBitangent);
    vec3 Ngeom = normalize(fragNormal);
    vec3 mapN = texture(normalMap, fragTexCoord).rgb * 2.0 - 1.0;
    vec3 N = normalize(mat3(T, B, Ngeom) * mapN);

    vec3 toLight = lightPos - fragWorldPos;
    float dist = length(toLight);
    vec3 L = toLight / max(dist, 0.0001);
    vec3 V = normalize(eyePos - fragWorldPos);
    vec3 H = normalize(L + V);

    float ndotl = max(dot(N, L), 0.0);
    float ndoth = max(dot(N, H), 0.0);
    float spec = pow(ndoth, 16.0);

    float atten = 1.0 / max(attenuation.x + attenuation.y * dist + attenuation.z * dist * dist, 0.0001);

    vec3 color = (diffuseColor * ndotl + specularColor * spec) * atten + ambientColor;

    float shadow = 1.0;
    if (useShadow)
    {
        vec3 proj = fragShadowCoord.xyz / fragShadowCoord.w;
        proj = proj * 0.5 + 0.5;
        if (proj.x >= 0.0 && proj.x <= 1.0 && proj.y >= 0.0 && proj.y <= 1.0 && proj.z <= 1.0)
        {
            float shadowDepth = texture(shadowMap, proj.xy).r;
            if (proj.z - 0.002 > shadowDepth)
                shadow = 0.25;
        }
    }
    color *= shadow;

    finalColor = vec4(color, 1.0);
}