#ifndef MPDCTL_H
#define MPDCTL_H

#include <gio/gio.h>
#include <stdbool.h>

#define LOOP_STATUS_NONE "None"
#define LOOP_STATUS_TRACK "Track"
#define LOOP_STATUS_PLAYLIST "Playlist"

#define PLAYBACK_STATUS_PLAYING "Playing"
#define PLAYBACK_STATUS_PAUSED "Paused"
#define PLAYBACK_STATUS_STOPPED "Stopped"

struct mpris {
    gboolean can_quit, can_raise, has_track_list;
    gchar *identity;
    gchar **supported_mime_types, supported_uri_schemes;
};

struct player {
    gboolean can_control, can_go_next, can_go_previous,
        can_pause, can_play, can_seek, shuffle;
    gdouble maximum_rate, minimum_rate, rate, volume;
    gint64 position, length;
    gchar *loop_status, *playback_status;
};

struct metadata {
    gchar *album, *title, *artist;
    guint id;
    gint64 length;
};

void mpdctl_start(const char *host, const int port);

void mpdctl_close();

struct mpris *mpdctl_get_mpris();

struct player *mpdctl_get_player();

struct metadata *mpdctl_get_metadata();

void mpdctl_next();

void mpdctl_previous();

void mpdctl_toggle_pause();

void mpdctl_stop();

void mpdctl_play();

void mpdctl_pause();

void mpdctl_set_volume(guint volume);

void mpdctl_seek(float position, bool relative);

void mpdctl_set_repeat(bool repeat);

void mpdctl_set_single(bool single);

void mpdctl_set_random(bool random);

#endif
