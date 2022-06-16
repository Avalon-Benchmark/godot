/*************************************************************************/
/*  os_egl.cpp                                                           */
/*************************************************************************/
/*                       This file is part of:                           */
/*                           GODOT ENGINE                                */
/*                      https://godotengine.org                          */
/*************************************************************************/
/* Copyright (c) 2007-2021 Juan Linietsky, Ariel Manzur.                 */
/* Copyright (c) 2014-2021 Godot Engine contributors (cf. AUTHORS.md).   */
/*                                                                       */
/* Permission is hereby granted, free of charge, to any person obtaining */
/* a copy of this software and associated documentation files (the       */
/* "Software"), to deal in the Software without restriction, including   */
/* without limitation the rights to use, copy, modify, merge, publish,   */
/* distribute, sublicense, and/or sell copies of the Software, and to    */
/* permit persons to whom the Software is furnished to do so, subject to */
/* the following conditions:                                             */
/*                                                                       */
/* The above copyright notice and this permission notice shall be        */
/* included in all copies or substantial portions of the Software.       */
/*                                                                       */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,       */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF    */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.*/
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY  */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,  */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE     */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                */
/*************************************************************************/

#include "os_egl.h"

#include "core/os/dir_access.h"
#include "core/print_string.h"
#include "drivers/gles2/rasterizer_gles2.h"
#include "drivers/gles3/rasterizer_gles3.h"
#include "main/main.h"
#include "servers/visual/visual_server_raster.h"
#include "servers/visual/visual_server_wrap_mt.h"

#ifdef HAVE_MNTENT
#include <mntent.h>
#endif

#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <dlfcn.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

//stupid linux.h
#ifdef KEY_TAB
#undef KEY_TAB
#endif

static const double abs_resolution_mult = 10000.0;
static const double abs_resolution_range_mult = 10.0;

void OS_EGL::initialize_core() {

	crash_handler.initialize();

	OS_Unix::initialize_core();
}

int OS_EGL::get_current_video_driver() const {
	return video_driver_index;
}

Error OS_EGL::initialize(const VideoMode &p_desired, int p_video_driver, int p_audio_driver) {
	last_button_state = 0;

	last_click_ms = 0;
	last_click_button_index = -1;
	last_click_pos = Point2(-100, -100);
	args = OS::get_singleton()->get_cmdline_args();
	current_videomode = p_desired;
	main_loop = NULL;
	last_timestamp = 0;
	last_mouse_pos_valid = false;

// maybe contextgl wants to be in charge of creating the window
#if defined(OPENGL_ENABLED)
	ContextGL_EGL::ContextType opengl_api_type = ContextGL_EGL::GLES_3_0_COMPATIBLE;

	if (p_video_driver == VIDEO_DRIVER_GLES2) {
		opengl_api_type = ContextGL_EGL::GLES_2_0_COMPATIBLE;
	}

	bool editor = Engine::get_singleton()->is_editor_hint();
	bool gl_initialization_error = false;

	context_gl = NULL;
	while (!context_gl) {
		context_gl = memnew(ContextGL_EGL(current_videomode, opengl_api_type));

		if (context_gl->initialize(p_desired.width, p_desired.height) != OK) {
			memdelete(context_gl);
			context_gl = NULL;

			if (GLOBAL_GET("rendering/quality/driver/fallback_to_gles2") || editor) {
				if (p_video_driver == VIDEO_DRIVER_GLES2) {
					gl_initialization_error = true;
					break;
				}

				p_video_driver = VIDEO_DRIVER_GLES2;
				opengl_api_type = ContextGL_EGL::GLES_2_0_COMPATIBLE;
			} else {
				gl_initialization_error = true;
				break;
			}
		}
	}

	while (true) {
		if (opengl_api_type == ContextGL_EGL::GLES_3_0_COMPATIBLE) {
			if (RasterizerGLES3::is_viable() == OK) {
				RasterizerGLES3::register_config();
				RasterizerGLES3::make_current();
				break;
			} else {
				if (GLOBAL_GET("rendering/quality/driver/fallback_to_gles2") || editor) {
					p_video_driver = VIDEO_DRIVER_GLES2;
					opengl_api_type = ContextGL_EGL::GLES_2_0_COMPATIBLE;
					continue;
				} else {
					gl_initialization_error = true;
					break;
				}
			}
		}

		if (opengl_api_type == ContextGL_EGL::GLES_2_0_COMPATIBLE) {
			if (RasterizerGLES2::is_viable() == OK) {
				RasterizerGLES2::register_config();
				RasterizerGLES2::make_current();
				break;
			} else {
				gl_initialization_error = true;
				break;
			}
		}
	}

	if (gl_initialization_error) {
		OS::get_singleton()->alert("Your video card driver does not support any of the supported OpenGL versions.\n"
								   "Please update your drivers or if you have a very old or integrated GPU, upgrade it.\n"
								   "Alternatively, you can force software rendering by running Godot with the `LIBGL_ALWAYS_SOFTWARE=1`\n"
								   "environment variable set, but this will be very slow.",
				"Unable to initialize Video driver");
		return ERR_UNAVAILABLE;
	}

	video_driver_index = p_video_driver;

	context_gl->set_use_vsync(current_videomode.use_vsync);

#endif

	visual_server = memnew(VisualServerRaster);
	if (get_render_thread_mode() != RENDER_THREAD_UNSAFE) {
		visual_server = memnew(VisualServerWrapMT(visual_server, get_render_thread_mode() == RENDER_SEPARATE_THREAD));
	}

	if (current_videomode.maximized) {
		current_videomode.maximized = false;
		set_window_maximized(true);
		// borderless fullscreen window mode
	} else if (current_videomode.fullscreen) {
		current_videomode.fullscreen = false;
		set_window_fullscreen(true);
	} else if (current_videomode.borderless_window) {
	}

	// disable resizable window
	if (!current_videomode.resizable && !current_videomode.fullscreen) {
	}

	if (current_videomode.always_on_top) {
		current_videomode.always_on_top = false;
		set_window_always_on_top(true);
	}

	ERR_FAIL_COND_V(!visual_server, ERR_UNAVAILABLE);

	im_active = false;
	im_position = Vector2();

	visual_server->init();

	AudioDriverManager::initialize(p_audio_driver);

	input = memnew(InputDefault);

	window_has_focus = true; // Set focus to true at init

	if (p_desired.layered) {
		set_window_per_pixel_transparency_enabled(true);
	}

	return OK;
}

void OS_EGL::set_ime_active(const bool p_active) {
	im_active = p_active;
}

void OS_EGL::set_ime_position(const Point2 &p_pos) {
	im_position = p_pos;
}

String OS_EGL::get_unique_id() const {

	static String machine_id;
	if (machine_id.empty()) {
		if (FileAccess *f = FileAccess::open("/etc/machine-id", FileAccess::READ)) {
			while (machine_id.empty() && !f->eof_reached()) {
				machine_id = f->get_line().strip_edges();
			}
			f->close();
			memdelete(f);
		}
	}
	return machine_id;
}

void OS_EGL::finalize() {
	if (main_loop)
		memdelete(main_loop);
	main_loop = NULL;

	/*
	if (debugger_connection_console) {
		memdelete(debugger_connection_console);
	}
	*/

	memdelete(input);
	visual_server->finish();
	memdelete(visual_server);
	//memdelete(rasterizer);

#if defined(OPENGL_ENABLED)
	memdelete(context_gl);
#endif
	args.clear();
}

void OS_EGL::set_mouse_mode(MouseMode p_mode) {
}

void OS_EGL::warp_mouse_position(const Point2 &p_to) {
}

OS::MouseMode OS_EGL::get_mouse_mode() const {
	return mouse_mode;
}

int OS_EGL::get_mouse_button_state() const {
	return last_button_state;
}

Point2 OS_EGL::get_mouse_position() const {
	return last_mouse_pos;
}

bool OS_EGL::get_window_per_pixel_transparency_enabled() const {

	if (!is_layered_allowed()) return false;
	return layered_window;
}

void OS_EGL::set_window_per_pixel_transparency_enabled(bool p_enabled) {

	if (!is_layered_allowed()) return;
	if (layered_window != p_enabled) {
		if (p_enabled) {
			layered_window = true;
		} else {
			layered_window = false;
		}
	}
}

void OS_EGL::set_window_title(const String &p_title) {
}

void OS_EGL::set_window_mouse_passthrough(const PoolVector2Array &p_region) {
}

void OS_EGL::set_video_mode(const VideoMode &p_video_mode, int p_screen) {
}

OS::VideoMode OS_EGL::get_video_mode(int p_screen) const {
	return current_videomode;
}

void OS_EGL::get_fullscreen_mode_list(List<VideoMode> *p_list, int p_screen) const {
}

void OS_EGL::set_wm_fullscreen(bool p_enabled) {
}

void OS_EGL::set_wm_above(bool p_enabled) {
}

int OS_EGL::get_screen_count() const {
	return 1;
}

int OS_EGL::get_current_screen() const {
	return 0;
}

void OS_EGL::set_current_screen(int p_screen) {
}

Point2 OS_EGL::get_screen_position(int p_screen) const {
	return Point2i(0, 0);
}

Size2 OS_EGL::get_screen_size(int p_screen) const {
	return Point2i(current_videomode.width, current_videomode.height);
}

int OS_EGL::get_screen_dpi(int p_screen) const {
	return 96;
}

Point2 OS_EGL::get_window_position() const {
	return Point2i(0, 0);
}

void OS_EGL::set_window_position(const Point2 &p_position) {
}

Size2 OS_EGL::get_window_size() const {
	return Point2i(current_videomode.width, current_videomode.height);
}

Size2 OS_EGL::get_real_window_size() const {
	return Point2i(current_videomode.width, current_videomode.height);
}

Size2 OS_EGL::get_max_window_size() const {
	return max_size;
}

Size2 OS_EGL::get_min_window_size() const {
	return min_size;
}

void OS_EGL::set_min_window_size(const Size2 p_size) {
	min_size = p_size;
}

void OS_EGL::set_max_window_size(const Size2 p_size) {
	max_size = p_size;
}

void OS_EGL::set_window_size(const Size2 p_size) {
	context_gl->set_buffer_size(p_size.width, p_size.height);
//	glViewport(0, 0, p_size.width, p_size.height);
//	glScissor(0, 0, p_size.width, p_size.height);
	current_videomode.width = p_size.width;
	current_videomode.height = p_size.height;
}

void OS_EGL::set_window_fullscreen(bool p_enabled) {
}

bool OS_EGL::is_window_fullscreen() const {
	return current_videomode.fullscreen;
}

void OS_EGL::set_window_resizable(bool p_enabled) {
}

bool OS_EGL::is_window_resizable() const {
	return current_videomode.resizable;
}

void OS_EGL::set_window_minimized(bool p_enabled) {
}

bool OS_EGL::is_window_minimized() const {
	return false;
}

void OS_EGL::set_window_maximized(bool p_enabled) {
}

bool OS_EGL::is_window_maximize_allowed() const {
	return true;
}

bool OS_EGL::is_window_maximized() const {
	return false;
}

void OS_EGL::set_window_always_on_top(bool p_enabled) {
	current_videomode.always_on_top = p_enabled;
}

bool OS_EGL::is_window_always_on_top() const {
	return current_videomode.always_on_top;
}

bool OS_EGL::is_window_focused() const {
	return window_focused;
}

void OS_EGL::set_borderless_window(bool p_borderless) {
}

bool OS_EGL::get_borderless_window() {
	return false;
}

void OS_EGL::request_attention() {
}

void *OS_EGL::get_native_handle(int p_handle_type) {
	switch (p_handle_type) {
		case APPLICATION_HANDLE:
		case DISPLAY_HANDLE:
		case WINDOW_HANDLE:
		case WINDOW_VIEW:
		case OPENGL_CONTEXT:
		default: return NULL;
	}
}

MainLoop *OS_EGL::get_main_loop() const {

	return main_loop;
}

void OS_EGL::delete_main_loop() {
	if (main_loop)
		memdelete(main_loop);
	main_loop = NULL;
}

void OS_EGL::set_main_loop(MainLoop *p_main_loop) {
	main_loop = p_main_loop;
	input->set_main_loop(p_main_loop);
}

bool OS_EGL::can_draw() const {
	return !minimized;
};

void OS_EGL::set_clipboard(const String &p_text) {
};

String OS_EGL::get_clipboard() const {
	return "";
}

String OS_EGL::get_name() const {

	return "X11";
}

Error OS_EGL::shell_open(String p_uri) {
	Error ok;
	int err_code;
	List<String> args;
	args.push_back(p_uri);

	// Agnostic
	ok = execute("xdg-open", args, true, NULL, NULL, &err_code);
	if (ok == OK && !err_code) {
		return OK;
	} else if (err_code == 2) {
		return ERR_FILE_NOT_FOUND;
	}
	// GNOME
	args.push_front("open"); // The command is `gio open`, so we need to add it to args
	ok = execute("gio", args, true, NULL, NULL, &err_code);
	if (ok == OK && !err_code) {
		return OK;
	} else if (err_code == 2) {
		return ERR_FILE_NOT_FOUND;
	}
	args.pop_front();
	ok = execute("gvfs-open", args, true, NULL, NULL, &err_code);
	if (ok == OK && !err_code) {
		return OK;
	} else if (err_code == 2) {
		return ERR_FILE_NOT_FOUND;
	}
	// KDE
	ok = execute("kde-open5", args, true, NULL, NULL, &err_code);
	if (ok == OK && !err_code) {
		return OK;
	}
	ok = execute("kde-open", args, true, NULL, NULL, &err_code);
	return !err_code ? ok : FAILED;
}

bool OS_EGL::_check_internal_feature_support(const String &p_feature) {

	return p_feature == "pc";
}

String OS_EGL::get_config_path() const {

	if (has_environment("XDG_CONFIG_HOME")) {
		return get_environment("XDG_CONFIG_HOME");
	} else if (has_environment("HOME")) {
		return get_environment("HOME").plus_file(".config");
	} else {
		return ".";
	}
}

String OS_EGL::get_data_path() const {

	if (has_environment("XDG_DATA_HOME")) {
		return get_environment("XDG_DATA_HOME");
	} else if (has_environment("HOME")) {
		return get_environment("HOME").plus_file(".local/share");
	} else {
		return get_config_path();
	}
}

String OS_EGL::get_cache_path() const {

	if (has_environment("XDG_CACHE_HOME")) {
		return get_environment("XDG_CACHE_HOME");
	} else if (has_environment("HOME")) {
		return get_environment("HOME").plus_file(".cache");
	} else {
		return get_config_path();
	}
}

String OS_EGL::get_system_dir(SystemDir p_dir, bool p_shared_storage) const {
	String xdgparam;

	switch (p_dir) {
		case SYSTEM_DIR_DESKTOP: {

			xdgparam = "DESKTOP";
		} break;
		case SYSTEM_DIR_DCIM: {

			xdgparam = "PICTURES";

		} break;
		case SYSTEM_DIR_DOCUMENTS: {

			xdgparam = "DOCUMENTS";

		} break;
		case SYSTEM_DIR_DOWNLOADS: {

			xdgparam = "DOWNLOAD";

		} break;
		case SYSTEM_DIR_MOVIES: {

			xdgparam = "VIDEOS";

		} break;
		case SYSTEM_DIR_MUSIC: {

			xdgparam = "MUSIC";

		} break;
		case SYSTEM_DIR_PICTURES: {

			xdgparam = "PICTURES";

		} break;
		case SYSTEM_DIR_RINGTONES: {

			xdgparam = "MUSIC";

		} break;
	}

	String pipe;
	List<String> arg;
	arg.push_back(xdgparam);
	Error err = const_cast<OS_EGL *>(this)->execute("xdg-user-dir", arg, true, NULL, &pipe);
	if (err != OK)
		return ".";
	return pipe.strip_edges();
}

void OS_EGL::move_window_to_foreground() {
}

void OS_EGL::set_cursor_shape(CursorShape p_shape) {
}

OS::CursorShape OS_EGL::get_cursor_shape() const {
	return CURSOR_ARROW;
}

void OS_EGL::set_custom_mouse_cursor(const RES &p_cursor, CursorShape p_shape, const Vector2 &p_hotspot) {
}

void OS_EGL::release_rendering_thread() {

#if defined(OPENGL_ENABLED)
	context_gl->release_current();
#endif
}

void OS_EGL::make_rendering_thread() {

#if defined(OPENGL_ENABLED)
	context_gl->make_current();
#endif
}

void OS_EGL::swap_buffers() {

#if defined(OPENGL_ENABLED)
	context_gl->swap_buffers();
#endif
}

void OS_EGL::alert(const String &p_alert, const String &p_title) {

	if (is_no_window_mode_enabled()) {
		print_line("ALERT: " + p_title + ": " + p_alert);
		return;
	}

	const char *message_programs[] = { "zenity", "kdialog", "Xdialog", "xmessage" };

	String path = get_environment("PATH");
	Vector<String> path_elems = path.split(":", false);
	String program;

	for (int i = 0; i < path_elems.size(); i++) {
		for (uint64_t k = 0; k < sizeof(message_programs) / sizeof(char *); k++) {
			String tested_path = path_elems[i].plus_file(message_programs[k]);

			if (FileAccess::exists(tested_path)) {
				program = tested_path;
				break;
			}
		}

		if (program.length())
			break;
	}

	List<String> args;

	if (program.ends_with("zenity")) {
		args.push_back("--error");
		args.push_back("--width");
		args.push_back("500");
		args.push_back("--title");
		args.push_back(p_title);
		args.push_back("--text");
		args.push_back(p_alert);
	}

	if (program.ends_with("kdialog")) {
		args.push_back("--error");
		args.push_back(p_alert);
		args.push_back("--title");
		args.push_back(p_title);
	}

	if (program.ends_with("Xdialog")) {
		args.push_back("--title");
		args.push_back(p_title);
		args.push_back("--msgbox");
		args.push_back(p_alert);
		args.push_back("0");
		args.push_back("0");
	}

	if (program.ends_with("xmessage")) {
		args.push_back("-center");
		args.push_back("-title");
		args.push_back(p_title);
		args.push_back(p_alert);
	}

	if (program.length()) {
		execute(program, args, true);
	} else {
		print_line(p_alert);
	}
}


void OS_EGL::set_icon(const Ref<Image> &p_icon) {
}

void OS_EGL::force_process_input() {
}

void OS_EGL::run() {

	force_quit = false;

	if (!main_loop)
		return;

	main_loop->init();

	//uint64_t last_ticks=get_ticks_usec();

	//int frames=0;
	//uint64_t frame=0;

	while (!force_quit) {
		if (Main::iteration())
			break;
	};

	main_loop->finish();
}

bool OS_EGL::is_joy_known(int p_device) {
	return input->is_joy_mapped(p_device);
}

String OS_EGL::get_joy_guid(int p_device) const {
	return input->get_joy_guid_remapped(p_device);
}

void OS_EGL::_set_use_vsync(bool p_enable) {
#if defined(OPENGL_ENABLED)
	if (context_gl)
		context_gl->set_use_vsync(p_enable);
#endif
}
/*
bool OS_EGL::is_vsync_enabled() const {

	if (context_gl)
		return context_gl->is_using_vsync();

	return true;
}
*/
void OS_EGL::set_context(int p_context) {
}

OS::PowerState OS_EGL::get_power_state() {
	return POWERSTATE_NO_BATTERY;
}

int OS_EGL::get_power_seconds_left() {
	return -1;
}

int OS_EGL::get_power_percent_left() {
	return -1;
}

void OS_EGL::disable_crash_handler() {
	crash_handler.disable();
}

bool OS_EGL::is_disable_crash_handler() const {
	return crash_handler.is_disabled();
}

Error OS_EGL::move_to_trash(const String &p_path) {
	DirAccess *dir_access = DirAccess::create(DirAccess::ACCESS_FILESYSTEM);
	Error err = dir_access->remove(p_path);
	memdelete(dir_access);
	return err;
}

OS::LatinKeyboardVariant OS_EGL::get_latin_keyboard_variant() const {
	return LATIN_KEYBOARD_QWERTY;
}

int OS_EGL::keyboard_get_layout_count() const {
	return 0;
}

int OS_EGL::keyboard_get_current_layout() const {
	return 0;
}

void OS_EGL::keyboard_set_current_layout(int p_index) {
}

String OS_EGL::keyboard_get_layout_language(int p_index) const {
	return "";
}

String OS_EGL::keyboard_get_layout_name(int p_index) const {
	return "";
}


OS_EGL::OS_EGL() {
	layered_window = false;
	minimized = false;
	window_focused = true;
	mouse_mode = MOUSE_MODE_VISIBLE;
	last_position_before_fs = Vector2();
}
