#!/usr/bin/env bash
ffmpeg -framerate 240 -i build/frames/frame_%06d.tga -c:v libx264 -crf 16 -preset slow -pix_fmt yuv420p blob.mp4