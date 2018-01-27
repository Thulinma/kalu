/**
 * kalu - Copyright (C) 2012-2016 Olivier Brunel
 *
 * gui.c
 * Copyright (C) 2012-2016 Olivier Brunel <jjk@jjacky.com>
 *
 * This file is part of kalu.
 *
 * kalu is free software: you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later
 * version.
 *
 * kalu is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * kalu. If not, see http://www.gnu.org/licenses/
 */

#include <config.h>

/* kalu */
#include "kalu.h"
#include "gui.h"
#include "util-gtk.h"
#include "watched.h"
#include "preferences.h"
#include "conf.h"
#include "news.h"
#include "rt_timeout.h"
#include "imagemenuitem.h"
#ifndef DISABLE_UPDATER
#include "kalu-updater.h"
#include "updater.h"
#endif

#ifndef DISABLE_UPDATER
#define run_updater()   do {            \
    set_kalpm_busy (TRUE);              \
    updater_run (config->pacmanconf,    \
            config->cmdline_post);      \
} while (0)
#endif

#ifndef DISABLE_UPDATER
static void notif_run_simulation (NotifyNotification  *notification,
                                  const gchar         *action _UNUSED_,
                                  gpointer             data _UNUSED_);
#endif
static void menu_check_cb (GtkMenuItem *item, gpointer data);
static void menu_quit_cb (GtkMenuItem *item, gpointer data);
static gboolean set_status_icon (gboolean active);
#ifdef ENABLE_STATUS_NOTIFIER
static void sn_upd_status (gboolean active);
static void sn_refresh_tooltip (void);
#endif

extern kalpm_state_t kalpm_state;


void
free_notif (notif_t *notif)
{
    if (!notif)
    {
        return;
    }

    free (notif->summary);
    free (notif->text);
    if (notif->type == CHECK_NEWS || notif->type == CHECK_AUR)
    {
        /* CHECK_NEWS has xml_news; CHECK_AUR has cmdline w/ $PACKAGES replaced */
        free (notif->data);
    }
    else
    {
        /* CHECK_UPGRADES, CHECK_WATCHED & CHECK_WATCHED_AUR all use packages */
        FREE_PACKAGE_LIST (notif->data);
    }
    free (notif);
}

void
show_notif (notif_t *notif)
{
    NotifyNotification *notification;

    debug ("showing notif: %s\n%s\n--- EOF ---", notif->summary, notif->text);
    notification = new_notification (notif->summary, notif->text);
    if (config->notif_buttons)
    {
        if (notif->type & CHECK_UPGRADES)
        {
            if (!notif->data)
            {
                /* no data in this case means this is an error message about a
                 * conflict, in which case we still add the "Update system"
                 * button/action */
#ifndef DISABLE_UPDATER
                /* We also add the "Run simulation" button */
                notify_notification_add_action (notification, "run_simulation",
                        _c("notif-button", "Run simulation..."),
                        (NotifyActionCallback) notif_run_simulation,
                        NULL, NULL);
#endif
            }
            else if (config->check_pacman_conflict
                    && is_pacman_conflicting ((alpm_list_t *) notif->data))
            {
                notify_notification_add_action (notification, "do_conflict_warn",
                        _c("notif-button", "Possible pacman/kalu conflict..."),
                        (NotifyActionCallback) show_pacman_conflict,
                        NULL, NULL);
            }
            if (config->action != UPGRADE_NO_ACTION)
            {
                notify_notification_add_action (notification, "do_updates",
                        _c("notif-button", "Update system..."),
                        (NotifyActionCallback) action_upgrade,
                        NULL, NULL);
            }
        }
        else if (!notif->data)
        {
            /* no data means the notification was modified afterwards, as
             * news/packages have been marked read. No more data/action button,
             * just a simple notification (where text explains to re-do checks
             * to be up to date) */
        }
        else if (notif->type & CHECK_AUR)
        {
            notify_notification_add_action (notification, "do_updates_aur",
                    _c("notif-button", "Update AUR packages..."),
                    (NotifyActionCallback) action_upgrade,
                    notif->data,
                    NULL);
        }
        else if (notif->type & CHECK_WATCHED)
        {
            notify_notification_add_action (notification, "mark_watched",
                    _c("notif-button", "Mark packages..."),
                    (NotifyActionCallback) action_watched,
                    notif,
                    NULL);
        }
        else if (notif->type & CHECK_WATCHED_AUR)
        {
            notify_notification_add_action (notification, "mark_watched_aur",
                    _c("notif-button", "Mark packages..."),
                    (NotifyActionCallback) action_watched_aur,
                    notif,
                    NULL);
        }
        else if (notif->type & CHECK_NEWS)
        {
            notify_notification_add_action (notification, "mark_news",
                    _c("notif-button", "Show news..."),
                    (NotifyActionCallback) action_news,
                    notif,
                    NULL);
        }
    }
    /* we use a callback on "closed" to unref it, because when there's an action
     * we need to keep a ref, otherwise said action won't work */
    g_signal_connect (G_OBJECT (notification), "closed",
            G_CALLBACK (notification_closed_cb), NULL);
    notify_notification_show (notification, NULL);
}

gboolean
show_error_cmdline (gchar *arg[])
{
    GtkWidget *dialog;

    dialog = gtk_message_dialog_new (NULL,
            GTK_DIALOG_DESTROY_WITH_PARENT,
            GTK_MESSAGE_ERROR,
            GTK_BUTTONS_OK,
            "%s",
            _("Unable to start process"));
    gtk_message_dialog_format_secondary_markup (GTK_MESSAGE_DIALOG (dialog),
            _("Error while trying to run command line: %s\n\nThe error was: <b>%s</b>"),
            arg[0],
            arg[1]);
    gtk_window_set_title (GTK_WINDOW (dialog), _("kalu: Unable to start process"));
    gtk_window_set_decorated (GTK_WINDOW (dialog), FALSE);
    gtk_window_set_skip_taskbar_hint (GTK_WINDOW (dialog), FALSE);
    gtk_window_set_skip_pager_hint (GTK_WINDOW (dialog), FALSE);
    gtk_dialog_run (GTK_DIALOG (dialog));
    gtk_widget_destroy (dialog);
    free (arg[0]);
    free (arg[1]);
    free (arg);
    return FALSE;
}

static void
run_cmdline (char *cmdline)
{
    GError *error = NULL;

    set_kalpm_busy (TRUE);
    debug ("run cmdline: %s", cmdline);
    if (!g_spawn_command_line_sync (cmdline, NULL, NULL, NULL, &error))
    {
        /* we can't just show the error message from here, because this is ran
         * from another thread, hence no gtk_* functions can be used. */
        char **arg;
        arg = new (char *, 2);
        arg[0] = strdup (cmdline);
        arg[1] = strdup (error->message);
        g_main_context_invoke (NULL, (GSourceFunc) show_error_cmdline, (gpointer) arg);
    }
    set_kalpm_busy (FALSE);
    if (G_UNLIKELY (error))
    {
        g_clear_error (&error);
    }
    else
    {
        /* check again, to refresh the state (since an upgrade was probably just done) */
        kalu_check (TRUE);
    }

    if (cmdline != config->cmdline && cmdline != config->cmdline_aur)
    {
        free (cmdline);
    }
}

#ifndef DISABLE_UPDATER
static gboolean
run_simulation (NotifyNotification *notification)
{
    if (kalpm_state.is_busy)
    {
        show_error (_("Cannot run simulation: kalu is busy"),
                _("kalu cannot run a simulation while it is busy "
                    "(e.g. checking/upgrading the system).\n"
                    "Please wait until kalu isn't busy anymore and try again."),
                NULL);
        return FALSE;
    }

    if (notification)
    {
        notify_notification_close (notification, NULL);
    }

    set_kalpm_busy (TRUE);
    updater_run (NULL, NULL);
    return TRUE;
}

static void
notif_run_simulation (NotifyNotification  *notification,
                      const gchar         *action _UNUSED_,
                      gpointer             data _UNUSED_)
{
    if (!run_simulation (notification))
        /* restore if hidden */
        notify_notification_show (notification, NULL);
}
#endif

void
action_upgrade (NotifyNotification *notification, const char *action, gchar *_cmdline)
{
    /* action can be "do_updates" or "do_updates_aur" -- In the former case,
     * _cmdline is NULL, in the later case, it might contain the cmdline to run
     * (with $PACKAGES replaces with the list of packages to update) */
    gboolean do_updates = streq (action, "do_updates");
    gchar *cmdline;

    if (kalpm_state.is_busy)
    {
        if (do_updates)
            show_error (_("Cannot upgrade system: kalu is busy"),
                    _("kalu cannot upgrade the system while it is busy "
                        "(e.g. checking/upgrading the system).\n"
                        "Please wait until kalu isn't busy anymore and try again."),
                    NULL);
        else
            show_error (_("Cannot launch AUR upgrade: kalu is busy"),
                    _("kalu cannot launch AUR upgrade while it is busy "
                        "(e.g. checking/upgrading the system).\n"
                        "Please wait until kalu isn't busy anymore and try again."),
                    NULL);
        /* restore if hidden */
        notify_notification_show (notification, NULL);
        return;
    }

    /* we need to strdup cmdline because it will be free-d when notification
     * is destroyed, while we need it up to the other thread (run_cmdline) */
    cmdline = (_cmdline) ? strdup (_cmdline) : NULL;
    notify_notification_close (notification, NULL);

    if (!cmdline)
    {
        if (do_updates)
        {
#ifndef DISABLE_UPDATER
            if (config->action == UPGRADE_ACTION_KALU)
            {
                run_updater ();
            }
            else /* if (config->action == UPGRADE_ACTION_CMDLINE) */
            {
#endif
                cmdline = config->cmdline;
#ifndef DISABLE_UPDATER
            }
#endif
        }
        else
        {
            cmdline = config->cmdline_aur;
        }
    }

    if (cmdline)
    {
        /* run in a separate thread, to not block/make GUI unresponsive */
        g_thread_unref (g_thread_try_new ("action_upgrade",
                    (GThreadFunc) run_cmdline,
                    (gpointer) cmdline,
                    NULL));
    }
}

void
action_watched (NotifyNotification *notification, char *action _UNUSED_,
    notif_t *notif)
{
    notify_notification_close (notification, NULL);
    if (notif->data)
    {
        watched_update ((alpm_list_t *) notif->data, FALSE);
    }
    else
    {
        show_error (_("Unable to mark watched packages"),
                _("Watched packages have changed, "
                    "you need to run the checks again to be up-to-date."),
                NULL);
    }
}

void
action_watched_aur (NotifyNotification *notification, char *action _UNUSED_,
    notif_t *notif)
{
    notify_notification_close (notification, NULL);
    if (notif->data)
    {
        watched_update ((alpm_list_t *) notif->data, TRUE);
    }
    else
    {
        show_error (_("Unable to mark watched AUR packages"),
                _("Watched AUR packages have changed, "
                    "you need to run the checks again to be up-to-date."),
                NULL);
    }
}

void
action_news (NotifyNotification *notification, char *action _UNUSED_,
    notif_t *notif)
{
    GError *error = NULL;

    notify_notification_close (notification, NULL);
    if (notif->data)
    {
        set_kalpm_busy (TRUE);
        if (!news_show ((gchar *) notif->data, TRUE, &error))
        {
            show_error (_("Unable to show the news"), error->message, NULL);
            g_clear_error (&error);
        }
    }
    else
    {
        show_error (_("Unable to show unread news"),
                _("Read news have changed, "
                    "you need to run the checks again to be up-to-date."),
                NULL);
    }
}

void
notification_closed_cb (NotifyNotification *notification, gpointer data _UNUSED_)
{
    g_object_unref (notification);
}

gboolean
is_pacman_conflicting (alpm_list_t *packages)
{
    gboolean ret = FALSE;
    alpm_list_t *i;
    kalu_package_t *pkg;
    char *s, *ss, *old, *new, *so, *sn;

    FOR_LIST (i, packages)
    {
        pkg = i->data;
        if (streq ("pacman", pkg->name))
        {
            /* because we'll mess with it */
            old = strdup (pkg->old_version);
            /* locate begining of (major) version number (might have epoch: before) */
            s = strchr (old, ':');
            if (s)
            {
                so = s + 1;
            }
            else
            {
                so = old;
            }

            s = strrchr (so, '-');
            if (!s)
            {
                /* should not be possible */
                free (old);
                break;
            }
            *s = '.';

            /* because we'll mess with it */
            new = strdup (pkg->new_version);
            /* locate begining of (major) version number (might have epoch: before) */
            s = strchr (new, ':');
            if (s)
            {
                sn = s + 1;
            }
            else
            {
                sn = new;
            }

            s = strrchr (sn, '-');
            if (!s)
            {
                /* should not be possible */
                free (old);
                free (new);
                break;
            }
            *s = '.';

            int nb = 0; /* to know which part (major/minor) we're dealing with */
            while ((s = strchr (so, '.')) && (ss = strchr (sn, '.')))
            {
                *s = '\0';
                *ss = '\0';
                ++nb;

                /* if major or minor goes up, API changes is likely and kalu's
                 * dependency will kick in */
                if (atoi (sn) > atoi (so))
                {
                    ret = TRUE;
                    break;
                }

                /* if nb is 2 this was the minor number, past this we don't care */
                if (nb == 2)
                {
                    break;
                }
                so = s + 1;
                sn = ss + 1;
            }

            free (old);
            free (new);
            break;
        }
    }

    return ret;
}

inline void
kalu_check (gboolean is_auto)
{
    /* in case e.g. the menu was shown (sensitive) before an auto-check started */
    if (kalpm_state.is_busy)
    {
        return;
    }
    set_kalpm_busy (TRUE);

    /* run in a separate thread, to not block/make GUI unresponsive */
    g_thread_unref (g_thread_try_new ("kalu_check_work",
                (GThreadFunc) kalu_check_work,
                GINT_TO_POINTER (is_auto),
                NULL));
}

gboolean
kalu_auto_check (void)
{
    kalu_check (TRUE);
    return G_SOURCE_REMOVE;
}

static void
show_last_notifs (void)
{
    alpm_list_t *i;

    /* in case e.g. the menu was shown (sensitive) before an auto-check started */
    if (kalpm_state.is_busy)
    {
        return;
    }

    if (!config->last_notifs)
    {
        notif_t notif;

        notif.type = 0;
        notif.summary = (gchar *) _("No notifications to show.");
        notif.text = NULL;
        notif.data = NULL;

        show_notif (&notif);
        return;
    }

    FOR_LIST (i, config->last_notifs)
    {
        show_notif ((notif_t *) i->data);
    }
}

static void
update_icon (void)
{
    gint nb_upgrades = (kalpm_state.nb_upgrades == UPGRADES_NB_CONFLICT)
        ? 1 : kalpm_state.nb_upgrades;
    /* thing is, this function can be called from another thread (e.g. from
     * kalu_check_work via set_kalpm_nb, which runs in a separate thread not to
     * block GUI...) but when that happens, we can't use gtk_* functions, i.e.
     * we can't change the status icon. so, this will make sure the call to
     * set_status_icon happens in the main thread */
    gboolean active = (nb_upgrades + kalpm_state.nb_watched
                       + kalpm_state.nb_aur + + kalpm_state.nb_aur_not_found
                       + kalpm_state.nb_watched_aur + kalpm_state.nb_news > 0);

    g_main_context_invoke (NULL,
                           (GSourceFunc) set_status_icon,
                           GINT_TO_POINTER (active));
#ifdef ENABLE_STATUS_NOTIFIER
    sn_upd_status (active);
#endif
}

static void
set_pause (gboolean paused)
{
    /* in case e.g. the menu was shown (sensitive) before an auto-check started */
    if (kalpm_state.is_busy || kalpm_state.is_paused == paused)
    {
        return;
    }

    kalpm_state.is_paused = paused;
    if (paused)
    {
        debug ("pausing: disable next auto-checks; update icon");

        /* remove auto-check timeout */
        if (kalpm_state.timeout > 0)
        {
            g_source_remove (kalpm_state.timeout);
            kalpm_state.timeout = 0;
        }

        update_icon ();
#ifdef ENABLE_STATUS_NOTIFIER
        sn_refresh_tooltip ();
#endif
    }
    else
    {
        debug ("resuming: starting auto-checks");
        kalu_check (TRUE);
    }
}

static void
menu_check_cb (GtkMenuItem *item _UNUSED_, gpointer data _UNUSED_)
{
    kalu_check (FALSE);
}

static void
menu_pause_cb (GtkMenuItem *item _UNUSED_, gpointer data _UNUSED_)
{
    set_pause (!kalpm_state.is_paused);
}

static void
menu_quit_cb (GtkMenuItem *item _UNUSED_, gpointer data _UNUSED_)
{
    /* in case e.g. the menu was shown (sensitive) before an auto-check started */
    if (kalpm_state.is_busy)
    {
        return;
    }
    gtk_main_quit ();
}

static void
menu_manage_cb (GtkMenuItem *item _UNUSED_, gboolean is_aur)
{
    watched_manage (is_aur);
}

static inline void
kalu_sysupgrade (void)
{
    /* in case e.g. the menu was shown (sensitive) before an auto-check started */
    if (kalpm_state.is_busy || config->action == UPGRADE_NO_ACTION)
    {
        return;
    }

#ifndef DISABLE_UPDATER
    if (config->action == UPGRADE_ACTION_KALU)
    {
        run_updater ();
    }
    else /* if (config->action == UPGRADE_ACTION_CMDLINE) */
    {
#endif
        /* run in a separate thread, to not block/make GUI unresponsive */
        g_thread_unref (g_thread_try_new ("cmd_sysupgrade",
                    (GThreadFunc) run_cmdline,
                    (gpointer) config->cmdline,
                    NULL));
#ifndef DISABLE_UPDATER
    }
#endif
}

static void
menu_news_cb (GtkMenuItem *item _UNUSED_, gpointer data)
{
    GError *error = NULL;
    gboolean unread_only = GPOINTER_TO_INT (data);

    /* in case e.g. the menu was shown (sensitive) before an auto-check started */
    if (kalpm_state.is_busy)
    {
        return;
    }
    set_kalpm_busy (TRUE);

    if (!news_show (NULL, unread_only, &error))
    {
        show_error (_("Unable to show the recent Arch Linux news"),
                error->message, NULL);
        g_clear_error (&error);
    }
}

static void
menu_help_cb (GtkMenuItem *item _UNUSED_, gpointer data _UNUSED_)
{
    GError *error = NULL;

    if (!show_help (&error))
    {
        show_error (_("Unable to show help"), error->message, NULL);
        g_clear_error (&error);
    }
}

static void
menu_history_cb (GtkMenuItem *item _UNUSED_, gpointer data _UNUSED_)
{
    GError *error = NULL;

    if (!show_history (&error))
    {
        show_error (_("Unable to show change log"), error->message, NULL);
        g_clear_error (&error);
    }
}

static void
menu_prefs_cb (GtkMenuItem *item _UNUSED_, gpointer data _UNUSED_)
{
    show_prefs ();
}

extern const char kalu_logo[];
extern size_t kalu_logo_size;
static void
menu_about_cb (GtkMenuItem *item _UNUSED_, gpointer data _UNUSED_)
{
    GtkAboutDialog  *about;
    GInputStream    *stream;
    GdkPixbuf       *pixbuf;
    const char *authors[] = {
        "Olivier Brunel", "Dave Gamble", "Pacman Development Team",
        NULL };
    const char *artists[] = { "Painless Rob", NULL };

    about = GTK_ABOUT_DIALOG (gtk_about_dialog_new ());
    stream = g_memory_input_stream_new_from_data (kalu_logo, kalu_logo_size, NULL);
    pixbuf = gdk_pixbuf_new_from_stream (stream, NULL, NULL);
    g_object_unref (G_OBJECT (stream));
    gtk_window_set_icon (GTK_WINDOW (about), pixbuf);
    gtk_about_dialog_set_logo (about, pixbuf);
    g_object_unref (G_OBJECT (pixbuf));

    gtk_about_dialog_set_program_name (about, PACKAGE_NAME);
    gtk_about_dialog_set_version (about, PACKAGE_VERSION);
    gtk_about_dialog_set_comments (about, PACKAGE_TAG);
    gtk_about_dialog_set_website (about, "http://jjacky.com/kalu");
    gtk_about_dialog_set_website_label (about, "http://jjacky.com/kalu");
    gtk_about_dialog_set_copyright (about, "Copyright (C) 2012-2016 Olivier Brunel");
    gtk_about_dialog_set_license_type (about, GTK_LICENSE_GPL_3_0);
    gtk_about_dialog_set_authors (about, authors);
    gtk_about_dialog_set_artists (about, artists);
    /* TRANSLATORS: Put your name here to get credit on the About window */
    gtk_about_dialog_set_translator_credits (about, _("translator-credits"));

    gtk_dialog_run (GTK_DIALOG (about));
    gtk_widget_destroy (GTK_WIDGET (about));
}

static gboolean
menu_unmap_cb (GtkWidget *menu, GdkEvent *event _UNUSED_, gpointer data _UNUSED_)
{
    gtk_widget_destroy (menu);
    g_object_unref (menu);
    return TRUE;
}

void
icon_popup_cb (GtkStatusIcon *_icon _UNUSED_, guint button, guint activate_time,
               gpointer data _UNUSED_)
{
    GtkWidget   *menu;
    GtkWidget   *item;
    GtkWidget   *image;
    guint        pos = 0;

    menu = gtk_menu_new ();

    item = donna_image_menu_item_new_with_label (
            _c("systray-menu", "Re-show last notifications..."));
    gtk_widget_set_sensitive (item, !kalpm_state.is_busy && config->last_notifs);
    image = gtk_image_new_from_icon_name ("edit-redo", GTK_ICON_SIZE_MENU);
    donna_image_menu_item_set_image (DONNA_IMAGE_MENU_ITEM (item), image);
    gtk_widget_set_tooltip_text (item,
            _("Show notifications from last ran checks"));
    g_signal_connect (G_OBJECT (item), "activate",
            G_CALLBACK (show_last_notifs), NULL);
    gtk_widget_show (item);
    gtk_menu_attach (GTK_MENU (menu), item, 0, 1, pos, pos + 1); ++pos;

    item = donna_image_menu_item_new_with_label ((kalpm_state.is_paused)
            ? _c("systray-menu", "Resume automatic checks")
            : _c("systray-menu", "Pause automatic checks"));
    gtk_widget_set_sensitive (item, !kalpm_state.is_busy);
    image = gtk_image_new_from_icon_name ((kalpm_state.is_paused)
            ? "media-playback-start"
            : "media-playback-pause",
            GTK_ICON_SIZE_MENU);
    donna_image_menu_item_set_image (DONNA_IMAGE_MENU_ITEM (item), image);
    gtk_widget_set_tooltip_text (item, (kalpm_state.is_paused)
            ? _("Resume automatic checks (starting now)")
            : _("Pause automatic checks (until resumed)"));
    g_signal_connect (G_OBJECT (item), "activate",
            G_CALLBACK (menu_pause_cb), NULL);
    gtk_widget_show (item);
    gtk_menu_attach (GTK_MENU (menu), item, 0, 1, pos, pos + 1); ++pos;

    item = gtk_separator_menu_item_new ();
    gtk_widget_show (item);
    gtk_menu_attach (GTK_MENU (menu), item, 0, 1, pos, pos + 1); ++pos;

    item = donna_image_menu_item_new_with_label (
            _c("systray-menu", "Check for Upgrades..."));
    gtk_widget_set_sensitive (item, !kalpm_state.is_busy);
    image = gtk_image_new_from_icon_name ("kalu", GTK_ICON_SIZE_MENU);
    donna_image_menu_item_set_image (DONNA_IMAGE_MENU_ITEM (item), image);
    gtk_widget_set_tooltip_text (item,
            _("Check if there are any upgrades available"));
    g_signal_connect (G_OBJECT (item), "activate",
            G_CALLBACK (menu_check_cb), NULL);
    gtk_widget_show (item);
    gtk_menu_attach (GTK_MENU (menu), item, 0, 1, pos, pos + 1); ++pos;

#ifndef DISABLE_UPDATER
    item = donna_image_menu_item_new_with_label (
            _c("systray-menu", "Upgrade simulation..."));
    gtk_widget_set_sensitive (item, !kalpm_state.is_busy);
    image = gtk_image_new_from_icon_name ("kalu", GTK_ICON_SIZE_MENU);
    donna_image_menu_item_set_image (DONNA_IMAGE_MENU_ITEM (item), image);
    gtk_widget_set_tooltip_text (item, _("Run a system upgrade simulation "
                "(to preview/help deal with conflicts)"));
    g_signal_connect_swapped (G_OBJECT (item), "activate",
            G_CALLBACK (run_simulation), NULL);
    gtk_widget_show (item);
    gtk_menu_attach (GTK_MENU (menu), item, 0, 1, pos, pos + 1); ++pos;
#endif

    if (config->action != UPGRADE_NO_ACTION)
    {
        item = donna_image_menu_item_new_with_label (
                _c("systray-menu", "System upgrade..."));
        gtk_widget_set_sensitive (item, !kalpm_state.is_busy);
        image = gtk_image_new_from_icon_name ("kalu", GTK_ICON_SIZE_MENU);
        donna_image_menu_item_set_image (DONNA_IMAGE_MENU_ITEM (item), image);
        gtk_widget_set_tooltip_text (item, _("Perform a system upgrade"));
        g_signal_connect (G_OBJECT (item), "activate",
                G_CALLBACK (kalu_sysupgrade), NULL);
        gtk_widget_show (item);
        gtk_menu_attach (GTK_MENU (menu), item, 0, 1, pos, pos + 1); ++pos;
    }

    item = gtk_separator_menu_item_new ();
    gtk_widget_show (item);
    gtk_menu_attach (GTK_MENU (menu), item, 0, 1, pos, pos + 1); ++pos;

    item = donna_image_menu_item_new_with_label (
            _c("systray-menu", "Show unread Arch Linux news..."));
    gtk_widget_set_sensitive (item, !kalpm_state.is_busy);
    image = gtk_image_new_from_icon_name ("kalu", GTK_ICON_SIZE_MENU);
    donna_image_menu_item_set_image (DONNA_IMAGE_MENU_ITEM (item), image);
    gtk_widget_set_tooltip_text (item, _("Show only unread Arch Linux news"));
    g_signal_connect (G_OBJECT (item), "activate",
            G_CALLBACK (menu_news_cb), GINT_TO_POINTER (1));
    gtk_widget_show (item);
    gtk_menu_attach (GTK_MENU (menu), item, 0, 1, pos, pos + 1); ++pos;

    item = donna_image_menu_item_new_with_label (
            _c("systray-menu", "Show recent Arch Linux news..."));
    gtk_widget_set_sensitive (item, !kalpm_state.is_busy);
    image = gtk_image_new_from_icon_name ("kalu", GTK_ICON_SIZE_MENU);
    donna_image_menu_item_set_image (DONNA_IMAGE_MENU_ITEM (item), image);
    gtk_widget_set_tooltip_text (item, _("Show most recent Arch Linux news"));
    g_signal_connect (G_OBJECT (item), "activate",
            G_CALLBACK (menu_news_cb), GINT_TO_POINTER (0));
    gtk_widget_show (item);
    gtk_menu_attach (GTK_MENU (menu), item, 0, 1, pos, pos + 1); ++pos;

    item = gtk_separator_menu_item_new ();
    gtk_widget_show (item);
    gtk_menu_attach (GTK_MENU (menu), item, 0, 1, pos, pos + 1); ++pos;

    item = donna_image_menu_item_new_with_label (
            _c("systray-menu", "Manage watched packages..."));
    g_signal_connect (G_OBJECT (item), "activate",
            G_CALLBACK (menu_manage_cb), (gpointer) FALSE);
    gtk_widget_show (item);
    gtk_menu_attach (GTK_MENU (menu), item, 0, 1, pos, pos + 1); ++pos;

    item = donna_image_menu_item_new_with_label (
            _c("systray-menu", "Manage watched AUR packages..."));
    g_signal_connect (G_OBJECT (item), "activate",
            G_CALLBACK (menu_manage_cb), (gpointer) TRUE);
    gtk_widget_show (item);
    gtk_menu_attach (GTK_MENU (menu), item, 0, 1, pos, pos + 1); ++pos;

    item = gtk_separator_menu_item_new ();
    gtk_widget_show (item);
    gtk_menu_attach (GTK_MENU (menu), item, 0, 1, pos, pos + 1); ++pos;

    item = donna_image_menu_item_new_with_label (
            _c("systray-menu", "Preferences"));
    image = gtk_image_new_from_icon_name ("preferences-desktop", GTK_ICON_SIZE_MENU);
    donna_image_menu_item_set_image (DONNA_IMAGE_MENU_ITEM (item), image);
    gtk_widget_set_tooltip_text (item, _("Edit preferences"));
    g_signal_connect (G_OBJECT (item), "activate",
            G_CALLBACK (menu_prefs_cb), NULL);
    gtk_widget_show (item);
    gtk_menu_attach (GTK_MENU (menu), item, 0, 1, pos, pos + 1); ++pos;

    item = gtk_separator_menu_item_new ();
    gtk_widget_show (item);
    gtk_menu_attach (GTK_MENU (menu), item, 0, 1, pos, pos + 1); ++pos;

    item = donna_image_menu_item_new_with_label (
            _c("systray-menu", "Help"));
    image = gtk_image_new_from_icon_name ("help-contents", GTK_ICON_SIZE_MENU);
    donna_image_menu_item_set_image (DONNA_IMAGE_MENU_ITEM (item), image);
    gtk_widget_set_tooltip_text (item, _("Show help (man page)"));
    g_signal_connect (G_OBJECT (item), "activate",
            G_CALLBACK (menu_help_cb), NULL);
    gtk_widget_show (item);
    gtk_menu_attach (GTK_MENU (menu), item, 0, 1, pos, pos + 1); ++pos;

    item = donna_image_menu_item_new_with_label (
            _c("systray-menu", "Change log"));
    gtk_widget_set_tooltip_text (item, _("Show change log"));
    g_signal_connect (G_OBJECT (item), "activate",
            G_CALLBACK (menu_history_cb), (gpointer) TRUE);
    gtk_widget_show (item);
    gtk_menu_attach (GTK_MENU (menu), item, 0, 1, pos, pos + 1); ++pos;

    item = donna_image_menu_item_new_with_label (
            _c("systray-menu", "About"));
    image = gtk_image_new_from_icon_name ("help-about", GTK_ICON_SIZE_MENU);
    donna_image_menu_item_set_image (DONNA_IMAGE_MENU_ITEM (item), image);
    gtk_widget_set_tooltip_text (item,
            _("Show Copyright & version information"));
    g_signal_connect (G_OBJECT (item), "activate",
            G_CALLBACK (menu_about_cb), NULL);
    gtk_widget_show (item);
    gtk_menu_attach (GTK_MENU (menu), item, 0, 1, pos, pos + 1); ++pos;

    item = gtk_separator_menu_item_new ();
    gtk_widget_show (item);
    gtk_menu_attach (GTK_MENU (menu), item, 0, 1, pos, pos + 1); ++pos;

    item = donna_image_menu_item_new_with_label (
            _c("systray-menu", "Quit"));
    image = gtk_image_new_from_icon_name ("application-exit", GTK_ICON_SIZE_MENU);
    donna_image_menu_item_set_image (DONNA_IMAGE_MENU_ITEM (item), image);
    gtk_widget_set_sensitive (item, !kalpm_state.is_busy);
    gtk_widget_set_tooltip_text (item, _("Exit kalu"));
    g_signal_connect (G_OBJECT (item), "activate",
            G_CALLBACK (menu_quit_cb), NULL);
    gtk_widget_show (item);
    gtk_menu_attach (GTK_MENU (menu), item, 0, 1, pos, pos + 1); ++pos;

    /* since we don't pack the menu anywhere, we need to "take ownership" of it,
     * and we'll destroy it when done, i.e. when it's unmapped */
    g_object_ref_sink (menu);
    gtk_widget_add_events (menu, GDK_STRUCTURE_MASK);
    g_signal_connect (G_OBJECT (menu), "unmap-event",
            G_CALLBACK (menu_unmap_cb), NULL);

    gtk_menu_popup (GTK_MENU (menu), NULL, NULL, NULL, NULL, button, activate_time);
}

#ifdef ENABLE_STATUS_NOTIFIER
void
sn_context_menu_cb (StatusNotifier *_sn _UNUSED_, gint x _UNUSED_, gint y _UNUSED_,
               gpointer data _UNUSED_)
{
    icon_popup_cb (NULL, 1, GDK_CURRENT_TIME, NULL);
}
#endif

void
process_fifo_command (const gchar *command)
{
    if (streq (command, "run-checks") || streq (command, "manual-checks"))
        kalu_check (FALSE);
    else if (streq (command, "auto-checks"))
        kalu_check (TRUE);
    else if (streq (command, "show-last-notifs"))
        show_last_notifs ();
    else if (streq (command, "toggle-pause"))
        set_pause (!kalpm_state.is_paused);
    else if (streq (command, "sysupgrade"))
        kalu_sysupgrade ();
    else if (streq (command, "show-unread-news"))
        menu_news_cb (NULL, GINT_TO_POINTER (1));
    else if (streq (command, "show-recent-news"))
        menu_news_cb (NULL, NULL);
    else if (streq (command, "popup-menu"))
        icon_popup_cb (NULL, 1, GDK_CURRENT_TIME, NULL);
#ifndef DISABLE_UPDATER
    else if (streq (command, "run-simulation"))
        run_simulation (NULL);
#endif
    else
        show_error (_("kalu received an unknown FIFO command"), command, NULL);
}

GPtrArray *open_windows = NULL;
static gboolean has_hidden_windows = FALSE;

void
add_open_window (gpointer window)
{
    if (!open_windows)
    {
        open_windows = g_ptr_array_new ();
    }
    g_ptr_array_add (open_windows, window);
}

void
remove_open_window (gpointer window)
{
    g_ptr_array_remove (open_windows, window);
}

static inline void
toggle_open_windows (void)
{
    if (!open_windows || open_windows->len == 0)
    {
        return;
    }

    g_ptr_array_foreach (open_windows,
            (GFunc) ((has_hidden_windows) ? gtk_widget_show : gtk_widget_hide),
            NULL);
    has_hidden_windows = !has_hidden_windows;
}

static guint icon_press_timeout = 0;

static void
process_click_action (on_click_t on_click)
{
    switch (on_click)
    {
        case DO_SYSUPGRADE:
            kalu_sysupgrade ();
            break;
        case DO_CHECK:
            kalu_check (FALSE);
            break;
#ifndef DISABLE_UPDATER
        case DO_SIMULATION:
            run_simulation (NULL);
            break;
#endif
        case DO_TOGGLE_WINDOWS:
            toggle_open_windows ();
            break;
        case DO_LAST_NOTIFS:
            show_last_notifs ();
            break;
        case DO_TOGGLE_PAUSE:
            set_pause (!kalpm_state.is_paused);
            break;
        case DO_EXIT:
            menu_quit_cb (NULL, NULL);
            break;
        case DO_NOTHING:
        case DO_SAME_AS_ACTIVE: /* not possible, silence warning */
            break;
    }
}

static gboolean
icon_press_click (gpointer data _UNUSED_)
{
    icon_press_timeout = 0;

    if (g_source_is_destroyed (g_main_current_source ()))
        return G_SOURCE_REMOVE;

    process_click_action ((kalpm_state.is_paused
            && config->on_sgl_click_paused != DO_SAME_AS_ACTIVE)
        ? config->on_sgl_click_paused
        : config->on_sgl_click);

    return G_SOURCE_REMOVE;
}

#ifdef ENABLE_STATUS_NOTIFIER
void sn_cb (gpointer data)
{
    guint t = GPOINTER_TO_UINT (data);

    if (t == SN_ACTIVATE)
        icon_press_click (NULL);
    else /* SN_SECONDARY_ACTIVATE */
        process_click_action ((kalpm_state.is_paused
                    && config->on_mdl_click_paused != DO_SAME_AS_ACTIVE)
                ? config->on_mdl_click_paused
                : config->on_mdl_click);
}
#endif

gboolean
icon_press_cb (GtkStatusIcon *icon _UNUSED_, GdkEventButton *event, gpointer data _UNUSED_)
{
    /* left button? */
    if (event->button == 1)
    {
        if (event->type == GDK_2BUTTON_PRESS)
        {
            /* we probably had a timeout set for the click, remove it */
            if (icon_press_timeout > 0)
            {
                g_source_remove (icon_press_timeout);
                icon_press_timeout = 0;
            }

            process_click_action ((kalpm_state.is_paused
                        && config->on_dbl_click_paused != DO_SAME_AS_ACTIVE)
                    ? config->on_dbl_click_paused
                    : config->on_dbl_click);
            return TRUE;
        }
        else if (event->type == GDK_BUTTON_PRESS)
        {
            /* As per GTK manual: on a dbl-click, we get GDK_BUTTON_PRESS twice
             * and then a GDK_2BUTTON_PRESS. Obviously, we want then to ignore
             * the two GDK_BUTTON_PRESS.
             * Also per manual, for a double click to occur, the second button
             * press must occur within 1/4 of a second of the first; so:
             * - on GDK_BUTTON_PRESS we set a timeout, in 250ms
             *  - if it expires/gets triggered, it was a click
             *  - if another click happens within that time, the timeout will be
             *    removed (see GDK_2BUTTON_PRESS above) and the clicks ignored
             * - if a GDK_BUTTON_PRESS occurs while a timeout is set, it's a
             * second click and ca be ignored, GDK_2BUTTON_PRESS will handle it */
            if (icon_press_timeout == 0)
            {
                guint delay;

                g_object_get (gtk_settings_get_default (),
                        "gtk-double-click-time",    &delay,
                        NULL);
                icon_press_timeout = g_timeout_add (delay, icon_press_click, NULL);
            }
            return TRUE;
        }
    }
    /* middle button? */
    else if (event->button == 2 && event->type == GDK_BUTTON_PRESS)
    {
        process_click_action ((kalpm_state.is_paused
                    && config->on_mdl_click_paused != DO_SAME_AS_ACTIVE)
                ? config->on_mdl_click_paused
                : config->on_mdl_click);
        return TRUE;
    }

    return FALSE;
}

#ifdef ENABLE_STATUS_NOTIFIER
StatusNotifier *sn = NULL;
GdkPixbuf *sn_icon[NB_SN_ICONS] = { NULL, };
#endif
GtkStatusIcon *icon = NULL;


#define addstr(...)     do {                        \
    len = snprintf (s, (size_t) *max, __VA_ARGS__); \
    *max -= len;                                    \
    s += len;                                       \
} while (0)
enum {
    TT_FULL,
    TT_TITLE,
    TT_BODY
};
static void make_tooltip (gchar *s, gint *max, guint tt)
{
    gint len;

    if (tt == TT_FULL || tt == TT_TITLE)
    {
        addstr ("[%s%s] ",
                /* TRANSLATORS: This goes in kalu's systray tooltip. It usually
                 * just says "kalu" but will show this when paused, indicating that
                 * kalu is indeed in paused mode. */
                (kalpm_state.is_paused) ? _("paused kalu") : "kalu",
                (has_hidden_windows) ? " +" : "");

        if (kalpm_state.is_busy)
        {
            addstr (_("Checking/updating in progress..."));
            return;
        }
        else if (kalpm_state.last_check == NULL)
        {
            return;
        }
    }

    if (tt == TT_TITLE)
    {
        gchar *s_date;

        s_date = g_date_time_format (kalpm_state.last_check, "%F %R");
        addstr (_("Last checked on %s"), s_date);
        g_free (s_date);
        return;
    }
    else if (tt == TT_FULL)
    {
        GDateTime *current;
        GTimeSpan timespan;

        current = g_date_time_new_now_local ();
        timespan = g_date_time_difference (current, kalpm_state.last_check);
        g_date_time_unref (current);

        if (timespan < G_TIME_SPAN_MINUTE)
        {
            addstr (_("Last checked just now"));
        }
        else
        {
            gint days = 0, hours = 0, minutes = 0;

            if (timespan >= G_TIME_SPAN_DAY)
            {
                days = (gint) (timespan / G_TIME_SPAN_DAY);
                timespan -= (days * G_TIME_SPAN_DAY);
            }
            if (timespan >= G_TIME_SPAN_HOUR)
            {
                hours = (gint) (timespan / G_TIME_SPAN_HOUR);
                timespan -= (hours * G_TIME_SPAN_HOUR);
            }
            if (timespan >= G_TIME_SPAN_MINUTE)
            {
                minutes = (gint) (timespan / G_TIME_SPAN_MINUTE);
                timespan -= (minutes * G_TIME_SPAN_MINUTE);
            }

            /* TRANSLATORS: Constructing e.g. "Last checked 1 hour 23 minutes ago"
             * by adding a string, the days/hours/minutes if apply, and a last
             * string. (Make sure to end this first, and the days/hours/minutes
             * strings with a space) */
            addstr (_("Last checked "));
            if (days > 0)
                addstr (_n("1 day ", "%d days ", (long unsigned int) days),
                        days);
            if (hours > 0)
                addstr (_n("1 hour ", "%d hours ", (long unsigned int) hours),
                        hours);
            if (minutes > 0)
                addstr (_n("1 minute ", "%d minutes ", (long unsigned int) minutes),
                        minutes);
            /* TRANSLATORS: Ending the "Last checked 42 days ago" sentence. Use " "
             * if it doesn't apply. */
            addstr (_("ago"));
        }
    }

    if (config->syncdbs_in_tooltip)
    {
        long unsigned int nb_syncdbs = 0;

        if (kalpm_state.synced_dbs && kalpm_state.synced_dbs->len > 0)
        {
            gsize i;

            for (i = 0; i < kalpm_state.synced_dbs->len; ++i)
                if (kalpm_state.synced_dbs->str[i] == '\0')
                    ++nb_syncdbs;
        }
        if (nb_syncdbs > 0)
        {
            addstr (_n( "\nsync possible for 1 db",
                        "\nsync possible for %d dbs",
                        nb_syncdbs),
                    (int) nb_syncdbs);
        }
    }

    if (kalpm_state.nb_news > 0)
    {
        addstr (_n( "\n1 unread news",
                    "\n%d unread news",
                    (long unsigned int) kalpm_state.nb_news),
                kalpm_state.nb_news);
    }
    if (kalpm_state.nb_upgrades > 0)
    {
        addstr (_n( "\n1 upgrade available",
                    "\n%d upgrades available",
                    (long unsigned int) kalpm_state.nb_upgrades),
                kalpm_state.nb_upgrades);
    }
    else if (kalpm_state.nb_upgrades == UPGRADES_NB_CONFLICT)
    {
        addstr (_("\nupgrades available (unknown number due to conflict)"));
    }
    if (kalpm_state.nb_watched > 0)
    {
        addstr (_n( "\n1 watched package updated",
                    "\n%d watched packages updated",
                    (long unsigned int) kalpm_state.nb_watched),
                kalpm_state.nb_watched);
    }
    if (kalpm_state.nb_aur > 0)
    {
        addstr (_n( "\n1 AUR package updated",
                    "\n%d AUR packages updated",
                    (long unsigned int) kalpm_state.nb_aur),
                kalpm_state.nb_aur);
    }
    if (kalpm_state.nb_aur_not_found > 0)
    {
        addstr (_n( "\n1 package not found in AUR",
                    "\n%d packages not found in AUR",
                    (long unsigned int) kalpm_state.nb_aur_not_found),
                kalpm_state.nb_aur_not_found);
    }
    if (kalpm_state.nb_watched_aur > 0)
    {
        addstr (_n( "\n1 watched AUR package updated",
                    "\n%d watched AUR packages updated",
                    (long unsigned int) kalpm_state.nb_watched_aur),
                kalpm_state.nb_watched_aur);
    }
}
#undef addstr

gboolean
icon_query_tooltip_cb (GtkWidget *_icon _UNUSED_, gint x _UNUSED_, gint y _UNUSED_,
                       gboolean keyboard_mode _UNUSED_, GtkTooltip *tooltip,
                       gpointer data _UNUSED_)
{
    gchar buf[512];
    gint max = 512;

    make_tooltip (buf, &max, TT_FULL);
    if (max <= 0)
    {
        sprintf (buf, _("kalu: error setting tooltip"));
    }
    gtk_tooltip_set_text (tooltip, buf);
    return TRUE;
}

static gboolean
set_status_icon (gboolean active)
{
    if (active)
    {
#ifdef ENABLE_STATUS_NOTIFIER
        if (sn)
        {
            guint i;
            const gchar *s;

            s = (kalpm_state.is_paused) ? "kalu-paused" : "kalu";
            i = (kalpm_state.is_paused) ? SN_ICON_KALU_PAUSED : SN_ICON_KALU;
            if (sn_icon[i])
                g_object_set (G_OBJECT (sn),
                        "main-icon-pixbuf",     sn_icon[i],
                        "tooltip-icon-pixbuf",  sn_icon[i],
                        NULL);
            else
                g_object_set (G_OBJECT (sn),
                        "main-icon-name",       s,
                        "tooltip-icon-name",    s,
                        NULL);
        }

        /* in case both are set (i.e. sn is e.g. waiting for a host, while we
         * use icon as fallback */
        if (icon)
#endif
        G_GNUC_BEGIN_IGNORE_DEPRECATIONS
        gtk_status_icon_set_from_icon_name (icon, (kalpm_state.is_paused)
                ? "kalu-paused"
                : "kalu");
        G_GNUC_END_IGNORE_DEPRECATIONS
    }
    else
    {
#ifdef ENABLE_STATUS_NOTIFIER
        if (sn)
        {
            guint i;
            const gchar *s;

            s = (kalpm_state.is_paused) ? "kalu-gray-paused" : "kalu-gray";
            i = (kalpm_state.is_paused) ? SN_ICON_KALU_GRAY_PAUSED : SN_ICON_KALU_GRAY;
            if (sn_icon[i])
                g_object_set (G_OBJECT (sn),
                        "main-icon-pixbuf",     sn_icon[i],
                        "tooltip-icon-pixbuf",  sn_icon[i],
                        NULL);
            else
                g_object_set (G_OBJECT (sn),
                        "main-icon-name",       s,
                        "tooltip-icon-name",    s,
                        NULL);
        }

        /* in case both are set (i.e. sn is e.g. waiting for a host, while we
         * use icon as fallback */
        if (icon)
#endif
        G_GNUC_BEGIN_IGNORE_DEPRECATIONS
        gtk_status_icon_set_from_icon_name (icon, (kalpm_state.is_paused)
                ? "kalu-gray-paused"
                : "kalu-gray");
        G_GNUC_END_IGNORE_DEPRECATIONS
    }
    /* do NOT get called back */
    return FALSE;
}

#ifdef ENABLE_STATUS_NOTIFIER
static void sn_upd_status (gboolean active)
{
    if (sn)
        status_notifier_set_status (sn,
                (active) ? STATUS_NOTIFIER_STATUS_ACTIVE : STATUS_NOTIFIER_STATUS_PASSIVE);
}
#endif

void
set_kalpm_nb (check_t type, gint nb, gboolean do_update_icon)
{
    if (type & CHECK_UPGRADES)
    {
        kalpm_state.nb_upgrades = nb;
    }

    if (type & CHECK_WATCHED)
    {
        kalpm_state.nb_watched = nb;
    }

    if (type & CHECK_AUR)
    {
        kalpm_state.nb_aur = nb;
    }

    if (type & _CHECK_AUR_NOT_FOUND)
    {
        kalpm_state.nb_aur_not_found = nb;
    }

    if (type & CHECK_WATCHED_AUR)
    {
        kalpm_state.nb_watched_aur = nb;
    }

    if (type & CHECK_NEWS)
    {
        kalpm_state.nb_news = nb;
    }

    if (do_update_icon)
    {
        update_icon ();
    }
}

inline GString **
get_kalpm_synced_dbs (void)
{
    return &kalpm_state.synced_dbs;
}

void
reset_kalpm_synced_dbs (void)
{
    if (kalpm_state.synced_dbs)
        kalpm_state.synced_dbs->len = 0;
}

static gboolean
switch_status_icon (void)
{
    static gboolean active = FALSE;
    active = !active;
    set_status_icon (active);
    /* keep timeout alive */
    return TRUE;
}

#ifdef ENABLE_STATUS_NOTIFIER
static void sn_refresh_tooltip (void)
{
    gchar buf[512];
    gint max = 512;

    if (!sn)
        return;

    status_notifier_freeze_tooltip (sn);
    make_tooltip (buf, &max, TT_TITLE);
    if (max > 0)
    {
        status_notifier_set_tooltip_title (sn, buf);
    }
    max = 512;
    /* in case there's nothing, else we would set the title as body */
    buf[0] = '\0';
    make_tooltip (buf, &max, TT_BODY);
    if (max > 0)
    {
        status_notifier_set_tooltip_body (sn, buf);
    }
    status_notifier_thaw_tooltip (sn);
}
#endif

void
set_kalpm_busy (gboolean busy)
{
    gint old = kalpm_state.is_busy;

    /* we use an counter because when using a cmdline for both upgrades & AUR,
     * and both are triggered at the same time (from notifications) then we
     * should only bo back to not busy when *both* are done; fixes #8 */
    if (busy)
    {
        ++kalpm_state.is_busy;
    }
    else if (kalpm_state.is_busy > 0)
    {
        --kalpm_state.is_busy;
    }

    /* make sure the state changed/there's something to do */
    if ((old > 0  && kalpm_state.is_busy > 0)
            || (old == 0 && kalpm_state.is_busy == 0))
    {
        return;
    }

    if (busy)
    {
        /* remove auto-check timeout */
        if (kalpm_state.timeout > 0)
        {
            g_source_remove (kalpm_state.timeout);
            kalpm_state.timeout = 0;
            debug ("state busy: disable next auto-checks");
        }

        /* set timeout for status icon */
        kalpm_state.timeout_icon = g_timeout_add (420,
                (GSourceFunc) switch_status_icon, NULL);
        debug ("state busy: switch icons");
    }
    else
    {
        /* remove status icon timeout */
        if (kalpm_state.timeout_icon > 0)
        {
            g_source_remove (kalpm_state.timeout_icon);
            kalpm_state.timeout_icon = 0;
            /* ensure icon is right */
            set_kalpm_nb (0, 0, TRUE);
            debug ("state non-busy: disable switch icons");
        }

        if (!kalpm_state.is_paused && config->interval > 0)
        {
            /* set timeout for next auto-check */
            guint seconds;

            seconds = (guint) config->interval;
            kalpm_state.timeout = rt_timeout_add_seconds (seconds,
                    (GSourceFunc) kalu_auto_check, NULL);
            debug ("state non-busy: next auto-checks in %d seconds", seconds);
        }
    }

#ifdef ENABLE_STATUS_NOTIFIER
    sn_refresh_tooltip ();
#endif
}

void
reset_timeout (void)
{
    guint seconds;

    if (kalpm_state.is_busy > 0 || kalpm_state.is_paused)
    {
        debug ("reset timeout: noting to do");
        return;
    }

    /* remove auto-check timeout */
    if (kalpm_state.timeout > 0)
    {
        g_source_remove (kalpm_state.timeout);
        kalpm_state.timeout = 0;
        debug ("reset timeout: disable next auto-checks");
    }

    if (config->interval > 0)
    {
        /* set timeout for next auto-check */
        seconds = (guint) config->interval;
        kalpm_state.timeout = rt_timeout_add_seconds (seconds,
                (GSourceFunc) kalu_auto_check, NULL);
        debug ("reset timeout: next auto-checks in %d seconds", seconds);
    }
}

static inline gboolean
is_within_skip (void)
{
    if (!config->has_skip)
    {
        return FALSE;
    }

    GDateTime *now, *begin, *end;
    gint year, month, day;
    gboolean within_skip = FALSE;

    now = g_date_time_new_now_local ();
    /* create GDateTime for begin & end of skip period */
    /* Note: begin & end are both for the current day, which means we can
     * have begin > end, with e.g. 18:00-09:00 */
    g_date_time_get_ymd (now, &year, &month, &day);
    begin = g_date_time_new_local (year, month, day,
            config->skip_begin_hour, config->skip_begin_minute, 0);
    end = g_date_time_new_local (year, month, day,
            config->skip_end_hour, config->skip_end_minute, 0);

    /* determine if we're within skip period */
    /* is begin > end ? e.g. 18:00 -> 09:00 */
    if (g_date_time_compare (begin, end) == 1)
    {
        /* we're within if before end OR after begin */
        within_skip = (g_date_time_compare (now, end) == -1
                || g_date_time_compare (now, begin) == 1);
    }
    /* e.g. 09:00 -> 18:00 */
    else
    {
        /* we're within if after begin AND before end */
        within_skip = (g_date_time_compare (now, begin) == 1
                && g_date_time_compare (now, end) == -1);
    }

    g_date_time_unref (now);
    g_date_time_unref (begin);
    g_date_time_unref (end);

    return within_skip;
}

gboolean
skip_next_timeout (gpointer no_checks)
{
    gboolean force = FALSE;

    /* remove current timeout */
    if (kalpm_state.timeout_skip > 0)
    {
        g_source_remove (kalpm_state.timeout_skip);
        kalpm_state.timeout_skip = 0;
    }

    /* first time we're called, determine where we're at */
    if (kalpm_state.skip_next == SKIP_UNKNOWN)
    {
        if (!config->has_skip)
        {
            debug ("skip period: none set");
            if (!no_checks)
            {
                /* we should still trigger auto-checks */
                kalu_check (TRUE);
            }
            return G_SOURCE_REMOVE;
        }
        kalpm_state.skip_next = (is_within_skip ()) ? SKIP_BEGIN : SKIP_END;
        force = TRUE;
    }

    /* toggle state */
    gboolean paused = (kalpm_state.skip_next == SKIP_BEGIN);
    debug ("skip period: auto-%s", (paused) ? "pausing" : "resuming");
    if (!kalpm_state.is_busy && !no_checks)
    {
        if (force)
        {
            /* means the state was unknown, so we should force the autochecks
             * and whatnot. this is done by making it believe we're changing the
             * state */
            kalpm_state.is_paused = !paused;
        }
        set_pause (paused);
    }
    else
    {
        /* since we're busy, we can't toggle right now. instead, we'll
         * update the flag, so it'll be takin into accound right away */
        kalpm_state.is_paused = paused;
        /* We might not be busy, but no_checks is set, in which case we do the
         * same as to not trigger the checks (via set_pause()). no_checks is
         * only set when called on app start and auto-checks are disabled
         * (config->interval == 0) so nothing should happen, and we only want to
         * set up the skip period.
         * However, we then need to update the icon (if paused) */
        if (no_checks && paused)
        {
            update_icon ();
        }
    }

    /* set new timeout_skip */
    GDateTime *now, *next;
    gint year, month, day, h;
    gint hour, minute;

    if (paused)
    {
        kalpm_state.skip_next = SKIP_END;
        hour = config->skip_end_hour;
        minute = config->skip_end_minute;
    }
    else
    {
        kalpm_state.skip_next = SKIP_BEGIN;
        hour = config->skip_begin_hour;
        minute = config->skip_begin_minute;
    }

    now = g_date_time_new_now_local ();
    g_date_time_get_ymd (now, &year, &month, &day);
    /* if the timeout is for a time before now, it gets bumped to tomorrow */
    h = g_date_time_get_hour (now);
    if (h > hour || (h == hour && g_date_time_get_minute (now) > minute))
    {
        ++day;
    }
    next = g_date_time_new_local (year, month, day, hour, minute, 0);

    GTimeSpan timespan;
    guint seconds;

    timespan = g_date_time_difference (next, now);
    seconds = (guint) (timespan / G_TIME_SPAN_SECOND);
    kalpm_state.timeout_skip = rt_timeout_add_seconds (seconds,
            (GSourceFunc) skip_next_timeout, NULL);
    debug ("next skip period in %d seconds", seconds);

    g_date_time_unref (now);
    g_date_time_unref (next);
    return G_SOURCE_REMOVE;
}

gboolean
reload_watched (gboolean is_aur, GError **error)
{
    GError *local_err = NULL;
    gchar file[PATH_MAX];
    conf_file_t conffile;

    if (is_aur)
    {
        /* clear */
        FREE_WATCHED_PACKAGE_LIST (config->watched_aur);
        /* load */
        snprintf (file, PATH_MAX - 1, "%s/kalu/watched-aur.conf",
                g_get_user_config_dir ());
        conffile = CONF_FILE_WATCHED_AUR;
    }
    else
    {
        /* clear */
        FREE_WATCHED_PACKAGE_LIST (config->watched);
        /* load */
        snprintf (file, PATH_MAX - 1, "%s/kalu/watched.conf",
                g_get_user_config_dir ());
        conffile = CONF_FILE_WATCHED;
    }

    if (!parse_config_file (file, conffile, &local_err))
    {
        g_propagate_error (error, local_err);
        return FALSE;
    }

    return TRUE;
}
