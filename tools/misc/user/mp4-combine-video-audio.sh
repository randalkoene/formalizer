#!/bin/bash
#
# Randal A. Koene, 20241225
#
# Use this to combine a video file with an audio file into one mp4
# if the two were downloaded separately, e.g. from Youtube.

videofile="$1"
audiofile="$2"

if [ "$videofile" = "" -o "$audiofile" = "" ]; then
	echo "Usage: mp4-combine-video-audio.sh <videofile> <audiofile>"
	echo "       The result will be called 'combined.mp4'."
	exit 1
fi

ffmpeg -i "$videofile" -i "$audiofile" -c:v copy -c:a copy combined.mp4
