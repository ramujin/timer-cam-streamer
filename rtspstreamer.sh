#!/bin/bash

# VIDSOURCE="rtsp://127.0.0.1:8554"
# AUDIO_OPTS="-c:a aac -b:a 160000 -ac 2"
# VIDEO_OPTS="-s 854x480 -c:v libx264 -b:v 800000"
# OUTPUT_HLS="-hls_time 10 -hls_list_size 10 -start_number 1"
# ffmpeg -i "$VIDSOURCE" -y $AUDIO_OPTS $VIDEO_OPTS $OUTPUT_HLS playlist.m3u8

ffmpeg -f avfoundation -pix_fmt yuyv422 -video_size 640x480 -framerate 30 -i "0:0" -ac 2 -vf format=yuyv422 -vcodec libx264 -maxrate 2000k -bufsize 2000k -acodec aac -ar 44100 -b:a 128k -f rtp_mpegts rtp://127.0.0.1:8554