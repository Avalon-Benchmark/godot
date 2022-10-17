/*************************************************************************/
/*  context_gl_egl.h                                                     */
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

#ifndef CONTEXT_GL_EGL_H
#define CONTEXT_GL_EGL_H

#ifdef EGL_ENABLED

#include "core/os/os.h"

#include <EGL/egl.h>
#include <EGL/eglext.h>

class ContextGL_EGL {

public:
	enum ContextType {
		OLDSTYLE,
		GLES_2_0_COMPATIBLE,
		GLES_3_0_COMPATIBLE
	};

private:
	OS::VideoMode default_video_mode;
	EGLDisplay eglDpy;
	EGLConfig eglCfg;
	EGLSurface eglSurf;
	EGLContext eglCtx;
	bool double_buffer;
	bool direct_render;
	EGLint egl_major;
	EGLint egl_minor;
	const char* mmap_path;
	size_t mmap_size_x;
	size_t mmap_size_y;
	size_t mmap_size_z;
	size_t mmap_size;
	int mmap_file;
	void* mmap_data;
	unsigned int mmap_pbo_id;
	bool use_vsync;
	ContextType context_type;
	EGLDisplay get_display();

public:
	void release_current();
	void make_current();
	void swap_buffers();
	void set_buffer_mmap(const int width, const int height);
	void set_buffer_size(const int width, const int height);
	int get_window_width();
	int get_window_height();
	void *get_glx_context();

	Error initialize(const int width, const int height);

	void set_use_vsync(bool p_use);
	bool is_using_vsync() const;

	ContextGL_EGL(const OS::VideoMode &p_default_video_mode, ContextType p_context_type);
	~ContextGL_EGL();
};

#endif

#endif
