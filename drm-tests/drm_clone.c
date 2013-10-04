/*
 * DRM based clone mode test program
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
	drmModeConnector *lcd_connector;	/* LCD connector pointer */
	drmModeEncoder *encoder, *lcd_encoder;	/* LCD encoder pointer */
	drmModeModeInfo lcd_mode;		/* LCD mode in use */
	drmModeCrtcPtr crtc;			/* crtc pointer */
	drmModePlaneRes *plane_resources;	/* plane resource pointer */
	drmModePlane *ovr;			/* plane pointer */

	/* HDMI Display variables */
	drmModeConnector *hdmi_connector;	/* HDMI connector pointer */
	drmModeEncoder *hdmi_encoder;		/* HDMI encoder pointer */
	drmModeModeInfo hdmi_mode;		/* HDMI mode in use */

	struct kms_driver *kms;
	/* Plane variables */
	uint32_t p_xres, p_yres;		/* plane mode: eg.320x240 */
	uint32_t fb_id1;			/* plane framebuffer id */
	uint32_t handles1[4], pitches1[4], offsets1[4]; /* we only use [0] */
	struct kms_bo *plane_bo1;
	uint32_t plane_id = 0;			/* plane id */
	unsigned int pipe = 0;			/* Possible crtc */
	uint32_t crtc_x, crtc_y, overlay_pos;	/* Overlay window position */
	uint32_t crtc_w, crtc_h;		/* size to be displayed on LCD */

	/* LCD CRTC variables */
	uint32_t xres, yres;			/* crtc mode: eg.800x480 */
	uint32_t fb_id2;			/* crtc framebuffer id */
	uint32_t handles2[4], pitches2[4], offsets2[4]; /* we only use [0] */
	struct kms_bo *plane_bo2;

	/* HDMI CRTC variables */
	uint32_t hdmi_xres, hdmi_yres;		/* hdmi mode: eg.640x480 */

	uint32_t flags = 0, i;
	int fd;					/* drm device handle */
	uint32_t oid;				/* old framebuffer id */

	if (argc == 8) {
		xres  = atoi(argv[1]);
		yres  = atoi(argv[2]);
		p_xres  = atoi(argv[3]);
		p_yres  = atoi(argv[4]);
		overlay_pos  = atoi(argv[5]);
		hdmi_xres  = atoi(argv[6]);
		hdmi_yres  = atoi(argv[7]);
	} else {
		printf("usage:\n#./drm_clone lcd_w lcd_h plane_w plane_h ");
		printf("overlay_pos hdmi_w hdmi_h\n");
		printf("\neg:\nLCD crtc mode=800x480\n");
		printf("plane mode=320x240\n");
		printf("HDMI crtc mode=640x480\n");
		printf("overlay plane window position:\n");
		printf("1 - top left corner\n");
		printf("2 - top right corner\n");
		printf("3 - bottom right  corner\n");
		printf("4 - bottom left corner\n");
		printf("5 - center\n");
		printf("\n#./drm_clone 800 480 320 240 1 640 480\n\n");
		exit(0);
	}

	switch (overlay_pos) {
	case 1:
		/* overlay plane window @ top left corner */
		crtc_x = 0;
		crtc_y = 0;
		break;
	case 2:
		/* overlay plane window @ top right corner */
		crtc_x = xres-p_xres;
		crtc_y = 0;
		break;
	case 3:
		/* overlay plane window @ bottom right corner */
		crtc_x = xres-p_xres;
		crtc_y = yres-p_yres;
		break;
	case 4:
		/* overlay plane window @ bottom left corner */
		crtc_x = 0;
		crtc_y = yres-p_yres;
		break;
	case 5:
	default:
		/* overlay plane window @ center */
		crtc_x = (xres/3);
		crtc_y = (yres/3);
		break;
	}

	crtc_w = p_xres;
	crtc_h = p_yres;

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

		if (connector->connection == DRM_MODE_CONNECTED && connector->count_modes > 0) {
			if (connector->connector_type == DRM_MODE_CONNECTOR_HDMIA) {
				hdmi_connector = connector;
				printf("HDMI connector id = %d\n", hdmi_connector->connector_id);
				break;
			} else {
				lcd_connector = connector;
				printf("LCD connector id = %d\n", lcd_connector->connector_id);
				continue;
			}
		}
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

		if (encoder->encoder_id == lcd_connector->encoder_id) {
			lcd_encoder = encoder;
			printf("LCD encoder id = %d\n", lcd_encoder->encoder_id);
			continue;
		} else if (encoder->encoder_id == hdmi_connector->encoder_id) {
			hdmi_encoder = encoder;
			printf("HDMI encoder id = %d\n", hdmi_encoder->encoder_id);
			break;
		}
		drmModeFreeEncoder(encoder);
	}

	if (i == resources->count_encoders) {
		printf("No active encoder found!\n");
		exit(0);
	}

	/* check requested mode for LCD */
	for (i = 0; i < lcd_connector->count_modes; ++i) {
		lcd_mode = lcd_connector->modes[i];
		if ((lcd_mode.hdisplay == xres) && (lcd_mode.vdisplay == yres))
			break;
	}

	if (i == lcd_connector->count_modes) {
		printf("requested mode for LCD not found!");
		exit(0);
	}

	/* check requested mode for HDMI Display */
	for (i = 0; i < hdmi_connector->count_modes; ++i) {
		hdmi_mode = hdmi_connector->modes[i];
		if ((hdmi_mode.hdisplay == hdmi_xres) && (hdmi_mode.vdisplay == hdmi_yres))
			break;
	}

	if (i == hdmi_connector->count_modes) {
		printf("requested mode for HDMI not found!");
		exit(0);
	}

	/* create kms driver */
	i = kms_create(fd, &kms);
	if (i) {
		printf("failed to create kms driver\n");
		exit(0);
	}

	/* get plane resource */
	plane_resources =  drmModeGetPlaneResources(fd);
	if (!plane_resources) {
		printf("drmModeGetPlaneResources failed\n");
		exit(0);
	}

	/* get plane */
	for (i = 0; i < plane_resources->count_planes && !plane_id; i++) {
		ovr = drmModeGetPlane(fd, plane_resources->planes[i]);
		if (!ovr)
			continue;

		if ((ovr->possible_crtcs & (1 << pipe)) && !ovr->crtc_id) {
			plane_id = ovr->plane_id;
			break;
		}
		drmModeFreePlane(ovr);
	}

	if (!plane_id) {
		printf("failed to find plane!\n");
		exit(0);
	}

	/* Plane */
	/* create test buffer for plane */
	plane_bo1 = create_test_buffer(kms, DRM_FORMAT_XRGB8888, p_xres,
				       p_yres, handles1, pitches1, offsets1,
				       PATTERN_TILES);
	if (!plane_bo1) {
		printf("create test buffer for plane failed\n");
		exit(0);
	}

	/* just use single plane format for now.. */
	if (drmModeAddFB2(fd, p_xres, p_yres, DRM_FORMAT_XRGB8888, handles1,
			  pitches1, offsets1, &fb_id1, flags)) {
		printf("failed to add fb for plane\n");
		exit(0);
	}

#if 1   /* overlay window on LCD */

	/* To achieve overlay, set plane before crtc */
	/* note src coords (last 4 args) are in Q16 format */
	if (drmModeSetPlane(fd, plane_id, lcd_encoder->crtc_id, fb_id1, flags,
			    crtc_x, crtc_y, crtc_w, crtc_h, 0, 0,
			    p_xres << 16, p_yres << 16)) {
		printf("failed to enable plane\n");
		exit(0);
	}

#else   /* overlay window on HDMI Display */

	/* To achieve overlay, set plane before crtc */
	/* note src coords (last 4 args) are in Q16 format */
	if (drmModeSetPlane(fd, plane_id, hdmi_encoder->crtc_id, fb_id1, flags,
			    crtc_x, crtc_y, crtc_w, crtc_h, 0, 0,
			    p_xres << 16, p_yres << 16)) {
		printf("failed to enable plane\n");
		exit(0);
	}
#endif

	/* LCD CRTC */
	/* create test buffer for LCD */
	plane_bo2 = create_test_buffer(kms, DRM_FORMAT_XRGB8888, xres, yres,
				       handles2, pitches2, offsets2,
				       PATTERN_SMPTE);
	if (!plane_bo2) {
		printf("create test buffer for LCD failed\n");
		exit(0);
	}

	/* just use single plane format for now.. */
	if (drmModeAddFB2(fd, xres, yres, DRM_FORMAT_XRGB8888, handles2,
			  pitches2, offsets2, &fb_id2, flags)) {
		printf("failed to add fb for crtc\n");
		exit(0);
	}

	/* set crtc, this will draw test pattern on LCD screen */
	if (drmModeSetCrtc(fd, lcd_encoder->crtc_id, fb_id2, 0, 0,
			   &lcd_connector->connector_id, 1, &lcd_mode)) {
		printf("failed to set mode for LCD!");
		exit(0);
	}

	/* CLONE MODE */
	/* HDMI CRTC */
	/* set crtc, this will draw test pattern on HDMI screen */
	if (drmModeSetCrtc(fd, hdmi_encoder->crtc_id, fb_id2, 0, 0,
			   &hdmi_connector->connector_id, 1, &hdmi_mode)) {
		printf("failed to set mode for HDMI!");
		exit(0);
	}

	/* XXX: Actually check if this is needed */
	drmModeDirtyFB(fd, fb_id2, 0, 0);

	/* wait for enter key */
	getchar();

	/* undo the drm setup in the correct sequence */
	drmModeSetCrtc(fd, encoder->crtc_id, oid, 0, 0, &connector->connector_id, 1, &(crtc->mode));
	drmModeRmFB(fd, fb_id2);
	drmModeRmFB(fd, fb_id1);
	drmModeFreePlane(ovr);
	drmModeFreePlaneResources(plane_resources);
	drmModeFreeEncoder(hdmi_encoder);
	drmModeFreeEncoder(lcd_encoder);
	drmModeFreeConnector(hdmi_connector);
	drmModeFreeConnector(lcd_connector);
	drmModeFreeCrtc(crtc);
	drmModeFreeResources(resources);
	close(fd);

	return 0;
}
