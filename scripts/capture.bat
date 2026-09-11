@ECHO OFF
ffmpeg -framerate 60 -i build/frames/frame_%%06d.png -c:v libx264 -crf 16 -preset slow -pix_fmt yuv420p blob.mp4