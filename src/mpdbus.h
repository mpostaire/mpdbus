#ifndef MPDBUS_H
#define MPDBUS_H

#include <gio/gio.h>

#define OBJECT_PATH "/org/mpris/MediaPlayer2"
#define DBUS_NAME "org.mpris.MediaPlayer2.mpd"
#define INTERFACE_MPRIS "org.mpris.MediaPlayer2"
#define INTERFACE_PLAYER "org.mpris.MediaPlayer2.Player"
#define TRACK_ID_PATH "/org/mpris/MediaPlayer2/Track/%d"

void send_properties_changed_signal(const gchar *interface_name, GVariantBuilder *changed, GVariantBuilder *invalidated);

#endif
