#!/bin/bash

## action to perform (start/stop)
ACTION="$1"
## log file
LOG="$2"
## destination for video file
DST="$7"
## window size
TOP="$3"
LEFT="$4"
WIDTH="$5"
HEIGHT="$6"
PIDFILENAME="$8"

RECORDER_PROGRAM="cvlc"


# Uncomment this to use the live screencast on icecast
# To start the live stream successfully you must configure
# the configuration file ezstream_stdin_vorbis.xml
# as you desire to point to the right running icecast server
# The configuration file is setted to work properly
# if it is used a localhost icecast server with the default
# configuration
# Client side you must have the ezstream installed
ICECAST="FALSE"
#ICECAST="TRUE"

# You must configure the right password; I put the default one
ICECAST_PASSWORD=hackme
ICECAST_ADDRESS=127.0.0.1
ICECAST_PORT=8000
ICECAST_MOUNTPOINT=ardesia.ogg

SCRIPT_FOLDER=`dirname "$0"`
RECORDER_PID_FILE=$PIDFILENAME



if [ "${ACTION}" = "start" ]
then
    echo "Ardesia passed the following options:" > $LOG
    echo "ACTION=${ACTION}" >> $LOG
    echo "DST=${DST}" >> $LOG
    echo "TOP=${TOP}" >> $LOG
    echo "LEFT=${LEFT}" >> $LOG
    echo "WIDTH=${WIDTH}" >> $LOG
    echo "HEIGHT=${HEIGHT}" >> $LOG

    if [ "x$TOP" == "x" ]
    then
        echo "No TOP coordinate found"
        exit 1000
    fi
    if [ "x$LEFT" == "x" ]
    then
        echo "No LEFT dimension found"
        exit 1001
    fi
    if [ "x$WIDTH" == "x" ]
    then
        echo "No WIDTH dimension found"
        exit 1002
    fi
    if [ "x$HEIGHT" == "x" ]
    then
        echo "No HEIGHT dimension found"
        exit 1003
    fi

    # Audio detection.
    AUDIO_INPUT=""
    if command -v pacmd &> /dev/null; then
        # If pacmd is available the use as audio input PulseAudio. This works on PipeWire also.
        # This use yout integrated mic if it exists.
        AUDIO_INPUT=":input-slave=pulse://alsa_input.pci-0000_00_1f.3.analog-stereo" # Sostituisci questo con il nome della tua sorgente
    elif command -v pw-record &> /dev/null; then
        # Id pw-record is availabòe, means that you are using PipeWire
	# In this cas might work the PulseAudio audio input and the logic above. This is a fallback.
        AUDIO_INPUT=":input-slave=pipewire-audio://"
    else
        # If you have not this commands use ALSA as a last resort.
        AUDIO_INPUT=":input-slave=alsa://"
    fi
    
    # Vlc common options.
    COMMONOPTIONS="-vvv screen:// --screen-top=$TOP --screen-left=$LEFT --screen-width=$WIDTH --screen-height=$HEIGHT --screen-fps=12 $AUDIO_INPUT"
    # *** FINE MODIFICA ***

    #This start the recording on file
    echo Start the screencast running $RECORDER_PROGRAM
    if [ "$ICECAST" = "TRUE" ]
    then
        RECORDER_PROGRAM_OPTIONS="${COMMONOPTIONS} --ignore-config --sout  "#transcode{venc=theora,vcodec=theo,vb=512,scale=0.7,acodec=vorb,ab=128,channels=2,samplerate=44100,audio-sync}:duplicate{dst=std{access=shout,mux=ogg,dst=source:$ICECAST_PASSWORD@$ICECAST_ADDRESS:$ICECAST_PORT/$ICECAST_MOUNTPOINT},dst=std{access=file,mux=ogg,dst=$DST}}""
    else
        RECORDER_PROGRAM_OPTIONS="${COMMONOPTIONS} --sout-theora-quality=5 --sout-vorbis-quality=1 --sout "#transcode{venc=theora,vcodec=theo,vb=512,scale=1.0,acodec=vorb,ab=128,channels=2,samplerate=44100,audio-sync}:standard{access=file,mux=ogg,dst=$DST}""
    fi
    echo "With arguments $RECORDER_PROGRAM_OPTIONS"
    $RECORDER_PROGRAM $RECORDER_PROGRAM_OPTIONS >>$LOG 2>&1 &
    RECORDER_PID=$!
    echo $RECORDER_PID >> $RECORDER_PID_FILE
fi

if [ "${ACTION}" = "pause" ]
then
  RECORDER_PID=$(cat $RECORDER_PID_FILE)
  echo "Pause the screencast sending TSTP to $RECORDER_PROGRAM" >> $LOG
  kill -TSTP $RECORDER_PID
fi

if [ "${ACTION}" = "resume" ]
then
  RECORDER_PID=$(cat $RECORDER_PID_FILE)
  echo "Resume the screencast sending CONT to $RECORDER_PROGRAM" >> $LOG
  kill -CONT $RECORDER_PID
fi

if [ "${ACTION}" = "stop" ]
then
  RECORDER_PID=$(cat $RECORDER_PID_FILE)
  echo "Stop the screencast killing $RECORDER_PROGRAM" >> $LOG
  kill -2 $RECORDER_PID
  rm $RECORDER_PID_FILE
fi
