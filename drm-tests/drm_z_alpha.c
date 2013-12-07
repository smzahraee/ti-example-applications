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
 * @File    drm_z_alpha.c
 * @Authour alaganraj <alaganraj.s@ti.com>
 * @Brief   drm based z-order & alpha blending test program
 * 
 * Z-order:
 * It determines, which overlay window appears on top of other.
 * 
 * Alpha Blend:
 * It determines transparency level of image as a result of both
 * global alpha & pre multiplied alpha value. 
 *
 * For two overlay windows, separate frame buffers are created & 
 * filled with pattern. Free planes are identified and attached to
 * lcd_crtc to draw overlay windows on lcd. 
 * 
 * This application gets the connector, encoder and mode details
 * for lcd using drm APIs. Another frame buffer created
 * and filled with test pattern.Call to drmModeSetCrtc() will draw 
 * pattern on display along with overlay windows.
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

struct plane {
	uint32_t id;
	uint32_t xres;
	uint32_t yres;
	uint32_t fb_id;
	uint32_t z_val;
	uint32_t glo_alp;
	uint32_t pre_mul_alp;
};

struct device {
	uint32_t fd;

	drmModeRes *res;
	drmModeCrtcPtr crtc;

	drmModeConnector *con;
	drmModeEncoder *enc;
	drmModeModeInfo mode;

	struct kms_driver *kms;
	struct plane p1; 
	struct plane p2; 
	uint32_t crtc_x;
	uint32_t crtc_y;
	uint32_t crtc_w;
	uint32_t crtc_h;

	uint32_t xres;
	uint32_t yres;
	uint32_t fb_id3;
};

/**
 *****************************************************************************
 * @brief:  This function gets lcd connector id
 *
 * @param:  dev  device pointer
 *****************************************************************************
*/
static void get_drm_connector(struct device *dev)
{
	uint32_t i;

	for (i = 0; i < dev->res->count_connectors; ++i) {
		dev->con = drmModeGetConnector(dev->fd,
					       dev->res->connectors[i]);
		if (!dev->con)
			continue;

		if (dev->con->connection == DRM_MODE_CONNECTED &&
		    dev->con->count_modes > 0)
			break;

		drmModeFreeConnector(dev->con);
	}

	if (i == dev->res->count_connectors) {
		error("No active connector found!\n");
		exit(0);
	}
}

/**
 *****************************************************************************
 * @brief:  This function gets lcd encoder id
 *
 * @param:  dev  device pointer
 *****************************************************************************
*/
static void get_drm_encoder(struct device *dev)
{
	uint32_t i;

	for (i = 0; i < dev->res->count_encoders; ++i) {
		dev->enc = drmModeGetEncoder(dev->fd,
					     dev->res->encoders[i]);
		if (!dev->enc)
			continue;

		if (dev->enc->encoder_id == dev->con->encoder_id)
			break;

		drmModeFreeEncoder(dev->enc);
	}

	if (i == dev->res->count_encoders) {
		error("No active encoder found!\n");
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

	for (i = 0; i < dev->con->count_modes; ++i) {
		dev->mode = dev->con->modes[i];

		if ((dev->mode.hdisplay == dev->xres) &&
		    (dev->mode.vdisplay == dev->yres))
			break;
	}

	if (i == dev->con->count_modes) {
		error("mode %dx%d not found!\n", dev->xres, dev->yres);
		exit(0);
	}
}

/**
 *****************************************************************************
 * @brief:  This function sets z-order value for plane 1 and plane 2.
 *	    Property id for z-order is 7, run modetest app to find.
 *
 * @param:  dev       device pointer
 * @param:  plane_id  plane id of overlay window
 *****************************************************************************
*/
static void set_z_order(struct device *dev, uint32_t plane_id)
{
	uint32_t i, val;

	if (plane_id == dev->p1.id)
		val = dev->p1.z_val;
	else
		val = dev->p2.z_val;

	i = drmModeObjectSetProperty(dev->fd, plane_id, 
				     DRM_MODE_OBJECT_PLANE, 7, val);
	if (i < 0) {
		error("set z-order for plane id %d failed\n", plane_id);
		exit(0);
	}
}

/**
 *****************************************************************************
 * @brief:  This function sets global alpha value for plane 1 and plane 2.
 *	     Property id for global alpha is 8, run modetest app to find.
 *
 * @param:  dev       device pointer
 * @param:  plane_id  plane id of overlay window
 *****************************************************************************
*/
static void set_global_alpha(struct device *dev, uint32_t plane_id)
{
	uint32_t i, val;

	if (plane_id == dev->p1.id)
		val = dev->p1.glo_alp;
	else
		val = dev->p2.glo_alp;

	i = drmModeObjectSetProperty(dev->fd, plane_id,
				     DRM_MODE_OBJECT_PLANE, 8, val);
	if (i < 0) {
		error("set global alpha for plane id %d failed\n", plane_id);
		exit(0);
	}
}

/**
 *****************************************************************************
 * @brief:  This function sets pre multiplied alpha value for plane 1 and
 * 	    plane 2. Property id for pre multiplied alpha is 9, run modetest
 *	    app to find.
 *
 * @param:  dev       device pointer
 * @param:  plane_id  plane id of overlay window
 *****************************************************************************
*/
static void set_pre_multiplied_alpha(struct device *dev, uint32_t plane_id)
{
	uint32_t i, val;

	if (plane_id == dev->p1.id)
		val = dev->p1.pre_mul_alp;
	else
		val = dev->p2.pre_mul_alp;

	i = drmModeObjectSetProperty(dev->fd, plane_id,
				     DRM_MODE_OBJECT_PLANE, 9, val);
	if (i < 0) {
		error("set pre multiply alpha for plane id %d failed\n",
		      plane_id);
		exit(0);
	}
}

/**
 *****************************************************************************
 * @brief:  This function creates test buffer with pattern filled. Get frame 
 * 	    buffer id for test buffer using drm API. Attach the plane1 to 
 * 	    lcd_crtc to draw overlay window on lcd.
 *
 * @param:  dev  device pointer
 *****************************************************************************
*/
static void set_plane1(struct device *dev)
{
        uint32_t flags = 0;
	uint32_t handles1[4], pitches1[4], offsets1[4];
	struct kms_bo *plane_bo1;

	/* Plane1 mode */
	dev->p1.xres = 320;
	dev->p1.yres = 240;

	/* Position of overlay window:(0,0) for top left corner,
	   change value if you want to place it somewhere.
	*/
	dev->crtc_x = 0;
	dev->crtc_y = 0;

	/* Size to be displayed on lcd */
	dev->crtc_w = dev->p1.xres;
	dev->crtc_h = dev->p1.yres;

	plane_bo1 = create_test_buffer(dev->kms, DRM_FORMAT_ARGB8888,
				       dev->p1.xres, dev->p1.yres,
				       handles1, pitches1, offsets1,
				       PATTERN_TILES);
	if (!plane_bo1) {
		error("create test buffer for plane1 failed\n");
		exit(0);
	}

	if (drmModeAddFB2(dev->fd, dev->p1.xres, dev->p1.yres,
			  DRM_FORMAT_ARGB8888, handles1, pitches1,
			  offsets1, &dev->p1.fb_id, flags)) {
		error("failed to add fb for plane1\n");
		exit(0);
	}

	if (drmModeSetPlane(dev->fd, dev->p1.id, dev->enc->crtc_id,
			    dev->p1.fb_id, flags, dev->crtc_x, dev->crtc_y,
			    dev->crtc_w, dev->crtc_h, 0, 0,
			    dev->p1.xres << 16, dev->p1.yres << 16)) {
		error("failed to enable plane1\n");
		exit(0);
	}
}

/**
 *****************************************************************************
 * @brief:  This function creates test buffer with pattern filled. Get frame
 *	    buffer id for test buffer using drm API. Attach the plane2 to 
 *	    lcd_crtc to draw another overlay window on lcd.
 *
 * @param:  dev  device pointer
 *****************************************************************************
*/
static void set_plane2(struct device *dev)
{
        uint32_t flags = 0;
	uint32_t handles2[4], pitches2[4], offsets2[4];
	struct kms_bo *plane_bo2;

	/* plane2 mode */
	dev->p2.xres = 160;
	dev->p2.yres = 120;

	dev->crtc_x = 0;
	dev->crtc_y = 0;

	dev->crtc_w = dev->p2.xres;
	dev->crtc_h = dev->p2.yres;

	plane_bo2 = create_test_buffer(dev->kms, DRM_FORMAT_ARGB8888,
				       dev->p2.xres, dev->p2.yres,
				       handles2, pitches2, offsets2,
				       PATTERN_SMPTE);
	if (!plane_bo2) {
		error("create test buffer for plane2 failed\n");
		exit(0);
	}

	if (drmModeAddFB2(dev->fd, dev->p2.xres, dev->p2.yres,
			  DRM_FORMAT_ARGB8888, handles2, pitches2,
			  offsets2, &dev->p2.fb_id, flags)) {
		error("failed to add fb for plane2\n");
		exit(0);
	}

	if (drmModeSetPlane(dev->fd, dev->p2.id, dev->enc->crtc_id,
			    dev->p2.fb_id, flags, dev->crtc_x, dev->crtc_y,
			    dev->crtc_w, dev->crtc_h, 0, 0,
			    dev->p2.xres << 16, dev->p2.yres << 16)) {
		error("failed to enable plane2\n");
		exit(0);
	}
}

/**
 *****************************************************************************
 * @brief:  This function creates test buffer with pattern filled. Get frame
 *	    buffer id for test buffer using drm API. Call to drmModeSetCrtc()
 *	    will draw pattern on lcd along with two overlay windows, which are 
 * 	    attached to it.
 *
 * @param:  dev  device pointer
 *****************************************************************************
*/
static void set_crtc(struct device *dev)
{
        uint32_t flags = 0;
	uint32_t handles3[4], pitches3[4], offsets3[4];
	struct kms_bo *plane_bo3;

	plane_bo3 = create_test_buffer(dev->kms, DRM_FORMAT_XRGB8888,
				       dev->xres, dev->yres,
				       handles3, pitches3, offsets3,
				       PATTERN_SMPTE);
	if (!plane_bo3) {
		error("create test buffer for crtc failed\n");
		exit(0);
	}

	if (drmModeAddFB2(dev->fd, dev->xres, dev->yres,
			  DRM_FORMAT_XRGB8888, handles3, pitches3,
			  offsets3, &dev->fb_id3, flags)) {
		error("failed to add fb for crtc\n");
		exit(0);
	}

	if (drmModeSetCrtc(dev->fd, dev->enc->crtc_id, dev->fb_id3,
			   0, 0, &dev->con->connector_id, 1,
			   &dev->mode)) {
		error("failed to set mode!\n");
		exit(0);
	}

}

/**
 *****************************************************************************
 * @brief:  This function extracts the lcd mode from command line
 *
 * @param:  dev  device pointer
 * @param:  p  	 optarg pointer
 *****************************************************************************
*/
static int parse_mode(struct device *dev, const char *p)
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
 * @brief:  This function extracts the plane1 properties from command line
 *
 * @param:  dev  device pointer
 * @param:  p  	 optarg pointer
 *****************************************************************************
*/
static int parse_plane1(struct device *dev, const char *p)
{
	if (sscanf(p, "%d:%d:%d:%d", &dev->p1.id, &dev->p1.z_val, 
		       &dev->p1.glo_alp, &dev->p1.pre_mul_alp) != 4)
		return -1;

	return 0;
}

/**
 *****************************************************************************
 * @brief:  This function extracts the plane2 properties from command line
 *
 * @param:  dev  device pointer
 * @param:  p  	 optarg pointer
 *****************************************************************************
*/
static int parse_plane2(struct device *dev, const char *p)
{
	if (sscanf(p, "%d:%d:%d:%d", &dev->p2.id, &dev->p2.z_val, 
		       &dev->p2.glo_alp, &dev->p2.pre_mul_alp) != 4)
		return -1;

	return 0;
}

static void usage()
{
	printf("Usage:\n");
	printf("-s wxh\t\t\t\t\t\t- set lcd mode\n");
	printf("-w plane_id:z-order:global_alpha:pre_mul_alpha\t");
	printf("- set z-order & alpha blending\n\n");
	printf("Run modetest app to find plane ids\n\n");
	printf("Z-order value between 0 to 3\n\t");
	printf("- lowest value for bottom\n\t");
	printf("- highest value for top\n\n");
	printf("Global alpha value between 0 to 255\n\t");
	printf("0   - fully transparent\n\t");
	printf("127 - semi transparent\n\t");
	printf("255 - fully opaque\n\n");
	printf("Pre multiplied alpha value\n\t");
	printf("0 - source is not premultiplied with alpha\n\t");
	printf("1 - source is premultiplied with alpha\n");
	printf("ex:\n# drmzalpha -s 800x480 -w 15:1:255:1 -w 16:2:255:1\n\n");
	exit(0);
}

static char optstr[] = "s:w:";

int main(int argc, char *argv[])
{
	struct device dev;

	uint32_t c, ret, plane_count = 0;
	uint32_t oid;

        memset(&dev, 0, sizeof dev);

	while ((c = getopt(argc, argv, optstr)) != -1) {

		switch (c) {
		case 's':
			if (parse_mode(&dev, optarg) < 0)
				usage();
			break;
		case 'w':
			if (!plane_count) {
				if (parse_plane1(&dev, optarg) < 0)
					usage();
			} else {
				if (parse_plane2(&dev, optarg) < 0)
					usage();
			}
			plane_count++;
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
	} else
		printf("/dev/dri/card0 open success!\n");

	/* Get drm resources */
	dev.res = drmModeGetResources(dev.fd);
	if (!dev.res) {
		error("drmModeGetResources failed\n");
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

	/* Create kms driver */
	ret = kms_create(dev.fd, &dev.kms);
	if (ret) {
		error("failed to create kms driver\n");
		exit(0);
	}

	/* Set plane1 properties */
	set_z_order(&dev, dev.p1.id);
	set_global_alpha(&dev, dev.p1.id);
	set_pre_multiplied_alpha(&dev, dev.p1.id);

	/* Set plane2 properties */
	set_z_order(&dev, dev.p2.id);
	set_global_alpha(&dev, dev.p2.id);
	set_pre_multiplied_alpha(&dev, dev.p2.id);

	/* To achieve overlay, set plane before crtc */
	set_plane1(&dev);
	set_plane2(&dev);
	set_crtc(&dev);

	drmModeDirtyFB(dev.fd, dev.fb_id3, 0, 0);

	getchar();

	/* Undo the drm setup in the correct sequence */
	drmModeSetCrtc(dev.fd, dev.enc->crtc_id, oid, 0, 0,
		       &dev.con->connector_id, 1, &(dev.crtc->mode));
	drmModeRmFB(dev.fd, dev.fb_id3);
	drmModeRmFB(dev.fd, dev.p2.fb_id);
	drmModeRmFB(dev.fd, dev.p1.fb_id);
	drmModeFreeEncoder(dev.enc);
	drmModeFreeConnector(dev.con);
	drmModeFreeCrtc(dev.crtc);
	drmModeFreeResources(dev.res);
	close(dev.fd);

	return 0;
}
