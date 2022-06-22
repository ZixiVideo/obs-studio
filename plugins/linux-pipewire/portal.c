/* portal.c
 *
 * Copyright 2021 Georges Basile Stavracas Neto <georges.stavracas@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "portal.h"
#include "pipewire.h"

#include <util/dstr.h>

#define REQUEST_PATH "/org/freedesktop/portal/desktop/request/%s/obs%u"
#define SESSION_PATH "/org/freedesktop/portal/desktop/session/%s/obs%u"

static GDBusConnection *connection = NULL;
static GDBusProxy *proxy = NULL;

static void ensure_proxy(void)
{
	g_autoptr(GError) error = NULL;
	if (!connection) {
		connection = g_bus_get_sync(G_BUS_TYPE_SESSION, NULL, &error);

		if (error) {
			blog(LOG_WARNING,
			     "[portals] Error retrieving D-Bus connection: %s",
			     error->message);
			return;
		}
	}

	if (!proxy) {
		proxy = g_dbus_proxy_new_sync(
			connection, G_DBUS_PROXY_FLAGS_NONE, NULL,
			"org.freedesktop.portal.Desktop",
			"/org/freedesktop/portal/desktop",
			"org.freedesktop.portal.ScreenCast", NULL, &error);

		if (error) {
			blog(LOG_WARNING,
			     "[portals] Error retrieving D-Bus proxy: %s",
			     error->message);
			return;
		}
	}
}

char *get_sender_name(void)
{
	char *sender_name;
	char *aux;

	ensure_proxy();

	sender_name =
		bstrdup(g_dbus_connection_get_unique_name(connection) + 1);

	/* Replace dots by underscores */
	while ((aux = strstr(sender_name, ".")) != NULL)
		*aux = '_';

	return sender_name;
}

uint32_t portal_get_available_capture_types(void)
{
	g_autoptr(GVariant) cached_source_types = NULL;
	uint32_t available_source_types;

	ensure_proxy();

	if (!proxy)
		return 0;

	cached_source_types =
		g_dbus_proxy_get_cached_property(proxy, "AvailableSourceTypes");
	available_source_types =
		cached_source_types ? g_variant_get_uint32(cached_source_types)
				    : 0;

	return available_source_types;
}

uint32_t portal_get_available_cursor_modes(void)
{
	g_autoptr(GVariant) cached_cursor_modes = NULL;
	uint32_t available_cursor_modes;

	ensure_proxy();

	if (!proxy)
		return 0;

	cached_cursor_modes =
		g_dbus_proxy_get_cached_property(proxy, "AvailableCursorModes");
	available_cursor_modes =
		cached_cursor_modes ? g_variant_get_uint32(cached_cursor_modes)
				    : 0;

	return available_cursor_modes;
}

uint32_t portal_get_screencast_version(void)
{
	g_autoptr(GVariant) cached_version = NULL;
	uint32_t version;

	ensure_proxy();

	if (!proxy)
		return 0;

	cached_version = g_dbus_proxy_get_cached_property(proxy, "version");
	version = cached_version ? g_variant_get_uint32(cached_version) : 0;

	return version;
}

GDBusConnection *portal_get_dbus_connection(void)
{
	ensure_proxy();
	return connection;
}

GDBusProxy *portal_get_dbus_proxy(void)
{
	ensure_proxy();
	return proxy;
}

void portal_create_request_path(char **out_path, char **out_token)
{
	static uint32_t request_token_count = 0;

	request_token_count++;

	if (out_token) {
		struct dstr str;
		dstr_init(&str);
		dstr_printf(&str, "obs%u", request_token_count);
		*out_token = str.array;
	}

	if (out_path) {
		char *sender_name;
		struct dstr str;

		sender_name = get_sender_name();

		dstr_init(&str);
		dstr_printf(&str, REQUEST_PATH, sender_name,
			    request_token_count);
		*out_path = str.array;

		bfree(sender_name);
	}
}

void portal_create_session_path(char **out_path, char **out_token)
{
	static uint32_t session_token_count = 0;

	session_token_count++;

	if (out_token) {
		struct dstr str;
		dstr_init(&str);
		dstr_printf(&str, "obs%u", session_token_count);
		*out_token = str.array;
	}

	if (out_path) {
		char *sender_name;
		struct dstr str;

		sender_name = get_sender_name();

		dstr_init(&str);
		dstr_printf(&str, SESSION_PATH, sender_name,
			    session_token_count);
		*out_path = str.array;

		bfree(sender_name);
	}
}
