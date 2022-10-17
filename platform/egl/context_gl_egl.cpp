/*************************************************************************/
/*  context_gl_egl.cpp                                                   */
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

#include "context_gl_egl.h"

#ifdef EGL_ENABLED
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include "thirdparty/glad/glad/glad.h"

EGLDisplay ContextGL_EGL::get_display() {
	PFNEGLGETPLATFORMDISPLAYEXTPROC eglGetPlatformDisplayEXT = (PFNEGLGETPLATFORMDISPLAYEXTPROC) eglGetProcAddress("eglGetPlatformDisplayEXT");
	PFNEGLQUERYDEVICESEXTPROC eglQueryDevicesEXT = (PFNEGLQUERYDEVICESEXTPROC) eglGetProcAddress("eglQueryDevicesEXT");
	PFNEGLQUERYDEVICEATTRIBEXTPROC eglQueryDeviceAttribEXT = (PFNEGLQUERYDEVICEATTRIBEXTPROC) eglGetProcAddress("eglQueryDeviceAttribEXT");

	char *env_cuda_id = std::getenv("EGL_CUDA_ID");
	int dev_cuda_id = -1;
	if (env_cuda_id) {
		dev_cuda_id = std::atoi(env_cuda_id);
	}

	EGLDeviceEXT egl_devices[16] = {};
	EGLint num_devices = 0;
	eglQueryDevicesEXT(16, &egl_devices[0], &num_devices);

	int egl_selected = -1;
	int egl_fallback = -1;
	int last_cuda_id = -1;

	for (int i=0; i<num_devices; i++) {
		EGLDeviceEXT egl_device = egl_devices[i];
		EGLAttrib egl_cuda_id;
		if (eglQueryDeviceAttribEXT(egl_device, EGL_CUDA_DEVICE_NV, &egl_cuda_id)) {
			if (last_cuda_id < egl_cuda_id ) {
				last_cuda_id = egl_cuda_id;
				egl_fallback = i;
			}
			if (egl_cuda_id == dev_cuda_id) {
				egl_selected = i;
			}
		}
	}

	if (egl_selected >= 0) {
		return eglGetPlatformDisplayEXT(EGL_PLATFORM_DEVICE_EXT, egl_devices[egl_selected], NULL);
	}
	if (egl_fallback >= 0) {
		return eglGetPlatformDisplayEXT(EGL_PLATFORM_DEVICE_EXT, egl_devices[egl_fallback], NULL);
	}

	return eglGetPlatformDisplayEXT(EGL_PLATFORM_DEVICE_EXT, EGL_DEFAULT_DISPLAY, NULL);
}

void ContextGL_EGL::release_current() {
	eglMakeCurrent(eglDpy, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
}

void ContextGL_EGL::make_current() {
	eglMakeCurrent(eglDpy, eglSurf, eglSurf, eglCtx);
}

void ContextGL_EGL::swap_buffers() {
	if (mmap_path == NULL) {
		eglSwapBuffers(eglDpy, eglSurf);
		return;
	}

	if (mmap_pbo_id == 0) {
		glGenBuffers(1, &mmap_pbo_id);
		glBindBuffer(GL_PIXEL_PACK_BUFFER, mmap_pbo_id);
		glBufferData(GL_PIXEL_PACK_BUFFER, mmap_size, NULL, GL_STREAM_READ);
		glReadPixels(0, 0, mmap_size_x, mmap_size_y, GL_RGBA, GL_UNSIGNED_BYTE, (void*)0);
		glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
	}

	glBindBuffer(GL_PIXEL_PACK_BUFFER, mmap_pbo_id);
	void *ptr = glMapBuffer(GL_PIXEL_PACK_BUFFER, GL_READ_ONLY);
	memcpy(mmap_data, ptr, mmap_size);
	glUnmapBuffer(GL_PIXEL_PACK_BUFFER);

	eglSwapBuffers(eglDpy, eglSurf);

	glBufferData(GL_PIXEL_PACK_BUFFER, mmap_size, NULL, GL_STREAM_READ);
	glPixelStorei(GL_PACK_ALIGNMENT, 1);
	glReadPixels(0, 0, mmap_size_x, mmap_size_y, GL_RGBA, GL_UNSIGNED_BYTE, (void*)0);
	glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
}

void ContextGL_EGL::set_buffer_mmap(const int width, const int height) {
	if (mmap_data) {
		munmap(mmap_data, mmap_size);
	}
	if (mmap_file > 0) {
		close(mmap_file);
	}
	if (mmap_path) {
		mmap_file = open(mmap_path, O_RDWR | O_CREAT | O_TRUNC, 0666);
		mmap_size_x = width;
		mmap_size_y = height;
		mmap_size_z = 4;
		mmap_size = mmap_size_x * mmap_size_y * mmap_size_z;
		if (ftruncate(mmap_file, mmap_size)) {
			mmap_size = 0;
			mmap_data = NULL;
		} else {
			mmap_data = mmap(NULL, mmap_size, PROT_READ | PROT_WRITE, MAP_SHARED, mmap_file, 0);
		}
		mmap_pbo_id = 0;
	} else {
		mmap_file = -1;
		mmap_size_x = 0;
		mmap_size_y = 0;
		mmap_size_y = 0;
		mmap_size = 0;
		mmap_data = NULL;
		mmap_pbo_id = 0;
	}
}

void ContextGL_EGL::set_buffer_size(const int width, const int height) {
	eglMakeCurrent(eglDpy, EGL_NO_SURFACE, EGL_NO_SURFACE, eglCtx);
	eglDestroySurface(eglDpy, eglSurf);

	static const EGLint pbufferAttribs[] = {
		EGL_WIDTH, width,
		EGL_HEIGHT, height,
		EGL_NONE,
	};
	eglSurf = eglCreatePbufferSurface(eglDpy, eglCfg, pbufferAttribs);
	eglMakeCurrent(eglDpy, eglSurf, eglSurf, eglCtx);

	set_buffer_mmap(width, height);
}

Error ContextGL_EGL::initialize(const int width, const int height) {
	EGLint numConfigs;

	static const EGLint visual_attribs_layers[] = {
		EGL_RENDERABLE_TYPE, EGL_OPENGL_BIT | EGL_OPENGL_ES2_BIT,
		EGL_SURFACE_TYPE, EGL_PBUFFER_BIT,
		EGL_RED_SIZE, 8,
		EGL_GREEN_SIZE, 8,
		EGL_BLUE_SIZE, 8,
		EGL_ALPHA_SIZE, 8,
		EGL_DEPTH_SIZE, 24,
		EGL_NONE
	};

	static const EGLint visual_attribs_simple[] = {
		EGL_RENDERABLE_TYPE, EGL_OPENGL_BIT | EGL_OPENGL_ES2_BIT,
		EGL_SURFACE_TYPE, EGL_PBUFFER_BIT,
		EGL_RED_SIZE, 8,
		EGL_GREEN_SIZE, 8,
		EGL_BLUE_SIZE, 8,
		EGL_DEPTH_SIZE, 24,
		EGL_NONE
	};

	static const EGLint pbufferAttribs[] = {
		EGL_WIDTH, width,
		EGL_HEIGHT, height,
		EGL_NONE,
	};

	static const EGLint contextAttribs[] = {
		EGL_CONTEXT_MAJOR_VERSION, (context_type == ContextType::GLES_3_0_COMPATIBLE) ? 3 : 2,
		EGL_CONTEXT_MINOR_VERSION, (context_type == ContextType::GLES_3_0_COMPATIBLE) ? 3 : 0,
		EGL_NONE,
	};

	eglDpy = get_display();
	eglInitialize(eglDpy, &egl_major, &egl_minor);

	if (OS::get_singleton()->is_layered_allowed()) {
		eglChooseConfig(eglDpy, visual_attribs_layers, &eglCfg, 1, &numConfigs);
	} else {
		eglChooseConfig(eglDpy, visual_attribs_simple, &eglCfg, 1, &numConfigs);
	}

	eglSurf = eglCreatePbufferSurface(eglDpy, eglCfg, pbufferAttribs);
	eglBindAPI(EGL_OPENGL_API);

	eglCtx = eglCreateContext(eglDpy, eglCfg, EGL_NO_CONTEXT, contextAttribs);

	eglMakeCurrent(eglDpy, eglSurf, eglSurf, eglCtx);

	set_buffer_mmap(width, height);
	set_use_vsync(false);

	return OK;
}

int ContextGL_EGL::get_window_width() {
	EGLint size = 0;
	eglQuerySurface(eglDpy, eglSurf, EGL_WIDTH, &size);
	return size;
}

int ContextGL_EGL::get_window_height() {
	EGLint size = 0;
	eglQuerySurface(eglDpy, eglSurf, EGL_HEIGHT, &size);
	return size;
}

void *ContextGL_EGL::get_glx_context() {
	return eglCtx;
}

void ContextGL_EGL::set_use_vsync(bool p_use) {
	eglSwapInterval(eglDpy, p_use);
	use_vsync = p_use;
}
bool ContextGL_EGL::is_using_vsync() const {
	return use_vsync;
}

ContextGL_EGL::ContextGL_EGL(const OS::VideoMode &p_default_video_mode, ContextType p_context_type) {
	context_type = p_context_type;
	double_buffer = false;
	direct_render = false;
	egl_major = 0;
	egl_minor = 0;
	mmap_path = getenv("EGL_MMAP_PATH");
	mmap_file = -1;
	mmap_size_x = 0;
	mmap_size_y = 0;
	mmap_size_z = 0;
	mmap_size = 0;
	mmap_data = NULL;
	mmap_pbo_id = 0;
}

ContextGL_EGL::~ContextGL_EGL() {
	release_current();
	eglTerminate(eglDpy);
}

#endif
