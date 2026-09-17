#version 300 es
precision mediump float;

uniform vec4 flatColor;

out vec4 finalColor;

void main()
{
    finalColor = flatColor;
}