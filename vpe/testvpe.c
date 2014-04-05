/*
 *  Copyright (c) 2012-2013, Texas Instruments Incorporated
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 *  *  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *
 *  *  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 *  *  Neither the name of Texas Instruments Incorporated nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 *  THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 *  PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 *  CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 *  EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 *  PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 *  OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 *  WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 *  OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 *  EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

/**
 * @file       testvpe.c
 * @authors    Nikhil Devshatwar <nikhil.nd@ti.com>
 *	       Alaganraj S <alaganraj.s@ti.com>
 * @brief      Example program to test following vpe features,
 *             deinterlace, scalar, color space conversion.
 *
 * This program is to deinterlace a video file
 * It reads video fields from a file and writes progressive frames to another file
 * It uses v4l2 mem2mem multiplanar APIs
 * It uses memory mapped bufffers from mem2mem driver
 * Most of the commonly used color formats are supported
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>

#include <linux/videodev2.h>
#include <linux/v4l2-controls.h>

#include <sys/mman.h>
#include <sys/ioctl.h>

#define pexit(str) { \
	perror(str); \
	exit(1); \
}

#define V4L2_CID_TRANS_NUM_BUFS         (V4L2_CID_PRIVATE_BASE)

/**<	Input  file descriptor				*/
int	fin = -1;

/**<	Output file descriptor				*/
int	fout = -1;

/**<	File descriptor for device			*/
int	fd   = -1;

/*
 * Convert a format name string into fourcc and calculate the
 * size of one frame and set flags
 */
int describeFormat (
	char	*format,
	int	width,
	int	height,
	int	*size,
	int	*fourcc,
	int	*coplanar,
	enum v4l2_colorspace *clrspc)
{
	*size   = -1;
	*fourcc = -1;
	if (strcmp (format, "rgb24") == 0) {
		*fourcc = V4L2_PIX_FMT_RGB24;
		*size = height * width * 3;
		*coplanar = 0;
		*clrspc = V4L2_COLORSPACE_SRGB;

	} else if (strcmp (format, "bgr24") == 0) {
		*fourcc = V4L2_PIX_FMT_BGR24;
		*size = height * width * 3;
		*coplanar = 0;
		*clrspc = V4L2_COLORSPACE_SRGB;

	} else if (strcmp (format, "argb32") == 0) {
		*fourcc = V4L2_PIX_FMT_RGB32;
		*size = height * width * 4;
		*coplanar = 0;
		*clrspc = V4L2_COLORSPACE_SRGB;

	} else if (strcmp (format, "abgr32") == 0) {
		*fourcc = V4L2_PIX_FMT_BGR32;
		*size = height * width * 4;
		*coplanar = 0;
		*clrspc = V4L2_COLORSPACE_SRGB;

	} else if (strcmp (format, "yuv444") == 0) {
		*fourcc = V4L2_PIX_FMT_YUV444;
		*size = height * width * 3;
		*coplanar = 0;
		*clrspc = V4L2_COLORSPACE_SMPTE170M;

	} else if (strcmp (format, "yvyu") == 0) {
		*fourcc = V4L2_PIX_FMT_YVYU;
		*size = height * width * 2;
		*coplanar = 0;
		*clrspc = V4L2_COLORSPACE_SMPTE170M;

	} else if (strcmp (format, "yuyv") == 0) {
		*fourcc = V4L2_PIX_FMT_YUYV;
		*size = height * width * 2;
		*coplanar = 0;
		*clrspc = V4L2_COLORSPACE_SMPTE170M;

	} else if (strcmp (format, "uyvy") == 0) {
		*fourcc = V4L2_PIX_FMT_UYVY;
		*size = height * width * 2;
		*coplanar = 0;
		*clrspc = V4L2_COLORSPACE_SMPTE170M;

	} else if (strcmp (format, "vyuy") == 0) {
		*fourcc = V4L2_PIX_FMT_VYUY;
		*size = height * width * 2;
		*coplanar = 0;
		*clrspc = V4L2_COLORSPACE_SMPTE170M;

	} else if (strcmp (format, "nv16") == 0) {
		*fourcc = V4L2_PIX_FMT_NV16;
		*size = height * width * 2;
		*coplanar = 0;
		*clrspc = V4L2_COLORSPACE_SMPTE170M;

	} else if (strcmp (format, "nv61") == 0) {
		*fourcc = V4L2_PIX_FMT_NV61;
		*size = height * width * 2;
		*coplanar = 0;
		*clrspc = V4L2_COLORSPACE_SMPTE170M;

	} else if (strcmp (format, "nv12") == 0) {
		*fourcc = V4L2_PIX_FMT_NV12;
		*size = height * width * 1.5;
		*coplanar = 1;
		*clrspc = V4L2_COLORSPACE_SMPTE170M;

	} else if (strcmp (format, "nv21") == 0) {
		*fourcc = V4L2_PIX_FMT_NV21;
		*size = height * width * 1.5;
		*coplanar = 1;
		*clrspc = V4L2_COLORSPACE_SMPTE170M;

	} else {
		return 0;

	}

	return 1;
}

/*
 * Set format of one of the CAPTURE or OUTPUT stream (passed as type)
 * Request to allocate memory mapped buffers, map them and store
 * the mapped addresses of y and uv buffers into the array
 */
int allocBuffers(
	int	type,
	int	width,
	int	height,
	int	coplanar,
	enum v4l2_colorspace clrspc,
	int	*sizeimage_y,
	int	*sizeimage_uv,
	int	fourcc,
	void	*base[],
	void 	*base_uv[],
	int	*numbuf,
	int	interlace)
{
	struct v4l2_format		fmt;
	struct v4l2_requestbuffers	reqbuf;
	struct v4l2_buffer		buffer;
	struct v4l2_plane		buf_planes[2];
	int				i = 0;
	int				ret = -1;

	bzero(&fmt,sizeof(fmt));
	fmt.type		= type;
	fmt.fmt.pix_mp.width	= width;
	fmt.fmt.pix_mp.height	= height;
	fmt.fmt.pix_mp.pixelformat = fourcc;
	fmt.fmt.pix_mp.colorspace = clrspc;

	if (type == V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE && interlace)
		fmt.fmt.pix_mp.field = V4L2_FIELD_ALTERNATE;
	else
		fmt.fmt.pix_mp.field = V4L2_FIELD_ANY;

	/* Set format and update the sizes of y and uv planes
	 * If the returned value is changed, it means driver corrected the size
	 */
	ret = ioctl(fd,VIDIOC_S_FMT,&fmt);
	if(ret < 0) {
		pexit("Cant set color format\n");
	} else {
		*sizeimage_y = fmt.fmt.pix_mp.plane_fmt[0].sizeimage;
		*sizeimage_uv = fmt.fmt.pix_mp.plane_fmt[1].sizeimage;
	}

	bzero(&reqbuf,sizeof(reqbuf));
	reqbuf.count	= *numbuf;
	reqbuf.type	= type;
	reqbuf.memory	= V4L2_MEMORY_MMAP;

	ret = ioctl (fd, VIDIOC_REQBUFS, &reqbuf);
	if(ret < 0) {
		pexit("Cant request buffers\n");
	} else {
		*numbuf = reqbuf.count;
	}

	for(i = 0; i < *numbuf; i++) {

		memset(&buffer, 0, sizeof(buffer));
	        buffer.type	= type;
	        buffer.memory	= V4L2_MEMORY_MMAP;
	        buffer.index	= i;
		buffer.m.planes	= buf_planes;
		buffer.length	= coplanar ? 2 : 1;

	        ret = ioctl (fd, VIDIOC_QUERYBUF, &buffer);
		if (ret < 0)
			pexit("Cant query buffers\n");

		printf("query buf, plane 0 = %d, plane 1 = %d\n", buffer.m.planes[0].length,
			buffer.m.planes[1].length);

		/* Map all buffers and store the base address of each buffer */
	        base[i]	= mmap(NULL, buffer.m.planes[0].length, PROT_READ | PROT_WRITE,
			MAP_SHARED, fd, buffer.m.planes[0].m.mem_offset);

		if (MAP_FAILED == base[i]) {
			while(i>=0){
				/* Unmap all previous buffers in case of failure*/
				i--;
				munmap(base[i], *sizeimage_y);
				base[i] = NULL;
			}
			pexit("Cant mmap buffers Y");
			return 0;
		}

		if (!coplanar)
			continue;

	        base_uv[i] = mmap(NULL, buffer.m.planes[1].length, PROT_READ | PROT_WRITE,
			MAP_SHARED, fd, buffer.m.planes[1].m.mem_offset);

		if (MAP_FAILED == base_uv[i]) {
			while(i >= 0){
				/* Unmap all previous buffers */
				i--;
				munmap(base_uv[i], *sizeimage_uv);
				base[i] = NULL;
			}
			pexit("Cant mmap buffers UV");
			return 0;
		}
	}

	return 1;
}

void releaseBuffers(
	void	*base[],
	int	numbuf,
	int	bufsize)
{
	while (numbuf >= 0){
		numbuf--;
		munmap(base[numbuf],bufsize);
	}
}

/*
 * Queue all buffers into the stream (passed as type)
 * Used to queue all empty buffers before starting capture
 */
int queueAllBuffers(
	int	type,
	int	numbuf
	)
{
	struct v4l2_buffer	buffer;
	struct v4l2_plane	buf_planes[2];
	int			i = 0;
	int			ret = -1;
	int			lastqueued = -1;

	for (i=0; i<numbuf; i++) {
		memset(&buffer,0,sizeof(buffer));
	        buffer.type = type;
	        buffer.memory = V4L2_MEMORY_MMAP;
	        buffer.index = i;
		buffer.m.planes	= buf_planes;
		buffer.length	= 2;

		ret = ioctl (fd, VIDIOC_QBUF, &buffer);
		if (-1 == ret) {
			break;
		} else {
			lastqueued=i;
		}
	}
	return lastqueued;
}

/* Queue one buffer (identified by index) */
int queue(
	int	type,
	int	index,
	int	field,
	int	size_y,
	int size_uv)
{
	struct v4l2_buffer	buffer;
	struct v4l2_plane	buf_planes[2];
	int			ret = -1;

	buf_planes[0].length = buf_planes[0].bytesused = size_y;
	buf_planes[1].length = buf_planes[1].bytesused = size_uv;
	buf_planes[0].data_offset = buf_planes[1].data_offset = 0;

	memset(&buffer,0,sizeof(buffer));
	buffer.type	= type;
	buffer.memory	= V4L2_MEMORY_MMAP;
	buffer.index	= index;
	buffer.m.planes	= buf_planes;
	buffer.field    = field;
	buffer.length	= 2;

	ret = ioctl (fd, VIDIOC_QBUF, &buffer);
	if(-1 == ret) {
		pexit("Failed to queue\n");
	}
	return ret;
}

/*
 * Dequeue one buffer
 * Index of dequeue buffer is returned by ioctl
 */
int dequeue(int	type, struct v4l2_buffer *buf, struct v4l2_plane *buf_planes)
{
	int			ret = -1;

	memset(buf, 0, sizeof(*buf));
	buf->type	= type;
	buf->memory	= V4L2_MEMORY_MMAP;
	buf->m.planes	= buf_planes;
	buf->length	= 2;
	ret = ioctl (fd, VIDIOC_DQBUF, buf);
	if(-1 == ret) {
		pexit("Failed to dequeue\n");
	}
	return ret;
}

/* Start processing */
void streamON (
	int	type)
{
	int	ret = -1;
	ret = ioctl (fd, VIDIOC_STREAMON, &type);
	if(-1 == ret) {
		pexit("Cant Stream on\n");
	}
}

/* Stop processing */
void streamOFF (
	int	type)
{
	int	ret = -1;
	ret = ioctl (fd, VIDIOC_STREAMOFF, &type);
	if(-1 == ret) {
		pexit("Cant Stream on\n");
	}
}

/* Wrapper to read to read whole buffer in one call */
void do_read (char *str, int fd, void *addr, int size) {
	int nbytes = size, ret = 0, val;
	do {
		nbytes = size - ret;
		addr = addr + ret;
		if (nbytes == 0) {
			break;
		}
		ret = read(fd, addr, nbytes);
	} while(ret > 0);

	if (ret < 0) {
		val = errno;
		printf ("Reading failed %s: %d %s\n", str, ret, strerror(val));
		exit (1);
	} else {
		printf ("Total bytes read %s = %d\n", str, size);
	}
}

/* Wrapper to read to write whole buffer in one call */
void do_write (char *str, int fd, void *addr, int size) {
	int nbytes = size, ret = 0, val;
	do {
		nbytes = size - ret;
		addr = addr + ret;
		if(nbytes == 0) {
			break;
		}
		ret = write(fd, addr, nbytes);
	} while(ret > 0);
	if (ret < 0) {
		val = errno;
		printf ("Writing failed %s: %d %s\n", str, ret, strerror(val));
		exit (1);
	} else {
		printf ("Total bytes written %s = %d\n", str, size);
	}
}

int main (
	int	argc,
	char	*argv[])
{
	int	i, ret = 0;

	int	srcHeight  = 0, dstHeight = 0;
	int	srcWidth   = 0, dstWidth  = 0;
	int	srcSize    = 0, dstSize   = 0, srcSize_uv = 0, dstSize_uv = 0;
	int	srcFourcc  = 0, dstFourcc = 0;
	int	src_coplanar = 0, dst_coplanar = 0;
	enum v4l2_colorspace	src_colorspace, dst_colorspace;

	void	*srcBuffers[6];
	void	*dstBuffers[6];
	void	*srcBuffers_uv[6];
	void	*dstBuffers_uv[6];
	int	src_numbuf = 6;
	int	dst_numbuf = 6;
	int	num_frames = 20;
	int	interlace = 0;
	int	translen = 3;
	struct	v4l2_control ctrl;
	int 	field;

	if (argc < 11 || argc > 12) {
		printf (
		"USAGE : <SRCfilename> <SRCWidth> <SRCHeight> <SRCFormat> "
			"<DSTfilename> <DSTWidth> <DSTHeight> <DSTformat> "
                        "<interlace> <translen> <numframes>[optional]\n");

		return 1;
	}

	/** Open input file in read only mode				*/
	fin		= open (argv[1], O_RDONLY);
	srcWidth	= atoi (argv[2]);
	srcHeight	= atoi (argv[3]);
	describeFormat (argv[4], srcWidth, srcHeight, &srcSize, &srcFourcc, &src_coplanar, &src_colorspace);

	/** Open output file in write mode Create the file if not present	*/
	fout		= open (argv[5], O_WRONLY | O_CREAT | O_TRUNC, 777);
	dstWidth	= atoi (argv[6]);
	dstHeight	= atoi (argv[7]);
	describeFormat (argv[8], dstWidth, dstHeight, &dstSize, &dstFourcc, &dst_coplanar, &dst_colorspace);

	interlace = atoi (argv[9]);
	translen = atoi (argv[10]);

	if (argc == 12)
		num_frames = atoi (argv[11]);

	printf ("Input  @ %d = %d x %d , %d\nOutput @ %d = %d x %d , %d\n",
		fin,  srcWidth, srcHeight, srcFourcc,
		fout, dstWidth, dstHeight, dstFourcc);

	if (	fin  < 0 || srcHeight < 0 || srcWidth < 0 || srcFourcc < 0 || \
		fout < 0 || dstHeight < 0 || dstWidth < 0 || dstFourcc < 0 || \
		interlace < 0 || translen < 0 || num_frames < 0) {
		printf("ERROR: Invalid arguments\n");
		/** TODO:Handle errors precisely		*/
		exit (1);
	}
	/*************************************
		Files are ready Now
	*************************************/

	fd = open("/dev/video0",O_RDWR);
	if(fd < 0) {
		pexit("Can't open device\n");
	}

	/* Number of buffers to process in one transaction - translen */
	memset(&ctrl, 0, sizeof(ctrl));
	ctrl.id = V4L2_CID_TRANS_NUM_BUFS;
	ctrl.value = translen;
	ret = ioctl(fd, VIDIOC_S_CTRL, &ctrl);
	if (ret < 0)
		pexit("Can't set translen control\n");

	printf("S_CTRL success\n");
	/* Allocate buffers for CAPTURE and OUTPUT stream */
	ret = allocBuffers (V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE, srcWidth,
		srcHeight, src_coplanar, src_colorspace, &srcSize, &srcSize_uv,
		srcFourcc, srcBuffers, srcBuffers_uv, &src_numbuf, interlace);
	if(ret < 0) {
		pexit("Cant Allocate buffurs for OUTPUT device\n");
	}

	ret = allocBuffers (V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE, dstWidth,
		dstHeight, dst_coplanar, dst_colorspace, &dstSize, &dstSize_uv,
		dstFourcc, dstBuffers, dstBuffers_uv, &dst_numbuf, interlace);
	if(ret < 0) {
		pexit("Cant Allocate buffurs for CAPTURE device\n");
	}

	/**	Queue All empty buffers	(Available to capture in)	*/
	ret = queueAllBuffers (V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE, dst_numbuf);
	if(ret < 0) {
		pexit("Error queueing buffers for CAPTURE device\n");
	}

	printf ("Input  Buffers = %d each of size %d\nOutput Buffers = %d each of size %d\n",
		src_numbuf, srcSize, dst_numbuf, dstSize);

	/*************************************
		Driver is ready Now
	*************************************/

	/**	Read  into the OUTPUT  buffers from fin file	*/

	field = V4L2_FIELD_TOP;
	for (i = 0; i < src_numbuf; i++) {
		do_read("Y plane", fin, srcBuffers[i], srcSize);
		if (src_coplanar)
			do_read("UV plane", fin, srcBuffers_uv[i], srcSize_uv);

		queue(V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE, i, field, srcSize, srcSize_uv);
		if (field == V4L2_FIELD_TOP)
			field = V4L2_FIELD_BOTTOM;
		else
			field = V4L2_FIELD_TOP;
	}

	/*************************************
		Data is ready Now
	*************************************/

	streamON (V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE);
	streamON (V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE);

	while (num_frames) {
		struct v4l2_buffer buf;
		struct v4l2_plane buf_planes[2];
		int last = num_frames == 1 ? 1 : 0;

		/* Wait for and dequeue one buffer from OUTPUT
		 * to write data for next interlaced field */
		dequeue(V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE, &buf, buf_planes);
		printf("dequeued source buffer with index %d\n", buf.index);

		if (!last) {
			do_read ("Y plane", fin, srcBuffers[buf.index], srcSize);
			if (src_coplanar)
				do_read ("UV plane", fin, srcBuffers_uv[buf.index], srcSize_uv);

			queue(V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE, buf.index, field, srcSize, srcSize_uv);
			if (field == V4L2_FIELD_TOP)
				field = V4L2_FIELD_BOTTOM;
			else
				field = V4L2_FIELD_TOP;
		}

		/* Dequeue progressive frame from CAPTURE stream
		 * write to the file and queue one empty buffer */
		dequeue(V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE, &buf, buf_planes);
		printf("dequeued dest buffer with index %d\n", buf.index);

		do_write("Y plane", fout, dstBuffers[buf.index], dstSize);
		if (dst_coplanar)
			do_write("UV plane", fout, dstBuffers_uv[buf.index], dstSize_uv);

		if (!last)
			queue(V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE, buf.index, V4L2_FIELD_NONE, dstSize, dstSize_uv);

		num_frames--;

		printf("frames left %d\n", num_frames);
	}

	/* Driver cleanup */
	streamOFF (V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE);
	streamOFF (V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE);

	releaseBuffers (srcBuffers, src_numbuf, srcSize);
	releaseBuffers (dstBuffers, dst_numbuf, dstSize);

	if (src_coplanar)
		releaseBuffers (srcBuffers_uv, src_numbuf, srcSize_uv);
	if (dst_coplanar)
		releaseBuffers (dstBuffers_uv, dst_numbuf, dstSize_uv);

	close(fin);
	close(fout);
	close(fd);

	return 0;
}
