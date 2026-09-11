@ECHO OFF
ffmpeg -framerate 240 -i build/frames/frame_%%06d.tga -c:v h264_amf -quality quality -pix_fmt yuv420p blob.mp4