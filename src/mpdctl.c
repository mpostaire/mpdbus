#include "mpdctl.h"
#include "mpdbus.h"
#include <gio/gio.h>
#include <glib.h>
#include <mpd/async.h>
#include <mpd/client.h>
#include <stdio.h>
#include <stdlib.h>

#define IDENTITY "Music Player Daemon"

#define BUF_SIZE 128

static struct mpd_connection *mpd;
static struct player *player;
static struct mpris *mpris;
static struct metadata *metadata;

void mpdctl_toggle_pause() {
    if (g_strcmp0(player->playback_status, PLAYBACK_STATUS_STOPPED) == 0) {
        mpdctl_play();
        return;
    }

    // we leave idle mode
    mpd_run_noidle(mpd);

    bool ret = mpd_run_toggle_pause(mpd);
    if (!ret) {
        fprintf(stderr, "mpd: could not toggle pause\n");
        // maybe exiting here is an overkill (we should at least try again).
        exit(EXIT_FAILURE);
    }

    // we reenter idle mode
    ret = mpd_send_idle(mpd);
    if (!ret) {
        fprintf(stderr, "mpd: could not set idle state\n");
        // maybe exiting here is an overkill (we should at least try again).
        exit(EXIT_FAILURE);
    }
}

void mpdctl_pause() {
    if (g_strcmp0(player->playback_status, PLAYBACK_STATUS_STOPPED) == 0) {
        return;
    }

    // we leave idle mode
    mpd_run_noidle(mpd);

    bool ret = mpd_run_pause(mpd, true);
    if (!ret) {
        fprintf(stderr, "mpd: could not pause\n");
        // maybe exiting here is an overkill (we should at least try again).
        exit(EXIT_FAILURE);
    }

    // we reenter idle mode
    ret = mpd_send_idle(mpd);
    if (!ret) {
        fprintf(stderr, "mpd: could not set idle state\n");
        // maybe exiting here is an overkill (we should at least try again).
        exit(EXIT_FAILURE);
    }
}

void mpdctl_previous() {
    if (g_strcmp0(player->playback_status, PLAYBACK_STATUS_STOPPED) == 0)
        return;

    // we leave idle mode
    mpd_run_noidle(mpd);

    bool ret = mpd_run_previous(mpd);
    if (!ret) {
        fprintf(stderr, "mpd: could not change to previous track\n");
        // maybe exiting here is an overkill (we should at least try again).
        exit(EXIT_FAILURE);
    }

    // we reenter idle mode
    ret = mpd_send_idle(mpd);
    if (!ret) {
        fprintf(stderr, "mpd: could not set idle state\n");
        // maybe exiting here is an overkill (we should at least try again).
        exit(EXIT_FAILURE);
    }
}

void mpdctl_stop() {
    // we leave idle mode
    mpd_run_noidle(mpd);

    bool ret = mpd_run_stop(mpd);
    if (!ret) {
        fprintf(stderr, "mpd: could not stop\n");
        // maybe exiting here is an overkill (we should at least try again).
        exit(EXIT_FAILURE);
    }

    // we reenter idle mode
    ret = mpd_send_idle(mpd);
    if (!ret) {
        fprintf(stderr, "mpd: could not set idle state\n");
        // maybe exiting here is an overkill (we should at least try again).
        exit(EXIT_FAILURE);
    }
}

void mpdctl_next() {
    if (g_strcmp0(player->playback_status, PLAYBACK_STATUS_STOPPED) == 0)
        return;

    // we leave idle mode
    mpd_run_noidle(mpd);

    bool ret = mpd_run_next(mpd);
    if (!ret) {
        fprintf(stderr, "mpd: could not change to next track\n");
        // maybe exiting here is an overkill (we should at least try again).
        exit(EXIT_FAILURE);
    }

    // we reenter idle mode
    ret = mpd_send_idle(mpd);
    if (!ret) {
        fprintf(stderr, "mpd: could not set idle state\n");
        // maybe exiting here is an overkill (we should at least try again).
        exit(EXIT_FAILURE);
    }
}

void mpdctl_seek(float position, bool relative) {
    if (g_strcmp0(player->playback_status, PLAYBACK_STATUS_STOPPED) == 0)
        return;

    // we leave idle mode
    mpd_run_noidle(mpd);

    bool ret = mpd_run_seek_current(mpd, position, relative);
    if (!ret) {
        fprintf(stderr, "mpd: could not seek current track\n");
        // maybe exiting here is an overkill (we should at least try again).
        exit(EXIT_FAILURE);
    }

    // we reenter idle mode
    ret = mpd_send_idle(mpd);
    if (!ret) {
        fprintf(stderr, "mpd: could not set idle state\n");
        // maybe exiting here is an overkill (we should at least try again).
        exit(EXIT_FAILURE);
    }
}

void mpdctl_play() {
    // we leave idle mode
    mpd_run_noidle(mpd);

    bool ret = mpd_run_play(mpd);
    if (!ret) {
        fprintf(stderr, "mpd: could not start playing\n");
        // maybe exiting here is an overkill (we should at least try again).
        exit(EXIT_FAILURE);
    }

    // we reenter idle mode
    ret = mpd_send_idle(mpd);
    if (!ret) {
        fprintf(stderr, "mpd: could not set idle state\n");
        // maybe exiting here is an overkill (we should at least try again).
        exit(EXIT_FAILURE);
    }
}

void mpdctl_set_volume(guint volume) {
    // we leave idle mode
    mpd_run_noidle(mpd);

    bool ret = mpd_run_set_volume(mpd, volume);
    if (!ret) {
        fprintf(stderr, "mpd: could not set volume\n");
        // maybe exiting here is an overkill (we should at least try again).
        exit(EXIT_FAILURE);
    }

    // we reenter idle mode
    ret = mpd_send_idle(mpd);
    if (!ret) {
        fprintf(stderr, "mpd: could not set idle state\n");
        // maybe exiting here is an overkill (we should at least try again).
        exit(EXIT_FAILURE);
    }
}

void mpdctl_set_repeat(bool repeat) {
    // we leave idle mode
    mpd_run_noidle(mpd);

    bool ret = mpd_run_repeat(mpd, repeat);
    if (!ret) {
        fprintf(stderr, "mpd: could not set repeat state\n");
        // maybe exiting here is an overkill (we should at least try again).
        exit(EXIT_FAILURE);
    }

    // we reenter idle mode
    ret = mpd_send_idle(mpd);
    if (!ret) {
        fprintf(stderr, "mpd: could not set idle state\n");
        // maybe exiting here is an overkill (we should at least try again).
        exit(EXIT_FAILURE);
    }
}

void mpdctl_set_single(bool single) {
    // we leave idle mode
    mpd_run_noidle(mpd);

    bool ret = mpd_run_single(mpd, single);
    if (!ret) {
        fprintf(stderr, "mpd: could not set single state\n");
        // maybe exiting here is an overkill (we should at least try again).
        exit(EXIT_FAILURE);
    }

    // we reenter idle mode
    ret = mpd_send_idle(mpd);
    if (!ret) {
        fprintf(stderr, "mpd: could not set idle state\n");
        // maybe exiting here is an overkill (we should at least try again).
        exit(EXIT_FAILURE);
    }
}

void mpdctl_set_random(bool random) {
    // we leave idle mode
    mpd_run_noidle(mpd);

    bool ret = mpd_run_random(mpd, random);
    if (!ret) {
        fprintf(stderr, "mpd: could not set random state\n");
        // maybe exiting here is an overkill (we should at least try again).
        exit(EXIT_FAILURE);
    }

    // we reenter idle mode
    ret = mpd_send_idle(mpd);
    if (!ret) {
        fprintf(stderr, "mpd: could not set idle state\n");
        // maybe exiting here is an overkill (we should at least try again).
        exit(EXIT_FAILURE);
    }
}

static void mpdctl_update() {
    struct mpd_status *status = mpd_run_status(mpd);
    if (status == NULL) {
        fprintf(stderr, "mpd: could not get status\n");
        // maybe exiting here is an overkill (we should at least try again).
        exit(EXIT_FAILURE);
    }

    GVariantBuilder *changed_builder = g_variant_builder_new(G_VARIANT_TYPE_ARRAY);
    GVariantBuilder *invalidated_builder = g_variant_builder_new(G_VARIANT_TYPE("as"));
    GVariantBuilder *metadata_builder = NULL;
    GVariant *metadata_variant = NULL;

    // update playback status
    enum mpd_state playback_status = mpd_status_get_state(status);
    gboolean playback_status_changed = FALSE;
    switch (playback_status) {
    case MPD_STATE_PLAY:
        if (g_strcmp0(player->playback_status, PLAYBACK_STATUS_PLAYING) != 0) {
            g_strlcpy(player->playback_status, PLAYBACK_STATUS_PLAYING, 8);
            playback_status_changed = TRUE;
        }
        break;
    case MPD_STATE_PAUSE:
        if (g_strcmp0(player->playback_status, PLAYBACK_STATUS_PAUSED) != 0) {
            g_strlcpy(player->playback_status, PLAYBACK_STATUS_PAUSED, 8);
            playback_status_changed = TRUE;
        }
        break;
    case MPD_STATE_STOP:
        if (g_strcmp0(player->playback_status, PLAYBACK_STATUS_STOPPED) != 0) {
            g_strlcpy(player->playback_status, PLAYBACK_STATUS_STOPPED, 8);
            playback_status_changed = TRUE;
        }
        break;
    }

    if (playback_status_changed) {
        g_variant_builder_add(changed_builder, "{sv}",
                              "PlaybackStatus",
                              g_variant_new_string(player->playback_status));
    }

    // update loop status
    bool repeat = mpd_status_get_repeat(status);
    bool single = mpd_status_get_single(status);
    gboolean loop_status_changed = FALSE;
    if (repeat && single) {
        if (g_strcmp0(player->loop_status, LOOP_STATUS_TRACK) != 0) {
            g_strlcpy(player->loop_status, LOOP_STATUS_TRACK, 9);
            loop_status_changed = TRUE;
        }
    } else if (repeat && !single) {
        if (g_strcmp0(player->loop_status, LOOP_STATUS_PLAYLIST) != 0) {
            g_strlcpy(player->loop_status, LOOP_STATUS_PLAYLIST, 9);
            loop_status_changed = TRUE;
        }
    } else if (g_strcmp0(player->loop_status, LOOP_STATUS_NONE) != 0) {
        g_strlcpy(player->loop_status, LOOP_STATUS_NONE, 9);
        loop_status_changed = TRUE;
    }
    if (loop_status_changed) {
        g_variant_builder_add(changed_builder, "{sv}",
                              "LoopStatus",
                              g_variant_new_string(player->loop_status));
    }

    // update shuffle
    gboolean shuffle_changed = FALSE;
    bool shuffle = mpd_status_get_random(status);
    if (player->shuffle != shuffle) {
        player->shuffle = shuffle;
        shuffle_changed = TRUE;
        g_variant_builder_add(changed_builder, "{sv}",
                              "Shuffle",
                              g_variant_new_boolean(player->shuffle));
    }

    // update volume
    gboolean volume_changed = FALSE;
    gdouble volume = (gdouble) mpd_status_get_volume(status) / 100.0;
    if (player->volume != volume) {
        player->volume = volume;
        volume_changed = TRUE;
        g_variant_builder_add(changed_builder, "{sv}",
                              "Volume",
                              g_variant_new_double(player->volume));
    }

    // update position
    gboolean position_changed = FALSE;
    gint64 position = mpd_status_get_elapsed_ms(status) * 1000;
    if (player->position != position) {
        player->position = position;
        position_changed = TRUE;
        g_variant_builder_add(changed_builder, "{sv}",
                              "Position",
                              g_variant_new_int64(player->position));
    }

    // TODO: these properties should not always be TRUE (see mpris spec)
    player->can_control = TRUE;
    player->can_go_next = TRUE;
    player->can_go_previous = TRUE;
    player->can_pause = TRUE;
    player->can_play = TRUE;
    player->can_seek = TRUE;

    // update metadata
    struct mpd_song *song = mpd_run_current_song(mpd);
    if (song == NULL) {
        fprintf(stderr, "mpd: could not get song\n");
        // maybe exiting here is an overkill (we should at least try again).
        exit(EXIT_FAILURE);
    }

    // check if this is a new track (it not there is no need to update metadata)
    gboolean metadata_changed = FALSE;
    guint id = mpd_song_get_id(song);
    if (metadata->id == id) {
        goto out;
    }
    metadata_changed = TRUE;

    metadata_builder = g_variant_builder_new(G_VARIANT_TYPE("a{sv}"));

    metadata->id = id;
    char buf[128];
    int r = snprintf(buf, 128, TRACK_ID_PATH, mpdctl_get_metadata()->id);
    g_variant_builder_add(metadata_builder, "{sv}", "mpris:trackid", g_variant_new_object_path(buf));

    g_strlcpy(metadata->album, mpd_song_get_tag(song, MPD_TAG_ALBUM, 0), BUF_SIZE);
    g_variant_builder_add(metadata_builder, "{sv}", "xesam:album", g_variant_new_string(mpdctl_get_metadata()->album));

    g_strlcpy(metadata->title, mpd_song_get_tag(song, MPD_TAG_TITLE, 0), BUF_SIZE);
    g_variant_builder_add(metadata_builder, "{sv}", "xesam:title", g_variant_new_string(mpdctl_get_metadata()->title));

    g_strlcpy(metadata->artist, mpd_song_get_tag(song, MPD_TAG_ARTIST, 0), BUF_SIZE);
    g_variant_builder_add(metadata_builder, "{sv}", "xesam:artist", g_variant_new_string(mpdctl_get_metadata()->artist));

    metadata->length = mpd_song_get_duration_ms(song) * 1000;
    g_variant_builder_add(metadata_builder, "{sv}", "mpris:length", g_variant_new_string(mpdctl_get_metadata()->artist));

    // TODO: more metadata
    // mpd_song_get_uri(song);

    metadata_variant = g_variant_builder_end(metadata_builder);
    g_variant_ref_sink(metadata_variant);

    g_variant_builder_add(changed_builder, "{sv}",
                          "Metadata",
                          metadata_variant);

out:
    // we can't send an empty changed builder (TODO: make this check more clever)
    if (playback_status_changed || loop_status_changed || shuffle_changed || volume_changed || position_changed || metadata_changed)
        send_properties_changed_signal(INTERFACE_PLAYER, changed_builder, invalidated_builder);

    if (metadata_variant != NULL)
        g_variant_unref(metadata_variant);
    if (metadata_builder != NULL)
        g_variant_builder_unref(metadata_builder);
    g_variant_builder_unref(changed_builder);
    g_variant_builder_unref(invalidated_builder);
    mpd_status_free(status);
    mpd_song_free(song);
}

static gboolean check_mpd_event(GIOChannel *source, GIOCondition condition, gpointer data) {
    // we receive what type of event it was
    enum mpd_idle flags = mpd_recv_idle(mpd, true);
    const char *buf = mpd_idle_name(flags);
    g_print("mpd: %s event received\n", buf);

    switch (flags) {
    case MPD_IDLE_PLAYER:
        // the player has been started, stopped or seeked
        mpdctl_update();
        break;
    case MPD_IDLE_OPTIONS:
        // options like repeat, random, crossfade, replay gain
        // repeat and random does not seem to work but single does with my functions
        // setting these with ncmpcpp for example works and they are detected
        mpdctl_update();
        break;
    case MPD_IDLE_MIXER:
        // the volume has been changed
        mpdctl_update();
        break;
    default:
        fprintf(stderr, "mpd: no support for %s event\n", buf);
        break;
    }

    // we reenter idle mode after parsing
    bool ret = mpd_send_idle(mpd);
    if (!ret) {
        fprintf(stderr, "mpd: could not set idle state\n");
        // maybe exiting here is an overkill (we should at least try again).
        exit(EXIT_FAILURE);
    }
    return TRUE;
}

struct mpris *mpdctl_get_mpris() {
    return mpris;
}

struct player *mpdctl_get_player() {
    return player;
}

struct metadata *mpdctl_get_metadata() {
    return metadata;
}

static void mpris_free(struct mpris *mpris) {
    free(mpris->identity);
    free(mpris);
}

static void player_free(struct player *player) {
    free(player->playback_status);
    free(player->loop_status);
    free(player);
}

static void metadata_free(struct metadata *metadata) {
    free(metadata->album);
    free(metadata->artist);
    free(metadata->title);
    free(metadata);
}

static struct mpris *mpris_new() {
    struct mpris *temp = (struct mpris *) malloc(sizeof(struct mpris));
    temp->can_quit = FALSE;
    temp->can_raise = FALSE;
    temp->has_track_list = FALSE;
    temp->identity = (gchar *) malloc(sizeof(IDENTITY));
    g_strlcpy(temp->identity, IDENTITY, sizeof(IDENTITY));
    return temp;
}

static struct player *player_new() {
    struct player *temp = (struct player *) malloc(sizeof(struct player));
    temp->playback_status = (gchar *) malloc(8);
    temp->loop_status = (gchar *) malloc(9);
    temp->rate = 1.0;
    temp->minimum_rate = 1.0;
    temp->maximum_rate = 1.0;
    return temp;
}

static struct metadata *metadata_new() {
    struct metadata *temp = (struct metadata *) malloc(sizeof(struct metadata));
    temp->album = (gchar *) malloc(BUF_SIZE);
    temp->title = (gchar *) malloc(BUF_SIZE);
    temp->artist = (gchar *) malloc(BUF_SIZE);
    return temp;
}

void mpdctl_close() {
    mpd_connection_free(mpd);
    mpris_free(mpris);
    player_free(player);
    metadata_free(metadata);
}

void mpdctl_start(const char *host, const int port) {
    mpris = mpris_new();
    player = player_new();
    metadata = metadata_new();

    // we can begin to monitor mpd
    // TODO: add password support
    mpd = mpd_connection_new(host, port, 0);

    if (mpd_connection_get_error(mpd) != MPD_ERROR_SUCCESS) {
        const char *err = mpd_connection_get_error_message(mpd);
        fprintf(stderr, "mpd connection failed with error: %s\n", err);
        free((char *) err);
        exit(EXIT_FAILURE);
    }

    mpd_connection_set_keepalive(mpd, true);
    int fd = mpd_connection_get_fd(mpd);

    GIOChannel *channel = g_io_channel_unix_new(fd);
    g_io_add_watch(channel, G_IO_IN, check_mpd_event, NULL);

    // update initial mpd properties
    mpdctl_update();

    bool ret = mpd_send_idle(mpd);
    if (!ret) {
        fprintf(stderr, "mpd: could not set idle state\n");
        // maybe exiting here is an overkill (we should at least try again).
        exit(EXIT_FAILURE);
    }

    g_print("connected to mpd at %s:%d\n", host, port);
}
