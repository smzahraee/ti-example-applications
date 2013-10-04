/*
 * DRM based z-order & alpha blending test program with 2 planes
 * Copyright (C) 2013 Texas Instruments
 * Authour: alaganraj <alaganraj.s@ti.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published by
 * the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <xf86drm.h>
#include <xf86drmMode.h>
#include <drm_fourcc.h>
#include <libkms.h>
#include "buffers.h"

int main(int argc, char *argv[])
{
	/* DRM system variables */
	drmModeRes *resources;			/* resource pointer */
	drmModeConnector *connector;		/* connector pointer */
	drmModeEncoder *encoder;		/* encoder pointer */
	drmModeModeInfo mode;			/* video mode in use */
	drmModeCrtcPtr crtc;			/* crtc pointer */

	struct kms_driver *kms;
	/* Plane1 variables */
	uint32_t p1_xres, p1_yres;		/* plane1 mode: eg.320x240 */
	uint32_t fb_id1;			/* plane1 framebuffer id */
	uint32_t handles1[4], pitches1[4], offsets1[4]; /* we only use [0] */
	struct kms_bo *plane_bo1;
	/* plane id, z-order, alpha blend variables */
	uint32_t plane1_id, plane1_z_val, plane1_glo_alp_val, plane1_pre_mul_alp_val;

	/* Plane2 variables */
	uint32_t p2_xres, p2_yres;		/* plane2 mode: eg.160x120 */
	uint32_t fb_id2;			/* plane2 framebuffer id */
	uint32_t handles2[4], pitches2[4], offsets2[4];	/* we only use [0] */
	struct kms_bo *plane_bo2;
	/* plane id, z-order, alpha blend variables */
	uint32_t plane2_id, plane2_z_val, plane2_glo_alp_val, plane2_pre_mul_alp_val;

	unsigned int pipe = 0;			/* possible crtc */
	uint32_t crtc_x, crtc_y;		/* overlay window position */
	uint32_t crtc_w, crtc_h;		/* size to be displayed on LCD */

	/* CRTC variables */
	uint32_t xres, yres;			/* crtc mode: eg.800x480 */
	uint32_t fb_id3;			/* crtc framebuffer id */
	uint32_t handles3[4], pitches3[4], offsets3[4]; /* we only use [0] */
	struct kms_bo *plane_bo3;

	uint32_t flags = 0, i;
	int fd;					/* drm device handle */
	uint32_t oid;				/* old framebuffer id */

	if (argc == 11) {
		xres  = atoi(argv[1]);
		yres  = atoi(argv[2]);
		plane1_id = atoi(argv[3]);
		plane1_z_val  = atoi(argv[4]);
		plane1_glo_alp_val  = atoi(argv[5]);
		plane1_pre_mul_alp_val  = atoi(argv[6]);
		plane2_id = atoi(argv[7]);
		plane2_z_val  = atoi(argv[8]);
		plane2_glo_alp_val  = atoi(argv[9]);
		plane2_pre_mul_alp_val  = atoi(argv[10]);
	} else {
		printf("usage:\n#./drm_z_alpha <crtc_w> <crtc_h> <plane1_id> <z_val> <glo_alpha> ");
		printf("<pre_mul_alpha> <plane2_id> <z_val> <glo_alpha> <pre_mul_alpha>\n");
		printf("\neg:\ncrtc mode = 800x480\n");
		printf("\nrun modetest app to find plane ids\n");
		printf("\nz-order value between 0 to 3\n\t- lowest value for bottom\n");
		printf("\t- highest value for top\n");
		printf("\nglobal alpha value between 0 to 255\n\t0   - fully transparent\n");
		printf("\t127 - semi transparent\n\t255 - fully opaque\n");
		printf("\npre multipled alpha value\n\t0 - source is premultiply with alpha\n");
		printf("\t1 - source is not premultiply with alpha\n");
		printf("\n#./drm_z_alpha 800 480 15 1 255 1 16 2 255 1\n\n");
		exit(0);
	}

	/* plane modes are hard coded to avoid too many command line arguments */
	/* plane1 mode */
	p1_xres = 320;
	p1_yres = 240;

	/* plane2 mode */
	p2_xres = 160;
	p2_yres = 120;

	/* open default dri device */
	fd = open("/dev/dri/card0", O_RDWR | O_CLOEXEC);
	if (fd < 0) {
		printf("couldn't open /dev/dri/card0\n");
		exit(0);
	} else {
		printf("/dev/dri/card0 open success!\n");
	}

	/* get drm resources */
	resources = drmModeGetResources(fd);
	if (!resources) {
		printf("drmModeGetResources failed\n");
		exit(0);
	}

	/* get original mode and framebuffer id */
	crtc = drmModeGetCrtc(fd, resources->crtcs[0]);
	if (!crtc) {
		printf("drmModeGetCrtc failed\n");
		exit(0);
	}
	oid = crtc->buffer_id;

	/* get drm connector */
	for (i = 0; i < resources->count_connectors; ++i) {
		connector = drmModeGetConnector(fd, resources->connectors[i]);
		if (!connector)
			continue;

		if (connector->connection == DRM_MODE_CONNECTED && connector->count_modes > 0)
			break;

		drmModeFreeConnector(connector);
	}

	if (i == resources->count_connectors) {
		printf("No active connector found!\n");
		exit(0);
	}

	/* acquire drm encoder */
	for (i = 0; i < resources->count_encoders; ++i) {
		encoder = drmModeGetEncoder(fd, resources->encoders[i]);
		if (!encoder)
			continue;

		if (encoder->encoder_id == connector->encoder_id)
			break;

		drmModeFreeEncoder(encoder);
	}

	if (i == resources->count_encoders) {
		printf("No active encoder found!\n");
		exit(0);
	}

	/* check for requested mode */
	for (i = 0; i < connector->count_modes; ++i) {
		mode = connector->modes[i];
		if ((mode.hdisplay == xres) && (mode.vdisplay == yres))
			break;
	}

	if (i == connector->count_modes) {
		printf("requested mode %dx%d not found!", xres, yres);
		exit(0);
	}

	/* create kms driver */
	i = kms_create(fd, &kms);
	if (i) {
		printf("failed to create kms driver\n");
		exit(0);
	}

	/* Set plane1 property */
	i = drmModeObjectSetProperty(fd, plane1_id, DRM_MODE_OBJECT_PLANE, 7, plane1_z_val);
	if (i < 0) {
		printf("set plane1_z_val property failed\n");
		exit(0);
	}

	i = drmModeObjectSetProperty(fd, plane1_id, DRM_MODE_OBJECT_PLANE, 8, plane1_glo_alp_val);
	if (i < 0) {
		printf("set plane1_glo_alp_val property failed\n");
		exit(0);
	}

	i = drmModeObjectSetProperty(fd, plane1_id, DRM_MODE_OBJECT_PLANE, 9, plane1_pre_mul_alp_val);
	if (i < 0) {
		printf("set plane1_pre_mul_alp_val property failed\n");
		exit(0);
	}

	/* Set plane2 property */
	i = drmModeObjectSetProperty(fd, plane2_id, DRM_MODE_OBJECT_PLANE, 7, plane2_z_val);
	if (i < 0) {
		printf("set plane2_z_val property failed\n");
		exit(0);
	}

	i = drmModeObjectSetProperty(fd, plane2_id, DRM_MODE_OBJECT_PLANE, 8, plane2_glo_alp_val);
	if (i < 0) {
		printf("set plane2_glo_alp_val property failed\n");
		exit(0);
	}

	i = drmModeObjectSetProperty(fd, plane2_id, DRM_MODE_OBJECT_PLANE, 9, plane2_pre_mul_alp_val);
	if (i < 0) {
		printf("set plane2_pre_mul_alp_val property failed\n");
		exit(0);
	}


	/* Planes */
	/* create test buffer for plane1 */
	plane_bo1 = create_test_buffer(kms, DRM_FORMAT_ARGB8888, p1_xres,
				       p1_yres, handles1, pitches1, offsets1,
				       PATTERN_TILES);
	if (!plane_bo1) {
		printf("create test buffer for plane1 failed\n");
		exit(0);
	}

	/* just use single plane format for now.. */
	if (drmModeAddFB2(fd, p1_xres, p1_yres, DRM_FORMAT_ARGB8888, handles1,
			  pitches1, offsets1, &fb_id1, flags)) {
		printf("failed to add fb for plane1\n");
		exit(0);
	}

	/* position of overlay window: top left corner, change value if
	   you want to place it somewhere
	*/
	crtc_x = 0;
	crtc_y = 0;

	crtc_w = p1_xres; /* plane width to be displayed on LCD */
	crtc_h = p1_yres; /* plane height to be displayed on LCD */

	/* To achieve overlay, set plane before crtc */
	/* note src coords (last 4 args) are in Q16 format */
	if (drmModeSetPlane(fd, plane1_id, encoder->crtc_id, fb_id1, flags,
			    crtc_x, crtc_y, crtc_w, crtc_h, 0, 0,
			    p1_xres << 16, p1_yres << 16)) {
		printf("failed to enable plane1\n");
		exit(0);
	}


	/* create test buffer for plane2 */
	plane_bo2 = create_test_buffer(kms, DRM_FORMAT_ARGB8888, p2_xres,
				       p2_yres, handles2, pitches2, offsets2,
				       PATTERN_SMPTE);
	if (!plane_bo2) {
		printf("create test buffer for plane2 failed\n");
		exit(0);
	}

	/* just use single plane format for now.. */
	if (drmModeAddFB2(fd, p2_xres, p2_yres, DRM_FORMAT_ARGB8888, handles2,
			  pitches2, offsets2, &fb_id2, flags)) {
		printf("failed to add fb for plane2\n");
		exit(0);
	}

	/* position of overlay window: top left corner, change value
	   if you want to place it somewhere
	*/
	crtc_x = 0;
	crtc_y = 0;

	crtc_w = p2_xres; /* plane width to be displayed on LCD */
	crtc_h = p2_yres; /* plane height to be displayed on LCD */

	/* To achieve overlay, set plane before crtc */
	/* note src coords (last 4 args) are in Q16 format */
	if (drmModeSetPlane(fd, plane2_id, encoder->crtc_id, fb_id2, flags,
			    crtc_x, crtc_y, crtc_w, crtc_h, 0, 0,
			    p2_xres << 16, p2_yres << 16)) {
		printf("failed to enable plane2\n");
		exit(0);
	}


	/* CRTC */
	/* create test buffer for plane */
	plane_bo3 = create_test_buffer(kms, DRM_FORMAT_XRGB8888, xres, yres,
				       handles3, pitches3, offsets3,
				       PATTERN_SMPTE);
	if (!plane_bo3) {
		printf("create test buffer for crtc failed\n");
		exit(0);
	}

	/* just use single plane format for now.. */
	if (drmModeAddFB2(fd, xres, yres, DRM_FORMAT_XRGB8888, handles3,
			  pitches3, offsets3, &fb_id3, flags)) {
		printf("failed to add fb for crtc\n");
		exit(0);
	}

	/* set crtc, this will draw test pattern on screen */
	if (drmModeSetCrtc(fd, encoder->crtc_id, fb_id3, 0, 0,
			   &connector->connector_id, 1, &mode)) {
		printf("failed to set mode!");
		exit(0);
	}

	/* XXX: Actually check if this is needed */
	drmModeDirtyFB(fd, fb_id3, 0, 0);

	/* wait for enter key */
	getchar();

	/* undo the drm setup in the correct sequence */
	drmModeSetCrtc(fd, encoder->crtc_id, oid, 0, 0, &connector->connector_id, 1, &(crtc->mode));
	drmModeRmFB(fd, fb_id3);
	drmModeRmFB(fd, fb_id2);
	drmModeRmFB(fd, fb_id1);
	drmModeFreeEncoder(encoder);
	drmModeFreeConnector(connector);
	drmModeFreeCrtc(crtc);
	drmModeFreeResources(resources);
	close(fd);

	return 0;
}
