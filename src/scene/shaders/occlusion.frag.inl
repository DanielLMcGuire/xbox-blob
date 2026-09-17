R"GLSL(#version 330

in float viewDist;

uniform vec2 targetSize;
uniform vec2 blobNDC;
uniform vec2 posMul;

out vec4 finalColor;

void main()
{
    vec2 ndc = (gl_FragCoord.xy / targetSize) * 2.0 - 1.0;

    vec2 delta = 2.0 * (ndc - blobNDC) * posMul;
    float r2 = dot(delta, delta);

    float vign = clamp(1.0 - r2, 0.0, 1.0);
    vign = vign * vign;

    float val = viewDist * vign;
    
    finalColor = vec4(val, val, val, 1.0);
}
)GLSL"