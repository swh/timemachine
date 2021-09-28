# TimeMachine

I used to always keep a minidisc recorder in my studio running in a mode where
when you pressed record it wrote the last 10 seconds of audio to the disk and
then caught up to realtime and kept recording. The recorder died and haven't
been able to replace it, so this is a simple jack app to do the same job. It
has the advantage that it never clips and can be wired to any part of the jack
graph.

The idea is that I doodle away with whatever is kicking around in my studio
and when I heard an interesting noise, I'd press record and capture it,
without having to try and recreate it. :)

## Building

This app requires:

* JACK
* GTK+ 2.x
* libsndfile

and optionally:
* LASH
* liblo

You will also need the -devel packages if you're using packages.

    ./configure
    make
    su -c "make install"

## Usage

Run it with "timemachine" then connect it up with a jack patchbay app. To
start recording click in the window, to stop recording click in the window
again.

It will create a file following tm-*.wav, with an ISO 8601 timestamp, eg
tm-2003-01-19T20:47:03.wav. The time is the time that the recording starts
from, not when you click.

## OSC

timemachine supports starting stopping via OSC, triggered by sending a message with the path `/start` or `/stop` and no arguments.

Example Python code:

    import liblo
    liblo.send(('localhost',7133), '/start')
    liblo.send(('localhost',7133), '/stop')

## Contact

You can report bugs through github at https://github.com/swh/timemachine
