/*
 * Copyright (c) 2013, Texas Instruments
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * - Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *
 * - Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 *
 * - Neither the name of the Texas Instruments nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION
 * OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * @File	drm_clone.c
 * @Authour	alaganraj <alaganraj.s@ti.com>
 * @Brief	drm based clone mode test program
 *
 * This test app will demonstrate the overlay window and clone mode
 * capability of omapdrm.
 * 
 * Clone Mode:
 * Same image will be displayed on lcd and hdmi display.
 *
 * For overlay window, frame buffer is created & filled with pattern.
 * Free plane is identified and attached to lcd_crtc to draw overlay
 * window on lcd.
 *
 * For clone mode, app gets the connector, encoder and mode details
 * for lcd and hdmi displays using drm APIs.Another frame buffer is
 * created and filled with test pattern.To achieve clone mode, both 
 * the displays use the same frame buffer id. Call to drmModeSetCrtc()
 * will draw pattern on display along with overlay window.
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

struct device {
	uint32_t fd;

	drmModeRes *res;
	drmModePlaneRes *plane_res;
	drmModeCrtcPtr crtc;

	drmModeConnector *lcd_con;
	drmModeEncoder *lcd_enc;
	drmModeModeInfo lcd_mode;

	drmModeConnector *hdmi_con;
	drmModeEncoder *hdmi_enc;		
	drmModeModeInfo hdmi_mode;

	struct kms_driver *kms;
	uint32_t p_xres;
	uint32_t p_yres;
	uint32_t fb_id1;
	uint32_t crtc_x;
	uint32_t crtc_y;
	uint32_t crtc_w;
	uint32_t crtc_h;
	uint32_t plane_id;

	uint32_t xres;
	uint32_t yres;
	uint32_t fb_id2;

	uint32_t hdmi_xres;
	uint32_t hdmi_yres;
};

/**
 *****************************************************************************
 * @brief:  This function gets lcd and hdmi connectors id
 *
 * @param:  dev  device pointer
 *****************************************************************************
*/
static void get_drm_connector(struct device *dev)
{
	uint32_t i;
	drmModeConnector *con;		

	for (i = 0; i < dev->res->count_connectors; ++i) {
		con = drmModeGetConnector(dev->fd, dev->res->connectors[i]);
		if (!con)
			continue;

		if (con->connection == DRM_MODE_CONNECTED &&
		    con->count_modes > 0) {
			if (con->connector_type == DRM_MODE_CONNECTOR_HDMIA) {
				dev->hdmi_con = con;
				printf("hdmi connector id = %d\n",
					dev->hdmi_con->connector_id);
				if (dev->lcd_con)
					break;
				else
					continue;
			} else {
				dev->lcd_con = con;
				printf("lcd connector id = %d\n",
					dev->lcd_con->connector_id);
				if (dev->hdmi_con)
					break;
				else
					continue;
			}
		}
		drmModeFreeConnector(con);
	}

	if (i == dev->res->count_connectors) {
		error("No active lcd or hdmi connector found!\n");
		exit(0);
	}
}

/**
 *****************************************************************************
 * @brief:  This function gets lcd and hdmi encoders id
 *
 * @param:  dev  device pointer
 *****************************************************************************
*/
static void get_drm_encoder(struct device *dev)
{
	uint32_t i;
	drmModeEncoder *enc;		

	for (i = 0; i < dev->res->count_encoders; ++i) {
		enc = drmModeGetEncoder(dev->fd, dev->res->encoders[i]);
		if (!enc)
			continue;

		if (enc->encoder_id == dev->lcd_con->encoder_id) {
			dev->lcd_enc = enc;
			printf("lcd encoder id = %d\n",
				dev->lcd_enc->encoder_id);
			if(dev->hdmi_enc)
				break;
			else
				continue;
		} else if (enc->encoder_id == dev->hdmi_con->encoder_id) {
			dev->hdmi_enc = enc;
			printf("hdmi encoder id = %d\n",
				dev->hdmi_enc->encoder_id);
			if(dev->lcd_enc)
				break;
			else
				continue;
		}
		drmModeFreeEncoder(enc);
	}

	if (i == dev->res->count_encoders) {
		error("No active lcd or hdmi encoder found!\n");
		exit(0);
	}
}

/**
 *****************************************************************************
 * @brief:  This function checks and gets the requested mode for lcd 
 *
 * @param:  dev  device pointer
 *****************************************************************************
*/
static void get_lcd_mode(struct device *dev)
{
	uint32_t i;

	for (i = 0; i < dev->lcd_con->count_modes; ++i) {
		dev->lcd_mode = dev->lcd_con->modes[i];

		if ((dev->lcd_mode.hdisplay == dev->xres) &&
		    (dev->lcd_mode.vdisplay == dev->yres))
			break;
	}

	if (i == dev->lcd_con->count_modes) {
		error("requested mode for lcd not found!\n");
		exit(0);
	}
}

/**
 *****************************************************************************
 * @brief:  This function checks and gets the requested mode for hdmi
 *
 * @param:  dev  device pointer
 *****************************************************************************
*/
static void get_hdmi_mode(struct device *dev)
{
	uint32_t i;

	for (i = 0; i < dev->hdmi_con->count_modes; ++i) {
		dev->hdmi_mode = dev->hdmi_con->modes[i];

		if ((dev->hdmi_mode.hdisplay == dev->hdmi_xres) &&
		    (dev->hdmi_mode.vdisplay == dev->hdmi_yres))
			break;
	}

	if (i == dev->hdmi_con->count_modes) {
		error("requested mode for hdmi not found!\n");
		exit(0);
	}
}

/**
 *****************************************************************************
 * @brief:  This function gets free plane, which will be used as overlay 
 * 	    window
 *
 * @param:  dev  device pointer
 *****************************************************************************
*/
static void get_plane_id(struct device *dev)
{
	drmModePlane *ovr;
	uint32_t i;

	/* Possible crtc */
	uint32_t pipe = 0;

	for (i = 0; i < dev->plane_res->count_planes && !dev->plane_id; i++) {
		ovr = drmModeGetPlane(dev->fd, dev->plane_res->planes[i]);
		if (!ovr)
			continue;

		/* Check possible crtcs, it already attached to some crtc? */
		if ((ovr->possible_crtcs & (1 << pipe)) && !ovr->crtc_id) {
			dev->plane_id = ovr->plane_id;
			break;
		}
		drmModeFreePlane(ovr);
	}

	if (!dev->plane_id) {
		error("failed to find plane!\n");
		exit(0);
	}
}

/**
 *****************************************************************************
 * @brief:  This function creates test buffer with pattern filled. Get frame 
 * 	    buffer id for test buffer using drm API. Attach the plane to 
 * 	    lcd_crtc to draw overlay window on lcd.
 *
 * @param:  dev  device pointer
 *****************************************************************************
*/
static void set_plane(struct device *dev)
{
	uint32_t flags = 0;
	uint32_t handles1[4], pitches1[4], offsets1[4];
	struct kms_bo *plane_bo1;

	/* Size to be displayed on lcd */
	dev->crtc_w = dev->p_xres;
	dev->crtc_h = dev->p_yres;

	plane_bo1 = create_test_buffer(dev->kms, DRM_FORMAT_XRGB8888, 
				       dev->p_xres, dev->p_yres,
				       handles1, pitches1, offsets1,
				       PATTERN_TILES);
	if (!plane_bo1) {
		error("create test buffer for plane failed\n");
		exit(0);
	}

	if (drmModeAddFB2(dev->fd, dev->p_xres, dev->p_yres,
			  DRM_FORMAT_XRGB8888, handles1, pitches1,
			  offsets1, &dev->fb_id1, flags)) {
		error("failed to add fb for plane\n");
		exit(0);
	}

#if 1   /* Overlay window on lcd */

	/* Note src coords (last 4 args) are in Q16 format */
	if (drmModeSetPlane(dev->fd, dev->plane_id, dev->lcd_enc->crtc_id, 
			    dev->fb_id1, flags, dev->crtc_x, dev->crtc_y,
			    dev->crtc_w, dev->crtc_h, 0, 0, 
			    dev->p_xres << 16, dev->p_yres << 16)) {
		error("failed to enable plane\n");
		exit(0);
	}

#else   /* Overlay window on hdmi Display */

	if (drmModeSetPlane(dev->fd, dev->plane_id, dev->hdmi_enc->crtc_id,
			    dev->fb_id1, flags, dev->crtc_x, dev->crtc_y,
			    dev->crtc_w, dev->crtc_h, 0, 0,
			    dev->p_xres << 16, dev->p_yres << 16)) {
		error("failed to enable plane\n");
		exit(0);
	}
#endif
}

/**
 *****************************************************************************
 * @brief:  This function creates test buffer with pattern filled. Get frame 
 * 	    buffer id for test buffer using drm API. Call to drmModeSetCrtc()
 *	    will draw pattern on lcd along with overlay window, which is 
 * 	    attached to it.
 *
 * @param:  dev  device pointer
 *****************************************************************************
*/
static void set_crtc(struct device *dev)
{
	uint32_t flags = 0;
	uint32_t handles2[4], pitches2[4], offsets2[4];
	struct kms_bo *plane_bo2;

	plane_bo2 = create_test_buffer(dev->kms, DRM_FORMAT_XRGB8888,
				       dev->xres, dev->yres,
				       handles2, pitches2, offsets2,
				       PATTERN_SMPTE);
	if (!plane_bo2) {
		error("create test buffer for lcd failed\n");
		exit(0);
	}

	if (drmModeAddFB2(dev->fd, dev->xres, dev->yres,
			  DRM_FORMAT_XRGB8888, handles2, pitches2,
			  offsets2, &dev->fb_id2, flags)) {
		error("failed to add fb for crtc\n");
		exit(0);
	}

	if (drmModeSetCrtc(dev->fd, dev->lcd_enc->crtc_id, dev->fb_id2,
			   0, 0, &dev->lcd_con->connector_id, 1,
			   &dev->lcd_mode)) {
		error("failed to set mode for lcd!\n");
		exit(0);
	}
}

/**
 *****************************************************************************
 * @brief:  This function achieves clone mode by using same framebuffer id, 
 *	    which is created for lcd.This will draw same test pattern on hdmi
 *
 * @param:  dev  device pointer
 *****************************************************************************
*/
static void set_clone_mode(struct device *dev)
{
	uint32_t flags = 0;


	if (drmModeSetCrtc(dev->fd, dev->hdmi_enc->crtc_id, dev->fb_id2,
			   0, 0, &dev->hdmi_con->connector_id, 1,
			   &dev->hdmi_mode)) {
		error("failed to set mode for hdmi!\n");
		exit(0);
	}
}

/**
 *****************************************************************************
 * @brief:  This function extracts the lcd mode from command line
 *
 * @param:  dev  device pointer
 * @param:  p  	 optarg pointer
 *
 * @return: 0 on success
 *****************************************************************************
*/
static int parse_lcd(struct device *dev, const char *p)
{
	char *end;

	dev->xres = strtoul(p, &end, 10);
	if (*end != 'x')
		return -1;

	p = end + 1;
	dev->yres = strtoul(p, &end, 10);

	return 0;
}

/**
 *****************************************************************************
 * @brief:  This function extracts the hdmi mode from command line
 *
 * @param:  dev  device pointer
 * @param:  p  	 optarg pointer
 *
 * @return: 0 on success
 *****************************************************************************
*/
static int parse_hdmi(struct device *dev, const char *p)
{
	char *end;

	dev->hdmi_xres = strtoul(p, &end, 10);
	if (*end != 'x')
		return -1;

	p = end + 1;
	dev->hdmi_yres = strtoul(p, &end, 10);

	return 0;
}

/**
 *****************************************************************************
 * @brief:  This function extracts the overlay window mode & position from
 *	    command line
 *
 * @param:  dev  device pointer
 * @param:  p  	 optarg pointer
 *
 * @return: 0 on success
 *****************************************************************************
*/
static int parse_plane(struct device *dev, const char *p)
{
	char *end;

	dev->p_xres = strtoul(p, &end, 10);
	if (*end != 'x')
		return -1;

	p = end + 1;
	dev->p_yres = strtoul(p, &end, 10);
	if (*end != ':')
		return -1;

	p = end + 1;
	dev->crtc_x = strtoul(p, &end, 10);
	if (*end != '+')
		return -1;

	p = end + 1;
	dev->crtc_y = strtoul(p, &end, 10);

	return 0;
}

static void usage()
{
	printf("Usage:\n");
	printf("-l wxh\t\t- set lcd mode\n");
	printf("-h wxh\t\t- set hdmi mode\n");
	printf("-p wxh:x+y\t- set plane mode & ");
	printf("overlay window position value (x,y)\n\nex:\n");
	printf("# drmclone -l 800x480 -p 320x240:0+0 -h 640x480\n\n");
	exit(0);
}

static char optstr[] = "p:l:h:";

int main(int argc, char *argv[])
{
	struct device dev;

	uint32_t c, ret;
	uint32_t oid;

	memset(&dev, 0, sizeof dev);

	while ((c = getopt(argc, argv, optstr)) != -1) {

		switch (c) {
		case 'p':
			if (parse_plane(&dev, optarg) < 0)
				usage();
			break;
		case 'l':
			if (parse_lcd(&dev, optarg) < 0)
				usage();
			break;
		case 'h':
			if (parse_hdmi(&dev, optarg) < 0)
				usage();
			break;
		default:
			usage();
			break;
		}
	}

	if (argc < 7)
		usage();

	/* Open default dri device */
	dev.fd = open("/dev/dri/card0", O_RDWR | O_CLOEXEC);
	if (dev.fd < 0) {
		error("couldn't open /dev/dri/card0\n");
		exit(0);
	} else {
		printf("/dev/dri/card0 open success!\n");
	}

	/* Get drm resources */
	dev.res = drmModeGetResources(dev.fd);
	if (!dev.res) {
		error("drmModeGetResources failed\n");
		exit(0);
	}

	/* Get plane resource */
	dev.plane_res =  drmModeGetPlaneResources(dev.fd);
	if (!dev.plane_res) {
		error("drmModeGetPlaneResources failed\n");
		exit(0);
	}

	/* Get original mode and framebuffer id */
	dev.crtc = drmModeGetCrtc(dev.fd, dev.res->crtcs[0]);
	if (!dev.crtc) {
		error("drmModeGetCrtc failed\n");
		exit(0);
	}
	oid = dev.crtc->buffer_id;

	get_drm_connector(&dev);
	get_drm_encoder(&dev);
	get_lcd_mode(&dev);
	get_hdmi_mode(&dev);
	get_plane_id(&dev);

	/* Create kms driver */
	ret = kms_create(dev.fd, &dev.kms);
	if (ret) {
		error("failed to create kms driver\n");
		exit(0);
	}

	/* To achieve overlay, set plane before crtc */
	set_plane(&dev);
	set_crtc(&dev);
	set_clone_mode(&dev);

	drmModeDirtyFB(dev.fd, dev.fb_id2, 0, 0);

	getchar();

	/* Undo the drm setup in the correct sequence */
	drmModeSetCrtc(dev.fd, dev.lcd_enc->crtc_id, oid, 
		       0, 0, &dev.lcd_con->connector_id,
		       1, &(dev.crtc->mode));

	drmModeRmFB(dev.fd, dev.fb_id2);
	drmModeRmFB(dev.fd, dev.fb_id1);
	drmModeFreePlaneResources(dev.plane_res);
	drmModeFreeEncoder(dev.hdmi_enc);
	drmModeFreeEncoder(dev.lcd_enc);
	drmModeFreeConnector(dev.hdmi_con);
	drmModeFreeConnector(dev.lcd_con);
	drmModeFreeCrtc(dev.crtc);
	drmModeFreeResources(dev.res);
	close(dev.fd);

	return 0;
}
