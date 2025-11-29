#!/bin/bash
# Ardesia Screencast Script - Fullscreen, PID corretto, crop rimosso

ACTION="$1"
LOGFILE="$2"
FILENAME="$3"
PIDFILE="$4"

RECORDER_PROGRAM="cvlc"

ICECAST="FALSE"
ICECAST_PASSWORD="hackme"
ICECAST_ADDRESS="127.0.0.1"
ICECAST_PORT=8000
ICECAST_MOUNTPOINT="ardesia.ogg"

FPS=12
VIDEO_ENCODER="theora"
VIDEO_CODEC="theo"
VIDEO_BITRATE=512
VIDEO_SCALE=1.0
THEORA_QUALITY=5
AUDIO_CODEC="vorb"
AUDIO_BITRATE=128
AUDIO_CHANNELS=2
AUDIO_RATE=44100
VORBIS_QUALITY=1

detect_audio_device() {
    if command -v pacmd &>/dev/null; then
        AUDIO_INPUT=":input-slave=pulse://"
    elif command -v pw-record &>/dev/null; then
        AUDIO_INPUT=":input-slave=pipewire-audio://"
    else
        AUDIO_INPUT=":input-slave=alsa://"
    fi
}

start_recording() {
    detect_audio_device

    COMMONOPTIONS="-vvv screen:// --screen-fps=$FPS $AUDIO_INPUT"

    TRANSCODE="#transcode{venc=$VIDEO_ENCODER,vcodec=$VIDEO_CODEC,vb=$VIDEO_BITRATE,scale=$VIDEO_SCALE,acodec=$AUDIO_CODEC,ab=$AUDIO_BITRATE,channels=$AUDIO_CHANNELS,samplerate=$AUDIO_RATE,audio-sync}"

    if [ "$ICECAST" = "TRUE" ]; then
        SOUT="$TRANSCODE:duplicate{dst=std{access=shout,mux=ogg,dst=source:$ICECAST_PASSWORD@$ICECAST_ADDRESS:$ICECAST_PORT/$ICECAST_MOUNTPOINT},dst=std{access=file,mux=ogg,dst=$FILENAME}}"
    else
        SOUT="$TRANSCODE:standard{access=file,mux=ogg,dst=$FILENAME}"
    fi

    RECORDER_PROGRAM_OPTIONS="$COMMONOPTIONS --sout-theora-quality=$THEORA_QUALITY --sout-vorbis-quality=$VORBIS_QUALITY --sout $SOUT"

    echo "Execute $RECORDER_PROGRAM $RECORDER_PROGRAM_OPTIONS" >> "$LOGFILE"

    $RECORDER_PROGRAM $RECORDER_PROGRAM_OPTIONS >>"$LOGFILE" 2>&1 &
    
    RECORDER_PID=$!
    echo $RECORDER_PID > "$PIDFILE"
    echo "Recorder started - PID: $RECORDER_PID" >> "$LOGFILE"
}

pause_recording() {
    [ ! -f "$PIDFILE" ] && { echo "ERROR: PID file not found" >> "$LOGFILE"; exit 1; }
    RECORDER_PID=$(cat "$PIDFILE")
    kill -TSTP "$RECORDER_PID"
    echo "Paused (PID: $RECORDER_PID)" >> "$LOGFILE"
}

resume_recording() {
    [ ! -f "$PIDFILE" ] && { echo "ERROR: PID file not found" >> "$LOGFILE"; exit 1; }
    RECORDER_PID=$(cat "$PIDFILE")
    kill -CONT "$RECORDER_PID"
    echo "Resumed (PID: $RECORDER_PID)" >> "$LOGFILE"
}

stop_recording() {
    [ ! -f "$PIDFILE" ] && { echo "WARNING: PID file not found" >> "$LOGFILE"; exit 0; }
    RECORDER_PID=$(cat "$PIDFILE")
    kill -2 "$RECORDER_PID"
    sleep 1
    rm -f "$PIDFILE"
    echo "Stopped (PID: $RECORDER_PID)" >> "$LOGFILE"
}


if [ -z "$ACTION" ] || [ -z "$LOGFILE" ] || [ -z "$FILENAME" ] || [ -z "$PIDFILE" ]; then
    echo "Usage: $0 <start|pause|resume|stop> <logfile> <filename> <pidfile>"
    exit 1
fi
case "$ACTION" in
    start) start_recording ;;
    pause) pause_recording ;;
    resume) resume_recording ;;
    stop)  stop_recording ;;
    *) echo "ERROR: Unknown command: $ACTION" >> "$LOG"; exit 1 ;;
esac

