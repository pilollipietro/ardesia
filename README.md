<pre>
===============================================================================================
                      ##     #####    #####    ######   ####    #     ##
                     #  #    #    #   #    #   #       #        #    #  #
                    #    #   #    #   #    #   #####    ####    #   #    #
                    ######   #####    #    #   #            #   #   ######
                    #    #   #   #    #    #   #       #    #   #   #    #
                    #    #   #    #   #####    ######   ####    #   #    #
===============================================================================================
</pre>

# Introduction


Ardesia is the free digital sketchpad software that help you to make coloured free-hand
annotations with digital ink everywhere, record it and share on the network.
It is easy to use and impressively fast and reactive.
You can draw upon the desktop or import an image and annotate it and redistribute your work
to the world.
Let's create quick sketch and artwork.

Thanks to Ardesia you are free to open any application and fix your ideas and comments as if
you wrote on a classic chalkboard. Tradition and innovation are living together in simplicity
thanks to a natural user interface that reproduces the natural feeling of the free hand
painting.
Reduce the traditional whiteboard and paper usage; Ardesia is smooth and fuzzy!

You can use the tool to make effective on-screen presentation, highlight things or point out
things of interest.
The tool facilitates the online presentations and demos showing in real time your computer
screen to anyone in the network.

You can use this tool to enhance your lessons or courses working with your preferred
applications and your preferred operating system.

Create nice tutorial and demos saving the desktop images with your free hand annotations.

Ardesia includes a sketchpad software that allow to free-hand draw geometrical shapes using
the shape recognizer.

Ardesia respects the interoperability standards, it imports and exports artefact using common
file open formats.
Ardesia works with all the pointing device; you can use a mouse, a touch screen, a drawing
tablet, a cheap&professional wiimote whiteboard or a commercial whiteboard, interactive
projector and any other devices supported by your operating system.

Ardesia has been designed with the Glade/GtkBuilder tool
and it uses to draw the cairo graphics library.

For more info about annotate and the new feature, please read the NEWS file located in the
current folder.


# License


This Program is distributed under the Gnu General Public License version 3.
See the file COPYING for details.


# Installation

Please read the INSTALL file located on the current folder.

# Runtime Requirements

## Composite manager
  In order to run Ardesia on Linux you need to run a composite manager such as
  compiz, kwin or xcompmgr. You can also use a compositing window manager as xfwm,
  metacity and mutter.

  If you are a Windows "user" I remember you that only these flavours are supported:
  * Windows 7 Ultimate, Enterprise, Professional and Home Premium
  * Windows Vista Business, Enterprise, Ultimate and Home Premium Editions


## VLC
  To use the recording feature you must install the VLC multimedia player
  and Streamer.

  On Debian/Ubuntu like distributions VLC could be already installed.
  Anyway, you could do this manually with this command:

  $sudo apt-get install vlc

  If you use an other Linux distro please use your package manager

  If you are using Windows you must download the vlc installer
  from http://www.videolan.org/vlc and install it.
  After the installation add to the PATH environment
  variable the vlc installation folder.


## xdg-utils (Linux only)
  On Ubuntu xdg-utils is installed by default (ubuntu-desktop needs it)
  If you use a Debian like distribution, you can install the xdg-utils with this command:

  ```$sudo apt-get install xdg-utils```


# Usage

## From Gui

Start Ardesia clicking in the menu on Applications/Tools/Ardesia
Now you can select colours and annotate the screen.

Video tutorial on http://www.youtube.com/watch?v=iTiV1qz-rEI

Button explanation

* Thickness: it selects the thin thickness for the selected tool;
  pressing the button you can select a thin, medium and thick thickness
* Drawing mode: it selects the drawing mode to be used.
  There are three different modes:
  * Handwriting: allow you to draw withou assistance
  * Rectifier: it rectifies your sketch;
    it transforms the closed path in poligons,
    it is useful to paint straight lines,
    it transforms the open path in broken lines.
  * Rounder: it rounds your sketch; 
    it transforms the closed path in ellipses or smoothed closed path,
    it transforms the open path in smoothed lines with bezier spline
* Filler: it fills the contiguos area with the selected colour
* Arrow: it puts an arrow at the end of the line sketched
* Text: insert a text annotation;
  select the text tool and then click on the desktop,
  now you can insert text with the real or virtual keyboard
* Highlighter: it allows to highlight on the desktop
* Eraser: it selects the eraser tool
* Pen: it selects the pen tool
* Blue: it selects the blue colour
* Green: it selects the green color
* Yellow: it selects the yellow colour
* Red: it selects the red colour
* White: it selects the white colour
* Colour selector: it allow to selects an other colour through a palette
* Unlock: it unlocks the grab to annotate on the desktop;
  after the unlock you can use as usual your desktop,
  if do you want restart to sketch push the lock icon
* Clear: it erases all the desktop annotation
* Undo: it reverses the annotation to the older state
* Redo: it advances the annotation to the more current state
* Preferences: it allow to set some preference;
  * you can select the background colour
  * you can select the background image;
    * a set of useful backgrounds will be shown
    * you can select an other image file also
* Screenshoot: it exports in a picture file your desktop screenshot
* Export as pdf: it export as pdf; the first time you must
  select a file name, then each time that you will click 
  on the icon you will append to the pdf a new page with your
  screen content
* Record: it records your desktop;
  it uses the vlc tool,
  if you want allow anyone to see your work
  you can forward the stream to an icecast server
  (please see the instruction inside the file located
   on $(prefix)/share/ardesia/scripts/screencast.sh or
   if you use Windows on bin\screencast.bat).
  After a correct configuration when you push the record the streaming will start
* Info: It shows the info about the tool
* Quit: It allows to quit the program

You can reuse your iwb project going in the ardesia workspace and "open with Ardesia"
the iwb file

Ardesia bar customization. You can change the ardesia bar look and feel copying the desktop/gtrc file in /ect/xdg/ardesia folder
and customizing it. The file might be auto explanatory.

## From Command Line

The default behaviour of ardesia is appear in the east zone of the screen
with a vertical layout.
If you want modify this launch ardesia from command line in this way:

```# ardesia --gravity north```


Here it follows a complete description of the options for the Ardesia:
<pre>
Usage: ardesia [options] [filename]

Ardesia the free digital sketchpad

options:
  --verbose ,	-V		Enable verbose mode to see the logs
  --decorate,	-d		Decorate the window with the borders
  --gravity ,	-g		Set the gravity of the bar. Possible values are:
  				east [default]
  				west
  				north
  				south
  --font ,      -f              Set the font family for the text window. Possible values are:
                                serif [default]
                                sans-serif
                                monospace
  --leftmargin, -l              Set the left margin in text window to set after hitting Enter
  --tabsize,    -t              Set the tabsize in pixel in text window
  --help    ,	-h		Shows the help screen
  --version ,	-v		Show version information and exit

filename:	  		The interactive Whiteboard Common File (iwb)
</pre>



# Troubleshooting


- Ardesia does not start and says that you must enable a composite manager
  - setup your computer to use the accelerated graphic drivers
    - intel graphics card might work with the free driver
    - nvidia graphics driver you must install the proprietary driver
    - ati you must install the proprietary driver
  - if you are using Gnome go in the panel under System/Preference/Appearance and put
    the visuals effect to normal
  - if you are using kde enable the kwin composite manager

- Ardesia bar is not full visible, it is greater than my desktop
  - try to put the bar in horizontal position with the gravity option
    e.g. ardesia -g south

- I can not move the Ardesia Bar. Why?
  - I have put the bar in the right of the screen because is the best
    position to be accessible with the hand and it does not hide
    important thinks on the desktop
    Anyway you can choose the position starting the Ardesia bar
    with the decorate option
    e.g. ardesia -d
    Now you can move the ardesia bar everywhere

- Ardesia is not able to record my desktop
  - check that you audio driver works fine, recording some speak
    with an other program
  - install the vlc package (it's required)
    http://www.videolan.org/vlc/
  - if you use Windows add the vlc folder to the PATH environment variable
  - if you have other problems try to customize the
    $PREFIX/share/ardesia/scripts/screencast.sh
    or share\ardesia\scripts\screencast.bat if you use Windows

- I have read that with Ardesia I can put my presentation on the web.
  How I can do this?
  - Ardesia supports the live screencast streaming
    only after that you have configured the file
    properly
    In order to make a presentation you must have:
    - A running icecast2 server
      (if you want to be reachable from Internet this machine must have
       a public IP address and a domain name)
    - A local machine that you use to make the presentation
      This computer must:
      - reach the icecast2 server
      - have ardesia installed
    - Open the file $PREFIX/share/ardesia/scripts/screencast.sh
      or share\ardesia\scripts\screencast.bat if you use Windows
      - Uncomment the line
        - ICECAST="TRUE";
      - Set these variable
        - ICECAST_PASSWORD
        - ICECAST_ADDRESS
        - ICECAST_PORT
        - ICECAST_MOUNTPOINT
    - Open Ardesia
    - Push the record button
    - Select a name
    - Enjoy the stream at the uri
      http://ICECAST_LOCATION:ICECAST_PORT/ICECAST_MOUNTPOINT.ogg
    - Now you can put this link on a web page or publish it in an other way
    - For furthermore information about icecast2 server please
      visit http://www.icecast.org/


# Original Author Info

* To get info about the tool, please contact:
      pilolli.pietro@gmail.com
* To report bugs, please contact:
      pilolli.pietro@gmail.com


Have fun!



