#include <stdio.h>
#include <stdint.h>
#include <linux/unistd.h>
#include <fcntl.h>
#include <string.h>

#include <sys/ioctl.h>
#include <linux/videodev2.h>
#include <linux/v4l2-subdev.h>
#include <linux/v4l2-mediabus.h>

#define MAX_NUM_FMTS	10
#define ERR	printf
#define DBG	printf

struct sensor_data {
	char devname[256];
	int fd;

	uint32_t fmt_codes[MAX_NUM_FMTS];
	int num_fmts;

	struct v4l2_mbus_framefmt mbus_fmt;

};

struct app_params {
	int width;
	int height;
	int code;
	int field;
	int fps;

	int stream_on;
	int info_only;
	char *fmtstr;
};

struct sensor_data default_data = {
	.devname = "/dev/cam1",
	.num_fmts = 0,
};

struct app_params default_prms = {
	.width = 1280,
	.height = 720,
	.code = 1,
	.field = 'p',
	.fps = 30,
	.stream_on = 1,
	.info_only = 0,
	.fmtstr = "",
};

void app_usage() {
	printf("\n");
	printf("sensor-cfg - A program to configure sensors independantly\n");
	printf("Options:-\n");
	printf("  --dev     <device name> (e.g. /dev/cam1)\n");
	printf("  --stream  <val> (1 or 0 to turn on or off the sensor)\n");
	printf("  --fmt     <width>x<height><field>:<code>:<fps>\n");
	printf("             * field should be 'i' or 'p' for interlaced or progressive\n");
	printf("             * code is the index of the enumerated pixel codes\n");
	printf("             * e.g. --fmt 1280x720p:1:30\n");
	printf("  --info    Only print the enumerated formats and get_format\n");
	printf("  --help    Show this help and exit\n");
	printf("\n");
}

int app_parse_args(int argc, char *argv[], struct sensor_data *data,
		struct app_params *prms) {
	int i;

	for (i = 1; i < argc; i++) {
		if (strcmp(argv[i], "--help") == 0) {
			return -1;
		} else if (strcmp(argv[i], "--info") == 0) {
			prms->info_only = 1;
		} else if (strcmp(argv[i], "--dev") == 0) {
			if (i+1 >= argc) {
				ERR("Invalid option '--dev'\n");
				return -2;
			}
			sscanf(argv[++i], "%s", data->devname);
		} else if (strcmp(argv[i], "--stream") == 0) {
			if (i+1 >= argc) {
				ERR("Invalid option '--stream'\n");
				return -3;
			}
			sscanf(argv[++i], "%d", &prms->stream_on);
		} else if (strcmp(argv[i], "--fmt") == 0) {
			if (i+1 >= argc) {
				ERR("Invalid option '--fmt'\n");
				return -4;
			}
			sscanf(argv[++i], "%dx%d%c:%d@%d", &prms->width, &prms->height,
				&prms->field, &prms->code, &prms->fps);
			prms->fmtstr = argv[i];
		} else {
			ERR("Invalid argument '%s'\n", argv[i]);
			return -5;
		}
	}
	return 0;
}

int apply_params(struct app_params *prms, struct sensor_data *data) {
	int idx;

	data->mbus_fmt.width = prms->width;
	data->mbus_fmt.height = prms->height;

	idx = prms->code;
	if (idx > data->num_fmts) {
		ERR("Invalid format %s (code out of range)\n", prms->fmtstr);
		return -1;
	}
	data->mbus_fmt.code = data->fmt_codes[idx];

	if (prms->field == 'i' || prms->field == 'I') {
		data->mbus_fmt.field = V4L2_FIELD_ALTERNATE;
	} else if (prms->field == 'p' || prms->field == 'P') {
		data->mbus_fmt.field = V4L2_FIELD_ALTERNATE;
	} else {
		ERR("Invalid format %s (Invalid field)\n", prms->fmtstr);
		return -2;
	}
	return 0;
}

int open_dev(struct sensor_data *data) {

	DBG("Opening device %s\n", data->devname);
	data->fd = open(data->devname, O_RDWR);

	if (data->fd < 0) {
		ERR("Failed to open %s\n", data->devname);
		return -1;
	}
	return 0;
}

int enum_fmts(struct sensor_data *data) {
	int ret = 0, i = 0;

	DBG("Enumerating formats\n");
	for (i = 0; i < MAX_NUM_FMTS; i++) {

		struct v4l2_subdev_mbus_code_enum code = {
			.index = i,
		};

		ret = ioctl(data->fd, VIDIOC_SUBDEV_ENUM_MBUS_CODE, &code);
		if (ret != 0)
			break;
		data->fmt_codes[i] = code.code;
	}

	if (i == 0) {
		ERR("Failed to enumerate mediabus formats\n");
		return -2;
	}
	data->num_fmts = i;

	return 0;
}

static uint32_t known_fmts[] = {
	V4L2_MBUS_FMT_YUYV8_2X8,
	V4L2_MBUS_FMT_UYVY8_2X8,
	V4L2_MBUS_FMT_YVYU8_2X8,
	V4L2_MBUS_FMT_VYUY8_2X8,
	V4L2_MBUS_FMT_YUYV10_2X10,
};

static char *known_fmt_desc[] = {
	"YUYV8_2X8 - 8 bit Y U Y V",
	"UYVY8_2X8 - 8 bit U Y V Y",
	"YVYU8_2X8 - 8 bit Y V Y U",
	"VYUY8_2X8 - 8 bit V Y U Y",
	"YUYV10_2X10 - 10 bit YUYV",
};

char *describe_fmt_code(uint32_t code) {
	int j, len  = sizeof(known_fmts) / sizeof(known_fmts[0]);
	char *desc;

	desc = "Unknown format";
	for (j = 0; j < len; j++) {
		if (code == known_fmts[j]) {
			desc = known_fmt_desc[j];
		}
	}
	return desc;
}

void print_fmts(struct sensor_data *data) {
	int i, j;

	DBG("Supported media formats(%d):-\n", data->num_fmts);
	for (i = 0; i < data->num_fmts; i++) {
		DBG(" %d] 0x%08x \"%s\"\n", i, data->fmt_codes[i],
			describe_fmt_code(data->fmt_codes[i]));
	}
}

int get_fmt(struct sensor_data *data) {
	struct v4l2_mbus_framefmt *mf = &data->mbus_fmt;
	struct v4l2_subdev_format fmt;
	int ret = 0;

	ret = ioctl(data->fd, VIDIOC_SUBDEV_G_FMT, &fmt);
	if (ret) {
		ERR("Failed to get subdev format\n");
		return -3;
	}

	*mf = fmt.format;
	DBG("Get format: %dx%d@(%s), field = 0x%08x, colorspace = 0x%08x\n",
		mf->width, mf->height, describe_fmt_code(mf->code), mf->field,
		mf->colorspace);
	return 0;
}

int set_fmt(struct sensor_data *data) {
	struct v4l2_mbus_framefmt *mf = &data->mbus_fmt;
	struct v4l2_subdev_format fmt = {
		.format = *mf,
	};
	int ret = 0;

	DBG("Set format: %dx%d@(%s), field = 0x%08x, colorspace = 0x%08x\n",
		mf->width, mf->height, describe_fmt_code(mf->code), mf->field,
		mf->colorspace);
	ret = ioctl(data->fd, VIDIOC_SUBDEV_S_FMT, &fmt);
	if (ret) {
		ERR("Failed to set subdev format\n");
		return -4;
	}
	return 0;
}


int start_streaming(struct sensor_data *data) {
	int ret = 0;
	uint32_t buf_type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

	DBG("Stream ON\n");
	ret = ioctl(data->fd, VIDIOC_STREAMON, &buf_type);
	if (ret) {
		ERR("Failed to stream ON\n");
		return -5;
	}
	return ret;
}

int stop_streaming(struct sensor_data *data) {
	int ret = 0;
	uint32_t buf_type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

	DBG("Stream OFF\n");
	ret = ioctl(data->fd, VIDIOC_STREAMOFF, &buf_type);
	if (ret) {
		ERR("Failed to stream OFF\n");
		return -6;
	}
	return ret;
}

int main(int argc, char *argv[]) {
	struct app_params prms = default_prms;
	struct sensor_data data = default_data;
	int ret = 0;

	if(ret = app_parse_args(argc, argv, &data, &prms)) {
		app_usage();
		return ret;
	}

	if (ret = open_dev(&data))
		return ret;

	if (prms.stream_on == 0) {
		if (ret = stop_streaming(&data))
			return ret;
		return 0;
	}

	if (ret = enum_fmts(&data))
		return ret;

	print_fmts(&data);

	if (prms.info_only)
		return 0;

	if (ret = get_fmt(&data))
		return ret;

	if (ret = apply_params(&prms, &data)) {
		app_usage();
		return ret;
	}

	if (ret = set_fmt(&data))
		return ret;

	if (ret = start_streaming(&data))
		return ret;

	return 0;
}
