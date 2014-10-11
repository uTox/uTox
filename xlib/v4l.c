static int fd = -1;
#ifdef __APPLE__
_Bool v4l_init(char *dev_name)
{
    return 0;
}

void v4l_close(void)
{
}

_Bool v4l_startread(void)
{
    return 0;
}

_Bool v4l_endread(void)
{
    return 0;
}

_Bool v4l_getframe(vpx_image_t *image)
{
    return 0;
}
#else
#include <sys/mman.h>
#ifdef __OpenBSD__
#include <sys/videoio.h>
#else
#include <linux/videodev2.h>
#endif

#include <libv4lconvert.h>

#define CLEAR(x) memset(&(x), 0, sizeof(x))

static int xioctl(int fh, unsigned long request, void *arg)
{
    int r;

    do {
        r = ioctl(fh, request, arg);
    } while (-1 == r && EINTR == errno);

    return r;
}
struct buffer {
    void   *start;
    size_t  length;
};
static struct buffer *buffers;
static uint32_t n_buffers;
static struct v4lconvert_data *v4lconvert_data;
static struct v4l2_format fmt, dest_fmt = {
    //.type = V4L2_BUF_TYPE_VIDEO_CAPTURE,
    .fmt = {
        .pix = {
            .pixelformat = V4L2_PIX_FMT_YVU420,
            //.field = V4L2_FIELD_NONE,
        },
    },
};

_Bool v4l_init(char *dev_name)
{
    fd = open(dev_name, O_RDWR /* required */ | O_NONBLOCK, 0);

    if (-1 == fd) {
        debug("Cannot open '%s': %d, %s\n", dev_name, errno, strerror(errno));
        return 0;
    }

    struct v4l2_capability cap;
    struct v4l2_cropcap cropcap;
    struct v4l2_crop crop;
    unsigned int min;

    if(-1 == xioctl(fd, VIDIOC_QUERYCAP, &cap)) {
        if (EINVAL == errno) {
            debug("%s is no V4L2 device\n", dev_name);
        } else {
            debug("VIDIOC_QUERYCAP error %d, %s\n", errno, strerror(errno));
        }
        return 0;
    }

    if(!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
        debug("%s is no video capture device\n", dev_name);
        return 0;
    }

    if(!(cap.capabilities & V4L2_CAP_STREAMING)) {
        debug("%s does not support streaming i/o\n", dev_name);
        return 0;
    }

    /* Select video input, video standard and tune here. */
    CLEAR(cropcap);

    cropcap.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    if (0 == xioctl(fd, VIDIOC_CROPCAP, &cropcap)) {
        crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        crop.c = cropcap.defrect; /* reset to default */

        if (-1 == xioctl(fd, VIDIOC_S_CROP, &crop)) {
            switch (errno) {
            case EINVAL:
                /* Cropping not supported. */
                break;
            default:
                /* Errors ignored. */
                break;
            }
        }
    } else {
        /* Errors ignored. */
    }

    v4lconvert_data = v4lconvert_create(fd);

    CLEAR(fmt);

    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    if(-1 == xioctl(fd, VIDIOC_G_FMT, &fmt)) {
        debug("VIDIOC_S_FMT error %d, %s\n", errno, strerror(errno));
        return 0;
    }

    /*if(fmt.fmt.pix.pixelformat != V4L2_PIX_FMT_YUYV) {
        debug("Unsupported video format: %u %u %u %u\n", fmt.fmt.pix.width, fmt.fmt.pix.height, fmt.fmt.pix.pixelformat, fmt.fmt.pix.field);
    }*/

    video_width = fmt.fmt.pix.width;
    video_height = fmt.fmt.pix.height;
    dest_fmt.fmt.pix.width = fmt.fmt.pix.width;
    dest_fmt.fmt.pix.height = fmt.fmt.pix.height;
    debug("Video size: %u %u\n", video_width, video_height);



    /* Buggy driver paranoia. */
    min = fmt.fmt.pix.width * 2;
    if (fmt.fmt.pix.bytesperline < min)
        fmt.fmt.pix.bytesperline = min;
    min = fmt.fmt.pix.bytesperline * fmt.fmt.pix.height;
    if (fmt.fmt.pix.sizeimage < min)
        fmt.fmt.pix.sizeimage = min;


    /* part 3*/
    //uint32_t buffer_size = fmt.fmt.pix.sizeimage;
    struct v4l2_requestbuffers req;

    CLEAR(req);

    req.count  = 4;
    req.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP;//V4L2_MEMORY_USERPTR;

    if (-1 == xioctl(fd, VIDIOC_REQBUFS, &req)) {
        if (EINVAL == errno) {
            debug("%s does not support x i/o\n", dev_name);
        } else {
            debug("VIDIOC_REQBUFS error %d, %s\n", errno, strerror(errno));
        }
        return 0;
    }

    if(req.count < 2) {
        fprintf(stderr, "Insufficient buffer memory on %s\n", dev_name);
        exit(EXIT_FAILURE);
    }

    buffers = calloc(req.count, sizeof(*buffers));

    for (n_buffers = 0; n_buffers < req.count; ++n_buffers) {
            struct v4l2_buffer buf;

            CLEAR(buf);

            buf.type        = V4L2_BUF_TYPE_VIDEO_CAPTURE;
            buf.memory      = V4L2_MEMORY_MMAP;
            buf.index       = n_buffers;

            if (-1 == xioctl(fd, VIDIOC_QUERYBUF, &buf)) {
                debug("VIDIOC_QUERYBUF error %d, %s\n", errno, strerror(errno));
                return 0;
            }

            buffers[n_buffers].length = buf.length;
            buffers[n_buffers].start =
                    mmap(NULL /* start anywhere */,
                          buf.length,
                          PROT_READ | PROT_WRITE /* required */,
                          MAP_SHARED /* recommended */,
                          fd, buf.m.offset);

            if(MAP_FAILED == buffers[n_buffers].start) {
                debug("mmap error %d, %s\n", errno, strerror(errno));
                return 0;
            }

    }

    /*buffers = calloc(4, sizeof(*buffers));

    if (!buffers) {
        debug("Out of memory\n");
        return 0;
    }

    for (n_buffers = 0; n_buffers < 4; ++n_buffers) {
        buffers[n_buffers].length = buffer_size;
        buffers[n_buffers].start = malloc(buffer_size);

        if (!buffers[n_buffers].start) {
            debug("Out of memory\n");
            return 0;
        }
    }*/
    return 1;
}

void v4l_close(void)
{
    int i;
    for(i = 0; i < n_buffers; ++i) {
        if(-1 == munmap(buffers[i].start, buffers[i].length)) {
            debug("munmap error\n");
        }
    }

    close(fd);
}

_Bool v4l_startread(void)
{
    debug("start webcam\n");
    unsigned int i;
    enum v4l2_buf_type type;

    for (i = 0; i < n_buffers; ++i) {
        struct v4l2_buffer buf;

        CLEAR(buf);
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;//V4L2_MEMORY_USERPTR;
        buf.index = i;
        //buf.m.userptr = (unsigned long)buffers[i].start;
        //buf.length = buffers[i].length;

        if (-1 == xioctl(fd, VIDIOC_QBUF, &buf)) {
            debug("VIDIOC_QBUF error %d, %s\n", errno, strerror(errno));
            return 0;
        }
    }
    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (-1 == xioctl(fd, VIDIOC_STREAMON, &type)) {
        debug("VIDIOC_STREAMON error %d, %s\n", errno, strerror(errno));
        return 0;
    }

    return 1;
}

_Bool v4l_endread(void)
{
    debug("stop webcam\n");
    enum v4l2_buf_type type;
    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if(-1 == xioctl(fd, VIDIOC_STREAMOFF, &type)) {
        debug("VIDIOC_STREAMOFF error %d, %s\n", errno, strerror(errno));
        return 0;
    }

    return 1;
}

int v4l_getframe(vpx_image_t *image)
{
    struct v4l2_buffer buf;
    //unsigned int i;

    CLEAR(buf);

    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;//V4L2_MEMORY_USERPTR;

    if (-1 == ioctl(fd, VIDIOC_DQBUF, &buf)) {
        switch (errno) {
        case EINTR:
        case EAGAIN:
            return 0;

        case EIO:
        /* Could ignore EIO, see spec. */

        /* fall through */

        default:
            debug("VIDIOC_DQBUF error %d, %s\n", errno, strerror(errno));
            return -1;

        }
    }

    /*for (i = 0; i < n_buffers; ++i)
        if (buf.m.userptr == (unsigned long)buffers[i].start
                && buf.length == buffers[i].length)
            break;

    if(i >= n_buffers) {
        debug("fatal error\n");
        return 0;
    }*/

    void *data = (void*)buffers[buf.index].start; //length = buf.bytesused //(void*)buf.m.userptr

    /* assumes planes are continuous memory */
    v4lconvert_convert(v4lconvert_data, &fmt, &dest_fmt, data, fmt.fmt.pix.sizeimage, image->planes[0], (video_width * video_height * 3) / 2);

    /*if(fmt.fmt.pix.pixelformat == V4L2_PIX_FMT_YUYV) {
        yuv422to420(image->planes[0], image->planes[1], image->planes[2], data, video_width, video_height);
    } else {

    }*/

    if (-1 == xioctl(fd, VIDIOC_QBUF, &buf)) {
        debug("VIDIOC_QBUF error %d, %s\n", errno, strerror(errno));
    }

    return 1;
}
#endif
