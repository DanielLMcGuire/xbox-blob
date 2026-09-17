R"GLSL(#version 300 es
precision mediump float;

in float v_depthVal;

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

    float val = v_depthVal * vign;
    
    finalColor = vec4(val, val, val, 1.0);
}
)GLSL"