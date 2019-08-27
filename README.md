# mpdbus
mpdbus is a basic mpd client that exposes its controls via dbus following the mpris specification.

## Installation
Clone this repository, cd in the cloned directory then use `make install`. To uninstall use `make uninstall`.
This program depends on `glib2` and `libmpdclient` so you may need to install these before.

## Usage
Open a terminal and launch the program using the `mpdbus` command.

```
Usage: mpdbus [OPTIONS]
  -d, --daemon          Launch as a daemon.
  -h, --help            Show this message.
  -p, --password        mpd server password (default: none).
  -H, --host            mpd server host (default: localhost).
  -P, --port            mpd server port (default: 6600).
```

You can now monitor/control mpd using dbus. Use a tool like `d-feet` to inspect the dbus interfaces under the name `org.mpris.MediaPlayer2.mpd` in the session bus or check the online specification of mpris [here](https://specifications.freedesktop.org/mpris-spec/latest/).

## TODO
- Figure out how to make it work as a systemd service.
- more metadata
- implement OpenUri method
- implement playlists interface
- fix small position desync
- sometimes could not get song error is emited but it should not quit.
- fix LoopStatus None <-> Playlist no event interepted
- mpd password support
