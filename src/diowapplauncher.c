/* diowapplauncher */
/* Quick search for apps and launch */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "runcmd.h"
#include <assert.h>
#include <stdbool.h>
#include "externvars.h"
#include "configsgen.c"
#include "create-shm.h"
#include "filter-list.h"
#include <cairo/cairo.h>
#include "check-if-directory.c"
#include <xkbcommon/xkbcommon.h>
#include <wayland-client-protocol.h>
#include <librsvg-2.0/librsvg/rsvg.h>
#include "xdg-shell-client-protocol.h"
#include "process-directory-desktop-files.h"

/* Global variables */
char *tempTextBuff = NULL;
char *filteredList[2048];
char *filteredListIcons[2048];
static bool lastItemReached = false;

/* Main state struct */
struct client_state {
	/* Globals */
	struct wl_shm *wl_shm;
	struct wl_seat *wl_seat;
	struct wl_display *wl_display;
    struct xdg_wm_base *xdg_wm_base;
	struct wl_registry *wl_registry;
	struct wl_compositor *wl_compositor;
	/* Objects */
	struct wl_surface *wl_surface;
	struct wl_keyboard *wl_keyboard;
    struct xdg_surface *xdg_surface;
    struct xdg_toplevel *xdg_toplevel;
	/* State */
	struct xkb_state *xkb_state;
	struct xkb_context *xkb_context;
	struct xkb_keymap *xkb_keymap;
	bool closed;
	int32_t width;
	int32_t height;
	uint32_t xkb_group;
	uint32_t xdg_serial;
};

/* dummy function */
static void noop() {}

/* Dummy function */
static struct wl_buffer *draw_frame(struct client_state *state);

/* Keyboard Managing */
static void wl_keyboard_keymap(void *data, struct wl_keyboard *wl_keyboard, uint32_t format,
																	int32_t fd, uint32_t size) {
	(void)wl_keyboard;
	struct client_state *state = data;
	assert(format == WL_KEYBOARD_KEYMAP_FORMAT_XKB_V1);
	char *map_shm = mmap(NULL, size, PROT_READ, MAP_SHARED, fd, 0);
	assert(map_shm != MAP_FAILED);
	struct xkb_keymap *xkb_keymap;
	xkb_keymap = xkb_keymap_new_from_string(state->xkb_context, map_shm,
										XKB_KEYMAP_FORMAT_TEXT_V1, XKB_KEYMAP_COMPILE_NO_FLAGS);
	munmap(map_shm, size);
	close(fd);

	struct xkb_state *xkb_state = xkb_state_new(xkb_keymap);
	xkb_keymap_unref(state->xkb_keymap);
	xkb_state_unref(state->xkb_state);
	state->xkb_keymap = xkb_keymap;
	state->xkb_state = xkb_state;
}

static void refresh_buffer(struct client_state *state) {
	struct wl_buffer *buffer = draw_frame(state);
	wl_surface_attach(state->wl_surface, buffer, 0, 0);
	wl_surface_damage_buffer(state->wl_surface, 0, 0, INT32_MAX, INT32_MAX);
	wl_surface_commit(state->wl_surface);
}

static void wl_keyboard_key(void *data, struct wl_keyboard *wl_keyboard, uint32_t serial,
													uint32_t time, uint32_t key, uint32_t statee) {
	(void)wl_keyboard;
	(void)serial;
	(void)time;
	struct client_state *state = data;
	char buf[128];
	uint32_t keycode = key + 8;
	const char *action = statee == WL_KEYBOARD_KEY_STATE_PRESSED ? "press" : "release";
	xkb_state_key_get_utf8(state->xkb_state, keycode, buf, sizeof(buf));
	// Do nothing on modifiers press
	xkb_keysym_t sym = xkb_state_key_get_one_sym(state->xkb_state, keycode);
	if (sym == XKB_KEY_Shift_L || sym == XKB_KEY_Shift_R || sym == XKB_KEY_Control_L || \
		sym == XKB_KEY_Control_R || sym == XKB_KEY_Alt_L || sym == XKB_KEY_Alt_R || \
		sym == XKB_KEY_Super_L || sym == XKB_KEY_Super_R || sym == XKB_KEY_Tab || \
		sym == XKB_KEY_Caps_Lock) {
		return;
	}
	// Action on key press
	if (statee == WL_KEYBOARD_KEY_STATE_PRESSED && strcmp(action, "press") == 0) {
		// Arrow down
		if (sym == XKB_KEY_Down && !lastItemReached) {
			int i = 0;
			for (; filteredList[i] != NULL && filteredList[2] != NULL; i++) {
				filteredList[i] = filteredList[i + 1];
			}
			filteredList[i - 1] = NULL;
			if (filteredList[0] != NULL) {
				snprintf(tempTextBuff, strlen(filteredList[0]) + 1, "%s", filteredList[0]);
			}
			// Adjust the size but only if there is enough items in the filtered list
			if (i <= 9) {
				state->height = state->height - 50;
			}
			if (i > 10) {
				state->height = 520;
			}
			if (i <= 2) {
				printf("reached\n");
				lastItemReached = true;
				if (filteredList[1] != NULL) {
					state->height = 120;
					filteredList[0] = filteredList[1];
					snprintf(tempTextBuff, strlen(filteredList[0]) + 1, "%s", filteredList[0]);
					filteredList[1] = NULL;
					refresh_buffer(state);
					return;
				}
				else {
					return;
				}
			}
			refresh_buffer(state);
			return;
		}
		else if (sym == XKB_KEY_Down && lastItemReached) {
			return;
		}
		// Backspace action
		if (sym == XKB_KEY_BackSpace) {
			if (strlen(tempTextBuff) > 1) { // If more than one character is typed
				lastItemReached = false;
				// FIxing the bug with multibyte chars
				if (tempTextBuff[0] < 0) {
					tempTextBuff[0] = '_';
					tempTextBuff[1] = '\0';
					refresh_buffer(state);
					return;
				}
				else {
					tempTextBuff[strlen(tempTextBuff) - 1] = '\0';
					filtered_list(names, filteredList, tempTextBuff);
					// Adjusting height of results dialog
					state->height = 70;
					for (int i = 0; filteredList[i] != NULL && state->height < 510; i++) {
						state->height = state->height + 50;
					}
					refresh_buffer(state);
					return;
				}
			}
			else {
				// FIxing the bug with multibyte chars
				if (tempTextBuff[0] < 0) {
					tempTextBuff[0] = '_';
					tempTextBuff[1] = '\0';
					refresh_buffer(state);
					return;
				}
				else {
					tempTextBuff[0] = '_'; // If there is only the default character
					filtered_list(names, filteredList, tempTextBuff);
					state->height = 70;
					refresh_buffer(state);
					return;
				}
			}
		}
		// On Enter press
		if (sym == XKB_KEY_Return) {
			// Get the name of the app
			printf("Selected app: %s\n", filteredList[0]);
			// Get the exec of the app
			int i = 0;
			for (i = 0; names[i] != NULL; i++) {
				if (strcmp(names[i], filteredList[0]) == 0) {
					break;
				}
			}
			printf("Executing: %s\n", execs[i]);
			run_cmd(execs[i]);
			state->closed = true;
			return;
		}
		// Other keys action
		else {
			if (strcmp(tempTextBuff, "_") == 0) { // First key press
				tempTextBuff[0] = '\0';
				strcat(tempTextBuff, buf);
				filtered_list(names, filteredList, tempTextBuff);
				// Adjusting height of results dialog
				state->height = 70;
				for (int i = 0; filteredList[i] != NULL && state->height < 510; i++) {
					state->height = state->height + 50;
				}
				refresh_buffer(state);
				return;
			}
			else {
				state->height = 70;
				strcat(tempTextBuff, buf); // Second and all the rest of key presses
				filtered_list(names, filteredList, tempTextBuff);
				// Adjusting height of results dialog
				for (int i = 0; filteredList[i] != NULL && state->height < 510; i++) {
					state->height = state->height + 50;
				}
				refresh_buffer(state);
				return;
			}
		}
	}
}

static void wl_keyboard_modifiers(void *data, struct wl_keyboard *wl_keyboard, 
			uint32_t serial, uint32_t mods_depressed, uint32_t mods_latched, uint32_t mods_locked,
																				uint32_t group) {
	(void)wl_keyboard;
	(void)serial;
	struct client_state *state = data;
	xkb_state_update_mask(state->xkb_state, mods_depressed, mods_latched,
																	mods_locked, 0, 0, group);
}

static const struct wl_keyboard_listener wl_keyboard_listener = {
	.keymap = wl_keyboard_keymap,
	.enter = noop,
	.leave = noop,
	.key = wl_keyboard_key,
	.modifiers = wl_keyboard_modifiers,
	.repeat_info = noop,
};

/************************************************************************************************/
/*********************************** DRAWING APPLAUNCHER ****************************************/
/************************************************************************************************/
static void wl_buffer_release(void *data, struct wl_buffer *wl_buffer) {
	/* Sent by the compositor when it's no longer using this buffer */
	(void)data;
	wl_buffer_destroy(wl_buffer);
}

static const struct wl_buffer_listener wl_buffer_listener = {
	.release = wl_buffer_release,
};

static struct wl_buffer *draw_frame(struct client_state *state) {
	int width = state->width;
	int height = state->height;
	int stride = width * 4;
	int size = stride * height;
	int fd = allocate_shm_file(size);

	uint32_t *data = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

	struct wl_shm_pool *pool = wl_shm_create_pool(state->wl_shm, fd, size);
	struct wl_buffer *buffer = wl_shm_pool_create_buffer(pool, 0, width, height, stride,
																	WL_SHM_FORMAT_ARGB8888);
	
	cairo_surface_t *surface = cairo_image_surface_create_for_data((unsigned char *)data,
																		CAIRO_FORMAT_RGB24,
																		width,
																		height,
																		stride);
	cairo_t *cr = cairo_create(surface);
	cairo_paint(cr);

	// Grey color
	cairo_set_source_rgba(cr, 0.2, 0.2, 0.2, 1.0);
	cairo_rectangle(cr, 0, 0, width, height);
	cairo_fill(cr);

	// Draw frame around
	cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 1.0);
	cairo_set_line_width(cr, 3);
	cairo_rectangle(cr, 0, 0, width, height);
	cairo_stroke(cr);

	// Draw prompt
	cairo_set_source_rgba(cr, 0.3, 0.3, 0.3, 1.0);
	cairo_set_line_width(cr, 1);
	cairo_rectangle(cr, 5, 5, width - 10, 59);
	cairo_fill(cr);

	// Draw prompt frame
	cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 1.0);
	cairo_set_line_width(cr, 1);
	cairo_rectangle(cr, 5, 5, width - 10, 59);
	cairo_stroke(cr);

	// Typing text
	cairo_set_font_size(cr, 40);
	cairo_move_to(cr, 15, 45);
	cairo_show_text(cr, tempTextBuff);

	// Creating the list of all apps
	int pos_y = 103;
	cairo_set_font_size(cr, 35);
	for (int i = 0; names[i] != NULL && filteredList[i] != NULL; i++) {
		// Limit results list height
		if (state->height > 510) {
			state->height = 510;
		}
		cairo_move_to(cr, 65, pos_y);
		if (i == 0) {
			cairo_set_source_rgba(cr, 0.4, 1.0, 0.9, 1.0);
		}
		else {
			cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 1.0);
		}
		cairo_show_text(cr, filteredList[i]);

		// Drawing icons
		const RsvgRectangle rect = {
			.x = 10,
			.y = pos_y - 33,
			.width = 45,
			.height = 45,
		};
		// Get icon path
		for (int i = 0; names[i] != NULL; i++) {
			for (int j = 0; filteredList[j] != NULL; j++) {
				if (strcmp(names[i], filteredList[j]) == 0) {
					filteredListIcons[j] = icons[i];
					continue;
				}
			}
		}
		RsvgHandle *svgIcon = rsvg_handle_new_from_file(filteredListIcons[i], NULL);
		rsvg_handle_render_document(svgIcon, cr, &rect, NULL);
		// Destroy cairo
		g_object_unref(svgIcon);

		pos_y = pos_y + 50;
	}	

	// Clear resources
	cairo_surface_flush(surface); // Flush changes to the surface
	cairo_surface_destroy(surface);
	cairo_destroy(cr);

	wl_shm_pool_destroy(pool);
	close(fd);

	munmap(data, size);
	wl_buffer_add_listener(buffer, &wl_buffer_listener, NULL);
	return buffer;
}

static void wl_seat_capabilities(void *data, struct wl_seat *wl_seat, uint32_t capabilities) {
	(void)wl_seat;
	struct client_state *state = data;
	bool have_keyboard = capabilities & WL_SEAT_CAPABILITY_KEYBOARD;
	if (have_keyboard && state->wl_keyboard == NULL) {
		state->wl_keyboard = wl_seat_get_keyboard(state->wl_seat);
		wl_keyboard_add_listener(state->wl_keyboard, &wl_keyboard_listener, state);
	}
	else if (!have_keyboard && state->wl_keyboard != NULL) {
		wl_keyboard_release(state->wl_keyboard);
		state->wl_keyboard = NULL;
	}
}

static const struct wl_seat_listener wl_seat_listener = {
	.capabilities = wl_seat_capabilities,
	.name = noop,
};

/* Configuring XDG surface */
static void xdg_surface_configure(void *data, struct xdg_surface *xdg_surface, uint32_t serial) {
    struct client_state *state = data;
    state->xdg_serial = serial;
    xdg_surface_ack_configure(xdg_surface, state->xdg_serial);
    struct wl_buffer *buffer = draw_frame(state);
	wl_surface_attach(state->wl_surface, buffer, 0, 0);
	wl_surface_commit(state->wl_surface);
}

static const struct xdg_surface_listener xdg_surface_listener = {
    .configure = xdg_surface_configure,
};

static void xdg_wm_base_ping(void *data, struct xdg_wm_base *xdg_wm_base, uint32_t serial) {
	(void)data;
    xdg_wm_base_pong(xdg_wm_base, serial);
}

static const struct xdg_wm_base_listener xdg_wm_base_listener = {
    .ping = xdg_wm_base_ping,
};

/* Configure XDG toplevel */
static void xdg_toplevel_close(void *data, struct xdg_toplevel *xdg_toplevel) {
	(void)data;
	(void)xdg_toplevel;
	struct client_state *state = data;
	printf("Closing toplevel\n");
	state->closed = true;
}

static const struct xdg_toplevel_listener xdg_toplevel_listener = {
	.configure = noop,
	.close = xdg_toplevel_close,
	.configure_bounds = noop,
	.wm_capabilities = noop,
};

static void registry_global(void *data, struct wl_registry *wl_registry, uint32_t name,
													const char *interface, uint32_t version) {
	(void)version;
	struct client_state *state = data;
	if (strcmp(interface, wl_shm_interface.name) == 0) {
		state->wl_shm = wl_registry_bind(wl_registry, name, &wl_shm_interface, 1);
	}
	else if (strcmp(interface, wl_compositor_interface.name) == 0) {
		state->wl_compositor = wl_registry_bind(wl_registry, name, &wl_compositor_interface, 4);
	}
	else if (strcmp(interface, wl_seat_interface.name) == 0) {
		state->wl_seat = wl_registry_bind(wl_registry, name, &wl_seat_interface, 7);
		wl_seat_add_listener(state->wl_seat, &wl_seat_listener, state);
	}
    else if (strcmp(interface, xdg_wm_base_interface.name) == 0) {
        state->xdg_wm_base = wl_registry_bind(wl_registry, name, &xdg_wm_base_interface, 1);
        xdg_wm_base_add_listener(state->xdg_wm_base, &xdg_wm_base_listener, state);
    }
}

static void registry_global_remove(void *data, struct wl_registry *wl_registry, uint32_t name) {
	(void)data;
	(void)name;
	(void)wl_registry;
	/* This space deliberately left blank */
	wl_registry_destroy(wl_registry);
}

static const struct wl_registry_listener wl_registry_listener = {
	.global = registry_global,
	.global_remove = registry_global_remove,
};

int main(void) {
	// Creating the buffer for text input
	tempTextBuff = malloc(sizeof(char) * 2048);
	tempTextBuff[0] = '_';

	// Create confinguration files
	create_configs();

	// Populating arrays with data from .desktop files
	const char *HOME = getenv("HOME");
	if (HOME == NULL) {
		fprintf(stderr, "Unable to determine the user's home directory.\n");
		return 1;
	}
	const char *localPath = "/.local/share/applications";
	char localPathBuff[strlen(HOME) + strlen(localPath) + 3];
	snprintf(localPathBuff, sizeof(localPathBuff), "%s%s", HOME, localPath);
	if (is_directory(localPathBuff)) {
		process_directory(localPathBuff);
	}
	else {
		fprintf(stderr, "Path: '%s' not found!\n", localPathBuff);
	}
	if (is_directory("/usr/share/applications")) {
		process_directory("/usr/share/applications");
	}
	else {
		fprintf(stderr, "Path: '/usr/share/applications' not found!\n");
	}
	if (is_directory("/usr/local/share/applications")) {
		process_directory("/usr/local/share/applications");
	}
	else {
		fprintf(stderr, "Path: '/usr/local/share/applications' not found!\n");
	}

	struct client_state state = { 0 };
	state.width = 700;
	state.height = 70;
    
	state.wl_display = wl_display_connect(NULL);
	state.wl_registry = wl_display_get_registry(state.wl_display);
	wl_registry_add_listener(state.wl_registry, &wl_registry_listener, &state);
    state.xkb_context = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
	wl_display_roundtrip(state.wl_display);
	state.wl_surface = wl_compositor_create_surface(state.wl_compositor);
    state.xdg_surface = xdg_wm_base_get_xdg_surface(state.xdg_wm_base, state.wl_surface);
	xdg_surface_add_listener(state.xdg_surface, &xdg_surface_listener, &state);
	state.xdg_toplevel = xdg_surface_get_toplevel(state.xdg_surface);
	xdg_toplevel_add_listener(state.xdg_toplevel, &xdg_toplevel_listener, &state);
    xdg_toplevel_set_app_id(state.xdg_toplevel, "diowapplauncher");
    xdg_toplevel_set_title(state.xdg_toplevel, "DioW App Launcher");
	/// committing the surface showing the panel
	wl_surface_commit(state.wl_surface);

	while (!state.closed && wl_display_dispatch(state.wl_display) != -1) {
		/* This space deliberately left blank */
	}

	// free resources
	if (names[0] != NULL) {
		for (int i = 0; names[i] != NULL; i++) {
			free(names[i]);
			free(execs[i]);
			free(icons[i]);
		}
	}

	xkb_context_unref(state.xkb_context);
	xkb_keymap_unref(state.xkb_keymap);
	xkb_state_unref(state.xkb_state);
	/// clear SSIDs and security status names
	if (tempTextBuff != NULL) {
		free(tempTextBuff);
		tempTextBuff = NULL;
	}
	if (state.wl_keyboard) {
		wl_keyboard_destroy(state.wl_keyboard);
	}
	if (state.xdg_toplevel) {
		xdg_toplevel_destroy(state.xdg_toplevel);
	}
	if (state.xdg_surface) {
		xdg_surface_destroy(state.xdg_surface);
	}
	if (state.wl_surface) {
		wl_surface_destroy(state.wl_surface);
	}
	if (state.wl_seat) {
		wl_seat_destroy(state.wl_seat);
	}
	if (state.xdg_wm_base) {
		xdg_wm_base_destroy(state.xdg_wm_base);
	}
	if (state.wl_registry) {
		wl_registry_destroy(state.wl_registry);
	}
	if (state.wl_display) {
		wl_display_disconnect(state.wl_display);
	}

	printf("Wayland client terminated!\n");

    return 0;
}
