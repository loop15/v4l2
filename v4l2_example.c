/*v4l2_example.c*/
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <linux/videodev2.h>
#include <sys/mman.h>
#include <sys/time.h>

#define TRUE            (1)
#define FALSE           (0)
#define FILE_VIDEO      "/dev/video0"
#define IMAGE           "./img/demo"
#define IMAGEWIDTH      640
#define IMAGEHEIGHT     480
#define FRAME_NUM       4

int fd;
struct v4l2_buffer buf;

struct buffer
{
    void *start;
    unsigned int length;
    long long int timestamp;
} *buffers;

/*
常用的VIDIOC命令：
 1. VIDIOC_QUERYCAP （查询设备属性）
 2. VIDIOC_ENUM_FMT (显示所有支持的格式)
 3. VIDIOC_S_FMT （设置视频捕获格式）
 4. VIDIOC_G_FMT （获取硬件现在的视频捕获格式）
 5. VIDIOC_TRY_FMT （检查是否支持某种帧格式）
 6. VIDIOC_ENUM_FRAMESIZES （枚举设备支持的分辨率信息）
 7. VIDIOC_ENUM_FRAMEINTERVALS (获取设备支持的帧间隔)
 8. VIDIOC_S_PARM && VIDIOC_G_PARM (设置和获取流参数)
 9. VIDIOC_QUERYCAP (查询驱动的修剪能力)
 10. VIDIOC_S_CROP (设置视频信号的边框)
 11. VIDIOC_G_CROP (读取设备信号的边框)
 12. VIDIOC_REQBUFS (向设备申请缓存区)
 13. VIDIOC_QUERYBUF (获取缓存帧的地址、长度)
 14. VIDIOC_QBUF (把帧放入队列)
 15. VIDIOC_DQBUF (从队列中取出帧)
 16. VIDIOC_STREAMON && VIDIOC_STREAMOFF (启动/停止视频数据流)
————————————————
版权声明：本文为CSDN博主「Mark_minGE」的原创文章，遵循CC 4.0 BY-SA版权协议，转载请附上原文出处链接及本声明。
原文链接：https://blog.csdn.net/Mark_minGE/article/details/81427489
*/

int v4l2_init()
{
    struct v4l2_capability cap;
    struct v4l2_fmtdesc fmtdesc;
    struct v4l2_format fmt;
    struct v4l2_streamparm stream_para;

    //打开摄像头设备
    if ((fd = open(FILE_VIDEO, O_RDWR)) == -1)
    {
        printf("Error opening V4L interface\n");
        return FALSE;
    }

    //查询设备属性
    if (ioctl(fd, VIDIOC_QUERYCAP, &cap) == -1)
    {
        printf("Error opening device %s: unable to query device.\n", FILE_VIDEO);
        return FALSE;
    }
    else
    {
        /*
        struct v4l2_capability
        {
          __u8  driver[16];        // 驱动名字
          __u8  card[32];          // 设备名字
          __u8  bus_info[32];      // 设备在系统中的位置
          __u32 version;           // 驱动版本号
          __u32 capabilities;      // 设备支持的操作
          __u32 reserved[4];       // 保留字段
        };
        */
        printf("driver:\t\t%s\n", cap.driver);
        printf("card:\t\t%s\n", cap.card);
        printf("bus_info:\t%s\n", cap.bus_info);
        printf("version:\t%d\n", cap.version);
        printf("capabilities:\t%x\n", cap.capabilities);
//在实际操作过程中，可以将取得的 capabilites 与这些宏进行与远算来判断设备是否支持相应的功能。

//V4L2_CAP_VIDEO_CAPTURE 是否支持图像获取
        if ((cap.capabilities & V4L2_CAP_VIDEO_CAPTURE) == V4L2_CAP_VIDEO_CAPTURE)
        {
            printf("Device %s: Is a video capture device.\n", FILE_VIDEO);
        }

// 这个设备支持 video output 的接口，即这个设备具备 video output 的功能
        if ((cap.capabilities & V4L2_CAP_VIDEO_OUTPUT) == V4L2_CAP_VIDEO_OUTPUT)
        {
            printf("Device %s: Is a video output device.\n", FILE_VIDEO);
        }

// 这个设备支持 video overlay 的接口，即这个设备具备 video overlay 的功能，
//imag 在视频设备的 meomory 中保存，并直接在屏幕上显示，而不需要经过其他的处理。
        if ((cap.capabilities & V4L2_CAP_VIDEO_OVERLAY) == V4L2_CAP_VIDEO_OVERLAY)
        {
            printf("Device %s: Can do video overlay.\n", FILE_VIDEO);
        }

// 这个设备是否支持 read() 和 write() I/O 操作函数
        if ((cap.capabilities & V4L2_CAP_READWRITE) == V4L2_CAP_READWRITE)
        {
            printf("Device %s: Is a read/write systemcalls device.\n", FILE_VIDEO);
        }

        if ((cap.capabilities & V4L2_CAP_VIDEO_CAPTURE_MPLANE) == V4L2_CAP_VIDEO_CAPTURE_MPLANE)
        {
            printf("Device %s: Is a video capture device that supports multiplanar formats.\n", FILE_VIDEO);
        }

        if ((cap.capabilities & V4L2_CAP_VIDEO_OUTPUT_MPLANE) == V4L2_CAP_VIDEO_OUTPUT_MPLANE)
        {
            printf("Device %s: Is a video output device that supports multiplanar formats .\n", FILE_VIDEO);
        }

// 这个设备是否支持 streaming I/O 操作函数
        if ((cap.capabilities & V4L2_CAP_STREAMING) == V4L2_CAP_STREAMING)
        {
            printf("Device %s: supports streaming.\n", FILE_VIDEO);
        }
    }

    /*
    //相关的结构体：
    struct v4l2_fmtdesc
    {
      __u32  index;                // 要查询的格式序号，应用程序设置
      enum   v4l2_buf_type type;   // 帧类型，应用程序设置
      __u32  flags;                // 是否为压缩格式
      __u8   description[32];      // 格式名称
      __u32  pixelformat;          // 格式
      __u32  reserved[4];         // 保留，不使用设置为0
    };
    参数分析：
      1. index ：是用来确定格式的一个简单整型数。与其他V4L2所使用的索引一样，这个也是从0开始递增，至最大允许值为止。应用可以通过一直递增索引值知道返回-EINVAL的方式枚举所有支持的格式。
      2. type：是描述数据流类型的字段。对于视频捕获设备（摄像头或调谐器）来说，就是V4L2_BUF_TYPE_VIDEO_CAPTURE。
      3. flags: 只定义了一个值，即V4L2_FMT_FLAG_COMPRESSED，表示这是一个压缩的视频格式。
      4. description: 是对这个格式的一种简短的字符串描述。
      5. pixelformat: 应是描述视频表现方式的四字符码
    */
    //显示所有支持帧格式
    fmtdesc.index = 0;
    fmtdesc.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    printf("Support format:\n");
    while (ioctl(fd, VIDIOC_ENUM_FMT, &fmtdesc) != -1)
    {
        printf("\t%d.%s\n", fmtdesc.index + 1, fmtdesc.description);
        fmtdesc.index++;
    }

    //VIDIOC_TRY_FMT 检查是否支持某帧格式
    struct v4l2_format fmt_test;
    fmt_test.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt_test.fmt.pix.pixelformat = V4L2_PIX_FMT_RGB32;
    if (ioctl(fd, VIDIOC_TRY_FMT, &fmt_test) == -1)
    {
        printf("not support format RGB32!\n");
    }
    else
    {
        printf("support format RGB32\n");
    }

    // VIDIO_S_FMT 操作命令设置视频捕捉格式。
    printf("set fmt...\n");
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_RGB32; //jpg格式 V4L2_PIX_FMT_RGB32 V4L2_PIX_FMT_YUYV;//yuv格式
    fmt.fmt.pix.height = IMAGEHEIGHT;
    fmt.fmt.pix.width = IMAGEWIDTH;
    fmt.fmt.pix.field = V4L2_FIELD_INTERLACED;
    printf("fmt.type:\t\t%d\n", fmt.type);
    printf("pix.pixelformat:\t%c%c%c%c\n", fmt.fmt.pix.pixelformat & 0xFF, (fmt.fmt.pix.pixelformat >> 8) & 0xFF, (fmt.fmt.pix.pixelformat >> 16) & 0xFF, (fmt.fmt.pix.pixelformat >> 24) & 0xFF);
    printf("pix.height:\t\t%d\n", fmt.fmt.pix.height);
    printf("pix.width:\t\t%d\n", fmt.fmt.pix.width);
    printf("pix.field:\t\t%d\n", fmt.fmt.pix.field);
    if (ioctl(fd, VIDIOC_S_FMT, &fmt) == -1)
    {
        printf("Unable to set format\n");
        return FALSE;
    }

    //.VIDIOC_G_FMT 获取硬件现在的视频捕获格式
    printf("get fmt...\n");
    if (ioctl(fd, VIDIOC_G_FMT, &fmt) == -1)
    {
        printf("Unable to get format\n");
        return FALSE;
    }
    {
        printf("fmt.type:\t\t%d\n", fmt.type);
        printf("pix.pixelformat:\t%c%c%c%c\n", fmt.fmt.pix.pixelformat & 0xFF, (fmt.fmt.pix.pixelformat >> 8) & 0xFF, (fmt.fmt.pix.pixelformat >> 16) & 0xFF, (fmt.fmt.pix.pixelformat >> 24) & 0xFF);
        printf("pix.height:\t\t%d\n", fmt.fmt.pix.height);
        printf("pix.width:\t\t%d\n", fmt.fmt.pix.width);
        printf("pix.field:\t\t%d\n", fmt.fmt.pix.field);
    }

    //设置及查看帧速率，这里只能是30帧，就是1秒采集30张图
    memset(&stream_para, 0, sizeof(struct v4l2_streamparm));
    stream_para.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    stream_para.parm.capture.timeperframe.denominator = 30;
    stream_para.parm.capture.timeperframe.numerator = 1;

    if (ioctl(fd, VIDIOC_S_PARM, &stream_para) == -1)
    {
        printf("Unable to set frame rate\n");
        return FALSE;
    }
    if (ioctl(fd, VIDIOC_G_PARM, &stream_para) == -1)
    {
        printf("Unable to get frame rate\n");
        return FALSE;
    }
    {
        printf("numerator:%d\ndenominator:%d\n", stream_para.parm.capture.timeperframe.numerator, stream_para.parm.capture.timeperframe.denominator);
    }
    return TRUE;
}


int v4l2_mem_ops()
{
    unsigned int n_buffers;
    struct v4l2_requestbuffers req;

    //申请帧缓冲   VIDIOC_REQBUFS 向设备申请缓存区
    req.count = FRAME_NUM;                  // 缓存数量，也就是说在缓存队列里保持多少张照片
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE; //数据流类型，对视频捕获设备应是V4L2_BUF_TYPE_VIDEO_CAPTURE
    req.memory = V4L2_MEMORY_MMAP;          // V4L2_MEMORY_MMAP 或 V4L2_MEMORY_USERPTR
    if (ioctl(fd, VIDIOC_REQBUFS, &req) == -1)
    {
        printf("request for buffers error\n");
        return FALSE;
    }

    // 申请用户空间的地址列
    buffers = malloc(req.count * sizeof(*buffers));
    if (!buffers)
    {
        printf("out of memory!\n");
        return FALSE;
    }

    // 进行内存映射
    for (n_buffers = 0; n_buffers < FRAME_NUM; n_buffers++)
    {
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = n_buffers;
        //查询 取缓存帧的地址、长度
        if (ioctl(fd, VIDIOC_QUERYBUF, &buf) == -1)
        {
            printf("query buffer error\n");
            return FALSE;
        }

        //映射 把buf映射到内存 buffers 用户空间的一段内存区域直接映射到内核空间。传输效率高
        buffers[n_buffers].length = buf.length;
        buffers[n_buffers].start = mmap(NULL, buf.length, PROT_READ | PROT_WRITE, MAP_SHARED, fd, buf.m.offset);

        //显示缓存帧信息
        printf("Frame buffer %d: address=0x%x, length=%d\n", n_buffers, (unsigned int)buffers[n_buffers].start, buffers[n_buffers].length);
        if (buffers[n_buffers].start == MAP_FAILED)
        {
            printf("buffer map error\n");
            return FALSE;
        }
    }
    return TRUE;
}


int v4l2_frame_process()
{
    unsigned int n_buffers;
    enum v4l2_buf_type type;
    char file_name[100];
    char index_str[10];
    long long int extra_time = 0;
    long long int cur_time = 0;
    long long int last_time = 0;

    //入队和开启采集
    for (n_buffers = 0; n_buffers < FRAME_NUM; n_buffers++)
    {
        buf.index = n_buffers;
        ioctl(fd, VIDIOC_QBUF, &buf); //把帧放入队列 当缓存
    }
    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    ioctl(fd, VIDIOC_STREAMON, &type);

    //出队，处理，写入yuv文件，入队，循环进行
    int loop = 0;
    while (loop < 6)
    {
        for (n_buffers = 0; n_buffers < FRAME_NUM; n_buffers++)
        {
            //出队
            buf.index = n_buffers;
            ioctl(fd, VIDIOC_DQBUF, &buf); //取出帧

            //查看采集数据的时间戳之差，单位为微妙
            buffers[n_buffers].timestamp = buf.timestamp.tv_sec * 1000000 + buf.timestamp.tv_usec;
            cur_time = buffers[n_buffers].timestamp;
            extra_time = cur_time - last_time;
            last_time = cur_time;
            printf("time_deta:%lld\n\n", extra_time);
            printf("buf_len:%d\n", buffers[n_buffers].length);

            //处理数据只是简单写入文件，名字以loop的次数和帧缓冲数目有关
            printf("grab image data OK\n");
            memset(file_name, 0, sizeof(file_name));
            memset(index_str, 0, sizeof(index_str));
            sprintf(index_str, "%d", loop * 4 + n_buffers);
            strcpy(file_name, IMAGE);
            strcat(file_name, index_str);
            strcat(file_name, ".jpg");
            //strcat(file_name,".yuv");
            FILE *file = fopen(file_name, "wb");
            if (!file)
            {
                printf("open %s error\n", file_name);
                return (FALSE);
            }
			/*
			（1）buffer：是一个指针，对fwrite来说，是要获取数据的地址；
			（2）size：要写入内容的单字节数；
			（3）count:要进行写入size字节的数据项的个数；
			（4）stream:目标文件指针；
			（5）返回实际写入的数据项个数count。

			*/
            fwrite(buffers[n_buffers].start, IMAGEHEIGHT * IMAGEWIDTH * 2, 1, file);
            fclose(file);
            printf("save %s OK\n", file_name);

            //入队循环
            ioctl(fd, VIDIOC_QBUF, &buf); //放回帧
        }

        loop++;
    }
    return TRUE;
}

int v4l2_release()
{
    unsigned int n_buffers;
    enum v4l2_buf_type type;

    //关闭流
    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    ioctl(fd, VIDIOC_STREAMON, &type);

    //关闭内存映射
    for (n_buffers = 0; n_buffers < FRAME_NUM; n_buffers++)
    {
        munmap(buffers[n_buffers].start, buffers[n_buffers].length);
    }

    //释放自己申请的内存
    free(buffers);

    //关闭设备
    close(fd);
    return TRUE;
}

int v4l2_video_input_output()
{
    struct v4l2_input input;
    struct v4l2_standard standard;

    //首先获得当前输入的index,注意只是index，要获得具体的信息，就的调用列举操作
    memset(&input, 0, sizeof(input));
    if (-1 == ioctl(fd, VIDIOC_G_INPUT, &input.index))
    {
        printf("VIDIOC_G_INPUT\n");
        return FALSE;
    }
    //调用列举操作，获得 input.index 对应的输入的具体信息
    if (-1 == ioctl(fd, VIDIOC_ENUMINPUT, &input))
    {
        printf("VIDIOC_ENUM_INPUT \n");
        return FALSE;
    }
    printf("Current input %s supports:\n", input.name);


    //列举所有的所支持的 standard，如果 standard.id 与当前 input 的 input.std 有共同的
    //bit flag，意味着当前的输入支持这个 standard,这样将所有驱动所支持的 standard 列举一个
    //遍，就可以找到该输入所支持的所有 standard 了。

    memset(&standard, 0, sizeof(standard));
    standard.index = 0;
    while (0 == ioctl(fd, VIDIOC_ENUMSTD, &standard))
    {
        if (standard.id & input.std)
        {
            printf("%s\n", standard.name);
        }
        standard.index++;
    }
    // EINVAL indicates the end of the enumeration, which cannot be empty unless this device falls under the USB exception.

    if (errno != EINVAL || standard.index == 0)
    {
        printf("VIDIOC_ENUMSTD\n");
        return FALSE;
    }

}


int main(int argc, char const *argv[])
{
    printf("begin....\n");
    //  sleep(10);

    v4l2_init();
    printf("init....\n");
    // sleep(10);

    v4l2_mem_ops();
    printf("malloc....\n");
    //sleep(10);

    v4l2_frame_process();
    printf("process....\n");
    //sleep(10);

    v4l2_release();
    printf("release\n");
    sleep(20);

    return TRUE;
}
