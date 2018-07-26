/*
 * Copyright (c) 2012 Arvin Schnell <arvin.schnell@gmail.com>
 * Copyright (c) 2012 Rob Clark <rob@ti.com>
 * Copyright (c) 2013 Anand Balagopalakrishnan <anandb@ti.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sub license,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

/* Based on a egl cube test app originally written by Arvin Schnell */

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <GLES2/gl2.h>
#include <EGL/egl.h>
#include <wayland-client.h>
#include <wayland-egl.h>

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

#define WIDTH 256
#define HEIGHT 256

int width  =  WIDTH;
int height =  HEIGHT;
int readPix  =  0;
FILE *outFile = NULL;

static struct {
	EGLDisplay display;
	EGLConfig config;
	EGLContext context;
	EGLSurface surface;
	GLuint program;
	GLuint vertex_shader, fragment_shader;
	GLuint rotation_uniform;
} gl;

struct wayland {
	struct wl_display *display;
	struct wl_registry *registry;
	struct wl_compositor *compositor;
	struct wl_shell_surface *shell_surface;
	struct wl_shell *shell;
	struct wl_surface *surface;
	struct wl_egl_window *egl_window;
} wayland;

static void
registry_handle_global(void *data, struct wl_registry *registry,
		       uint32_t name, const char *interface, uint32_t version)
{
	struct wayland *d = data;

	if (strcmp(interface, "wl_compositor") == 0) {
		d->compositor =
			wl_registry_bind(registry, name,
					 &wl_compositor_interface, 1);
	} else if (strcmp(interface, "wl_shell") == 0) {
		d->shell = wl_registry_bind(registry, name,
					    &wl_shell_interface, 1);
	}
}

static void
registry_handle_global_remove(void *data, struct wl_registry *registry,
			      uint32_t name)
{
}

static const struct wl_registry_listener registry_listener = {
	registry_handle_global,
	registry_handle_global_remove
};

static int init_wayland (void)
{
	wayland.display = wl_display_connect(NULL);
	if(!wayland.display) {
		printf("wl_display_connect failed\n");
		return -1;
	}

	wayland.registry = wl_display_get_registry(wayland.display);
	wl_registry_add_listener(wayland.registry,
				 &registry_listener, &wayland);
	wl_display_dispatch(wayland.display);

	while(1) {
		if(wayland.compositor && wayland.shell)
			break;
		wl_display_roundtrip(wayland.display);
	}

	wayland.surface = wl_compositor_create_surface(wayland.compositor);
	wayland.shell_surface = wl_shell_get_shell_surface(wayland.shell, wayland.surface);
	wl_shell_surface_set_toplevel(wayland.shell_surface);

	wayland.egl_window = wl_egl_window_create(wayland.surface,
				width, height);

	return 0;

}

static int init_gl(void)
{
	EGLint major, minor, n;
	GLint ret;

	static const EGLint context_attribs[] = {
		EGL_CONTEXT_CLIENT_VERSION, 2,
		EGL_NONE
	};

	static const EGLint config_attribs[] = {
		EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
		EGL_RED_SIZE, 1,
		EGL_GREEN_SIZE, 1,
		EGL_BLUE_SIZE, 1,
		EGL_ALPHA_SIZE, 0,
		EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
		EGL_NONE
	};

	static const char *vertex_shader_source =
		"uniform mat4 rotation;\n"
		"attribute vec4 pos;\n"
		"attribute vec4 color;\n"
		"varying vec4 v_color;\n"
		"void main() {\n"
		"  gl_Position = rotation * pos;\n"
		"  v_color = color;\n"
		"}\n";

	static const char *fragment_shader_source =
		"precision mediump float;\n"
		"varying vec4 v_color;\n"
		"void main() {\n"
		"  gl_FragColor = v_color;\n"
		"}\n";

	gl.display = eglGetDisplay(wayland.display);

	if (!eglInitialize(gl.display, &major, &minor)) {
		printf("failed to initialize\n");
		return -1;
	}

	printf("Using display %p with EGL version %d.%d\n",
			gl.display, major, minor);

	printf("EGL Version \"%s\"\n", eglQueryString(gl.display, EGL_VERSION));
	printf("EGL Vendor \"%s\"\n", eglQueryString(gl.display, EGL_VENDOR));
	printf("EGL Extensions \"%s\"\n", eglQueryString(gl.display, EGL_EXTENSIONS));

	if (!eglBindAPI(EGL_OPENGL_ES_API)) {
		printf("failed to bind api EGL_OPENGL_ES_API\n");
		return -1;
	}

	if (!eglChooseConfig(gl.display, config_attribs, &gl.config, 1, &n) || n != 1) {
		printf("failed to choose config: %d\n", n);
		return -1;
	}

	gl.context = eglCreateContext(gl.display, gl.config,
			EGL_NO_CONTEXT, context_attribs);
	if (gl.context == NULL) {
		printf("failed to create context\n");
		return -1;
	}

	gl.surface = eglCreateWindowSurface(gl.display, gl.config, wayland.egl_window, NULL);
	if (gl.surface == EGL_NO_SURFACE) {
		printf("failed to create egl surface\n");
		return -1;
	}

	/* connect the context to the surface */
	eglMakeCurrent(gl.display, gl.surface, gl.surface, gl.context);


	gl.vertex_shader = glCreateShader(GL_VERTEX_SHADER);

	glShaderSource(gl.vertex_shader, 1, &vertex_shader_source, NULL);
	glCompileShader(gl.vertex_shader);

	glGetShaderiv(gl.vertex_shader, GL_COMPILE_STATUS, &ret);
	if (!ret) {
		char *log;

		printf("vertex shader compilation failed!:\n");
		glGetShaderiv(gl.vertex_shader, GL_INFO_LOG_LENGTH, &ret);
		if (ret > 1) {
			log = malloc(ret);
			glGetShaderInfoLog(gl.vertex_shader, ret, NULL, log);
			printf("%s", log);
		}

		return -1;
	}

	gl.fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);

	glShaderSource(gl.fragment_shader, 1, &fragment_shader_source, NULL);
	glCompileShader(gl.fragment_shader);

	glGetShaderiv(gl.fragment_shader, GL_COMPILE_STATUS, &ret);
	if (!ret) {
		char *log;

		printf("fragment shader compilation failed!:\n");
		glGetShaderiv(gl.fragment_shader, GL_INFO_LOG_LENGTH, &ret);

		if (ret > 1) {
			log = malloc(ret);
			glGetShaderInfoLog(gl.fragment_shader, ret, NULL, log);
			printf("%s", log);
		}

		return -1;
	}

	gl.program = glCreateProgram();

	glBindAttribLocation(gl.program, 0, "position");
	glBindAttribLocation(gl.program, 1, "color");

	glAttachShader(gl.program, gl.vertex_shader);
	glAttachShader(gl.program, gl.fragment_shader);

	glLinkProgram(gl.program);

	glGetProgramiv(gl.program, GL_LINK_STATUS, &ret);
	if (!ret) {
		printf("program linking failed!:\n");
		glGetProgramiv(gl.program, GL_INFO_LOG_LENGTH, &ret);

		if (ret > 1) {
			char log[1000];
			glGetProgramInfoLog(gl.program, ret, NULL, log);
			printf("%s", log);
		}

		return -1;
	}

	glUseProgram(gl.program);

	gl.rotation_uniform = glGetUniformLocation(gl.program, "rotation");

	return 0;
}


static void exit_gl(void)
{
        glDeleteProgram(gl.program);
        glDeleteShader(gl.fragment_shader);
        glDeleteShader(gl.vertex_shader);
        eglDestroySurface(gl.display, gl.surface);
        eglDestroyContext(gl.display, gl.context);
        eglMakeCurrent(gl.display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        eglTerminate(gl.display);
        return;
}

void cleanup_wlegl(void)
{
	exit_gl();

	wl_compositor_destroy(wayland.compositor);
	wl_registry_destroy(wayland.registry);
	wl_display_flush(wayland.display);
	wl_display_disconnect(wayland.display);

	if(outFile)
	  fclose(outFile);

	printf("Cleanup of GL and wayland completed\n");
	return;
}

static void
redraw(void )
{
	uint32_t time;
	static const GLfloat verts[4][2] = {
		{ -0.5, -0.5 },
		{  0.5, -0.5 },
		{ -0.5, 0.5  },
		{  0.5, 0.5  }
	};
	static const GLfloat colors[4][3] = {
		{ 1, 0, 0 },
		{ 0, 1, 0 },
		{ 0, 0, 1 },
		{ 1, 1, 0 }
	};
	GLfloat angle;
	GLfloat rotation[4][4] = {
		{ 1, 0, 0, 0 },
		{ 0, 1, 0, 0 },
		{ 0, 0, 1, 0 },
		{ 0, 0, 0, 1 }
	};
	struct wl_region *region;
	EGLint rect[4];
	EGLint buffer_age = 0;
	static const uint32_t speed_div = 5;
	struct timeval tv;

	gettimeofday(&tv, NULL);
	time = tv.tv_sec * 1000 + tv.tv_usec / 1000;

	angle = (time / speed_div) % 360 * M_PI / 180.0;
	rotation[0][0] =  cos(angle);
	rotation[0][2] =  sin(angle);
	rotation[2][0] = -sin(angle);
	rotation[2][2] =  cos(angle);

	glViewport(0, 0, width, height);

	glUniformMatrix4fv(gl.rotation_uniform, 1, GL_FALSE,
			   (GLfloat *) rotation);

	glClearColor(0.0, 0.0, 0.0, 0.5);
	glClear(GL_COLOR_BUFFER_BIT);

	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, verts);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, colors);
	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);

	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	glDisableVertexAttribArray(0);
	glDisableVertexAttribArray(1);

	eglSwapBuffers(gl.display, gl.surface);

	if(readPix && outFile ) {
		GLint eReadFormat,eReadType;
                char buf[WIDTH*HEIGHT*4];
                int glErr;
                glGetIntegerv(GL_IMPLEMENTATION_COLOR_READ_TYPE,&eReadType);
                glGetIntegerv(GL_IMPLEMENTATION_COLOR_READ_FORMAT,&eReadFormat);
                glReadPixels(0,0,WIDTH,HEIGHT,eReadFormat,eReadType,buf);
                glErr = glGetError();
                if(glErr)
                {
                        printf("glGetError %d encountered\n", glErr);
                }
                fwrite(buf, sizeof(char), 256*256*4, outFile);
	}

}

void print_usage()
{
	printf("Usage : simple-egl <options>\n");
	printf("\t-h : Help\n");
	printf("\t-r <filename>: ReadPixels. Dump client rendered pixles into file\n");
	printf("\t-n <number> (optional): Number of frames to render\n");
	printf("\t-Example: simple-egl-readpixels -n 60 -r output.rgb\n");
}

int signalhandler(int signum)
{
	switch(signum) {
	case SIGINT:
        case SIGTERM:
                /* Allow the pending page flip requests to be completed before
                 * the teardown sequence */
                sleep(1);
                printf("Handling signal number = %d\n", signum);
		cleanup_wlegl();
		break;
	default:
		printf("Unknown signal\n");
		break;
	}
	exit(1);
}

int main(int argc, char *argv[])
{
	uint32_t i = 0;
	int ret;
	int opt;
	int frame_count = -1;

	signal(SIGINT, signalhandler);
	signal(SIGTERM, signalhandler);

	while ((opt = getopt(argc, argv, "hr:n:")) != -1) {
		switch(opt) {
		case 'h':
			print_usage();
			return 0;

		case 'n':
			frame_count = atoi(optarg);
			break;

		case 'r':
			readPix = 1;
			outFile = fopen(optarg, "w+");
			if(outFile == NULL)
			{
				printf("not able to open file %s\n", argv[optind]);
			}
			break;
		default:
			printf("Undefined option %s\n", argv[optind]);
			print_usage();
			return -1;
		}
	}

	ret = init_wayland();
	if (ret) {
		printf("failed to initialize wayland\n");
		return ret;
	}

	ret = init_gl();
	if (ret) {
		printf("failed to initialize EGL\n");
		return ret;
	}

	/* clear the color buffer */
	glClearColor(0.5, 0.5, 0.5, 1.0);
	glClear(GL_COLOR_BUFFER_BIT);
	eglSwapBuffers(gl.display, gl.surface);

	while (frame_count != 0) {

		redraw();

		if(frame_count >= 0)
			frame_count--;
	}

	cleanup_wlegl();
	printf("\n Exiting simple-egl-readpixels \n");

	return ret;
}
