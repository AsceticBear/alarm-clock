/*
 * alarm-gconf.c -- GConf routines
 * 
 * Copyright (C) 2007-2008 Johannes H. Jensen <joh@pseudoberries.com>
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 * 
 * Authors:
 * 		Johannes H. Jensen <joh@pseudoberries.com>
 */

#include <time.h>

#include "alarm-applet.h"
#include "alarm-gconf.h"
#include "alarm-settings.h"
#include "alarm.h"

/*
 * LabelType enum to GConf string value map
 */
GConfEnumStringPair label_type_enum_map [] = {
	{ LABEL_TYPE_TIME,		"alarm-time"  },
	{ LABEL_TYPE_REMAIN,	"remaining-time"  },
	{ 0, NULL }
};

/*
 * GCONF CALLBACKS {{
 */
/*
void
alarm_applet_gconf_show_label_changed (GConfClient  *client,
									   guint         cnxn_id,
									   GConfEntry   *entry,
									   AlarmApplet  *applet)
{
	g_debug ("show_label_changed");
	
	if (!entry->value || entry->value->type != GCONF_VALUE_BOOL)
		return;
	
	applet->show_label = gconf_value_get_bool (entry->value);
	
	g_object_set (applet->label, "visible", applet->show_label, NULL);
	
	if (applet->preferences_dialog != NULL) {
		pref_update_label_show (applet);
	}
}

void
alarm_applet_gconf_label_type_changed (GConfClient  *client,
									  guint         cnxn_id,
									  GConfEntry   *entry,
									  AlarmApplet  *applet)
{
	g_debug ("label_type_changed");
	
	const gchar *tmp;
	
	if (!entry->value || entry->value->type != GCONF_VALUE_STRING)
		return;
	
	tmp = gconf_value_get_string (entry->value);
	if (tmp) {
		if (!gconf_string_to_enum (label_type_enum_map, tmp, (gint *)&(applet->label_type))) {
			// No match, set to default
			applet->label_type = DEF_LABEL_TYPE;
		}
		
		alarm_applet_label_update (applet);
	}
	
	if (applet->preferences_dialog != NULL) {
		pref_update_label_type (applet);
	}
}
*/

/**
 * Triggered on global changes to our gconf preference dir.
 * We do this because we're interested in the events where
 * an alarm directory is either added or deleted externally.
 * 
 * When this happens we update our list of alarms.
 */
static void
alarm_applet_gconf_global_change (GConfClient  *client,
								  guint         cnxn_id,
								  GConfEntry   *entry,
								  AlarmApplet  *applet)
{
	Alarm *a;
	GString *str;
	GList *l;
	gchar *dir;
	gint id, i, len;
	gboolean found = FALSE;
	
	g_debug ("GLOBAL_change: %s", entry->key);
	
	/*
	 * We're only interested in the first part of the key matching
	 * {applet_gconf_pref_dir}/{something}
	 * 
	 * Here we extract {something}
	 */
	dir = ALARM_GCONF_DIR;
	len = strlen (entry->key);
	str = g_string_new ("");
	
	for (i = strlen(dir) + 1; i < len; i++) {
		if (entry->key[i] == '/')
			break;
		
		str = g_string_append_c (str, entry->key[i]);
	}
	
	//g_debug ("\tEXTRACTED: %s", str->str);
	
	// Check if the key is valid
	id = alarm_gconf_dir_get_id (str->str);
	
	if (id >= 0) {
		// Valid, probably an alarm which has been removed
		g_debug ("GLOBAL change ON alarm #%d", id);
        
        // Check if the alarm exists in our alarms list
		for (l = applet->alarms; l != NULL; l = l->next) {
			a = ALARM (l->data);
			if (a->id == id) {
				// FOUND
				found = TRUE;
				break;
			}
		}
		
		if (found && entry->value == NULL) {
			// DELETED ALARM
			g_debug ("\tDELETE alarm #%d %p", id, a);
			
			/* If there's a settings dialog open for this
			 * alarm, close it.
			 */
            if (applet->settings_dialog->alarm == a) {
				alarm_settings_dialog_close (applet->settings_dialog);
			}
			
			/*
			 * Remove from list
			 */
			alarm_applet_alarms_remove (applet, a);
			
		} else if (!found && entry->value != NULL) {
			// ADDED ALARM
			/*
			 * Add to list
			 */
			a = alarm_new (ALARM_GCONF_DIR, id);
			
			g_debug ("\tADD alarm #%d %p", id, a);
			
			alarm_applet_alarms_add (applet, a);
            
		} else if (found) {
            //alarm_list_window_alarm_update (applet->list_window, a);
        }
	}
	
	g_string_free (str, TRUE);
}


/*
 * }} GCONF CALLBACKS
 */

/*
 * Init
 */
void
alarm_applet_gconf_init (AlarmApplet *applet)
{
	GConfClient *client;

	client = gconf_client_get_default ();

    
	
	/*key = panel_applet_gconf_get_full_key (PANEL_APPLET (applet->parent), KEY_SHOW_LABEL);
	applet->listeners [0] =
		gconf_client_notify_add (
				client, key,
				(GConfClientNotifyFunc) alarm_applet_gconf_show_label_changed,
				applet, NULL, NULL);
	g_free (key);
	
	key = panel_applet_gconf_get_full_key (PANEL_APPLET (applet->parent), KEY_LABEL_TYPE);
	applet->listeners [1] =
		gconf_client_notify_add (
				client, key,
				(GConfClientNotifyFunc) alarm_applet_gconf_label_type_changed,
				applet, NULL, NULL);
	g_free (key);
	*/
	/*
	 * Listen for changes to the alarms.
	 * We want to know when an alarm is added and removed.
	 */
	applet->listeners [2] =
		gconf_client_notify_add (
				client, ALARM_GCONF_DIR,
				(GConfClientNotifyFunc) alarm_applet_gconf_global_change,
				applet, NULL, NULL);
	
}

/* Load gconf values into applet.
 * We are very paranoid about gconf here. 
 * We can't rely on the schemas to exist, and so if we don't get any
 * defaults from gconf, we set them manually.
 * Not only that, but if some error occurs while setting the
 * defaults in gconf, we already have them copied locally. */
void
alarm_applet_gconf_load (AlarmApplet *applet)
{
	/*GConfClient *client;
	
	client = gconf_client_get_default ();*/
}
