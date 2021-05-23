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
#define printf(fmt, args...)    printf("%s(%d):%s "#fmt"\r\n", __FILE__, __LINE__, __func__, ##args)

struct buffer
{
    void *start;
    unsigned int length;
    long long int timestamp;
} *my_buffers;

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
*/

//1. VIDIOC_QUERYCAP （查询设备属性） 看看设备具体支持哪些功能，比如是否具有视频的输入或者音频的输入等等
void query_capacity(int fd)
{
    struct v4l2_capability cap;

    //查询设备属性
    if (ioctl(fd, VIDIOC_QUERYCAP, &cap) == -1)
    {
        printf("Error opening device %s: unable to query device.\n", FILE_VIDEO);
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

}

//2. VIDIOC_ENUM_FMT (显示所有支持的格式)
void enum_fmt(int fd)
{

    /*相关的结构体：
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
    struct v4l2_fmtdesc fmtdesc;
    fmtdesc.index = 0;
    fmtdesc.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    printf("Support format:\n");
    while (ioctl(fd, VIDIOC_ENUM_FMT, &fmtdesc) != -1)
    {
        printf("\t%d.%s\n", fmtdesc.index + 1, fmtdesc.description);
        fmtdesc.index++;
    }
}

//3. VIDIOC_TRY_FMT 检查是否支持某帧格式
void try_fmt(int fd)
{
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

    fmt_test.fmt.pix.pixelformat = V4L2_PIX_FMT_BGR24;
    if (ioctl(fd, VIDIOC_TRY_FMT, &fmt_test) == -1)
    {
        printf("not support format V4L2_PIX_FMT_BGR24!\n");
    }
    else
    {
        printf("support format V4L2_PIX_FMT_BGR24\n");
    }

    fmt_test.fmt.pix.pixelformat = V4L2_PIX_FMT_BGR32;
    if (ioctl(fd, VIDIOC_TRY_FMT, &fmt_test) == -1)
    {
        printf("not support format V4L2_PIX_FMT_BGR32!\n");
    }
    else
    {
        printf("support format V4L2_PIX_FMT_BGR32\n");
    }

    fmt_test.fmt.pix.pixelformat = V4L2_PIX_FMT_RGB444;
    if (ioctl(fd, VIDIOC_TRY_FMT, &fmt_test) == -1)
    {
        printf("not support format V4L2_PIX_FMT_RGB444!\n");
    }
    else
    {
        printf("support format V4L2_PIX_FMT_RGB444\n");
    }
}


//4. VIDIOC_G_FMT （获取硬件现在的视频捕获格式）
int set_fmt(int fd)
{
    struct v4l2_format fmt;
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_RGB32; //jpg格式 V4L2_PIX_FMT_RGB32 V4L2_PIX_FMT_YUYV;//yuv格式
    fmt.fmt.pix.height = IMAGEHEIGHT;
    fmt.fmt.pix.width = IMAGEWIDTH;
    fmt.fmt.pix.field = V4L2_FIELD_INTERLACED;
    printf("set fmt...\n");
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

    if (ioctl(fd, VIDIOC_G_FMT, &fmt) == -1)
    {
        printf("Unable to get format\n");
        return FALSE;
    }
    printf("fmt.type:\t\t%d\n", fmt.type);
    printf("pix.pixelformat:\t%c%c%c%c\n", fmt.fmt.pix.pixelformat & 0xFF, (fmt.fmt.pix.pixelformat >> 8) & 0xFF, (fmt.fmt.pix.pixelformat >> 16) & 0xFF, (fmt.fmt.pix.pixelformat >> 24) & 0xFF);
    printf("pix.height:\t\t%d\n", fmt.fmt.pix.height);
    printf("pix.width:\t\t%d\n", fmt.fmt.pix.width);
    printf("pix.field:\t\t%d\n", fmt.fmt.pix.field);
}

// 4 VIDIOC_S_PARM && VIDIOC_G_PARM (设置和获取流参数)
int set_s_parm_g_parm(int fd)
{
    struct v4l2_streamparm stream_para;
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
}


// 5 VIDIOC_QUERYBUF (获取缓存帧的地址、长度)向驱动申请视频的帧缓冲区，一般不会超过5个
int get_buffer()
{
    struct v4l2_requestbuffers req;         //申请帧缓冲   VIDIOC_REQBUFS 向设备申请缓存区
    req.count = FRAME_NUM;                  // 缓存数量，也就是说在缓存队列里保持多少张照片
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE; //数据流类型，对视频捕获设备应是V4L2_BUF_TYPE_VIDEO_CAPTURE
    req.memory = V4L2_MEMORY_MMAP;          // V4L2_MEMORY_MMAP 或 V4L2_MEMORY_USERPTR
    if (ioctl(fd, VIDIOC_REQBUFS, &req) == -1)
    {
        printf("request for my_buffers error\n");
        return FALSE;
    }

    my_buffers = malloc(req.count * sizeof(*my_buffers));//申请几个指针
    if (!my_buffers)
    {
        printf("out of memory!\n");
        return FALSE;
    }

    for (unsigned int i = 0; i < FRAME_NUM; i++) // 进行内存映射
    {
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = i;
        if (ioctl(fd, VIDIOC_QUERYBUF, &buf) == -1) // buf[i]申请到内存
        {
            printf("query buffer error\n");
            return FALSE;
        }
        //mmap物理内存映射： 将帧缓冲区的地址映射到用户空间，这样就可以直接操作采集到的帧了，而不必去复制
        my_buffers[i].length = buf.length;
        my_buffers[i].start = mmap(NULL, buf.length, PROT_READ | PROT_WRITE, MAP_SHARED, fd, buf.m.offset);

        //显示缓存帧信息
        printf("Frame buffer %d: address=0x%p, length=%d\n", i, my_buffers[i].start, my_buffers[i].length);
        if (my_buffers[i].start == MAP_FAILED)
        {
            printf("buffer map error\n");
            return FALSE;
        }
    }
    return TRUE;
}

//VIDIOC_DQBUF  VIDIOC_QBUF
int v4l2_frame_process()
{
    unsigned int i;
    enum v4l2_buf_type type;
    char file_name[100];
    char index_str[10];
    long long int extra_time = 0;
    long long int cur_time = 0;
    long long int last_time = 0;

//将申请到的帧缓冲全部放入视频采集输入队列，以便存放采集的数据。
    for (i = 0; i < FRAME_NUM; i++)
    {
        buf.index = i;//buf[i]
        ioctl(fd, VIDIOC_QBUF, &buf);
    }

//开始视频流数据的采集。 ioctl (fd_v4l, VIDIOC_STREAMON, &type)
    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    ioctl(fd, VIDIOC_STREAMON, &type);

    int loop = 0;//出队，处理，写入yuv文件，入队，循环进行
    while (loop < 1)
    {
        for (i = 0; i < FRAME_NUM; i++)
        {
            //出队 buf[i]    应用程序从视频采集输出队列中取出已含有采集数据的帧缓冲区
            buf.index = i;
            ioctl(fd, VIDIOC_DQBUF, &buf);
            //查看采集数据的时间戳之差，单位为微妙
            my_buffers[i].timestamp = buf.timestamp.tv_sec * 1000000 + buf.timestamp.tv_usec;//us
            cur_time = my_buffers[i].timestamp;
            extra_time = cur_time - last_time;
            last_time = cur_time;
            printf("time_deta:%lld\n\n", extra_time);
            printf("buf_len:%d\n", my_buffers[i].length);

            //处理数据只是简单写入文件，名字以loop的次数和帧缓冲数目有关
            printf("grab image data OK\n");
            memset(file_name, 0, sizeof(file_name));
            memset(index_str, 0, sizeof(index_str));
            sprintf(index_str, "%d", loop * 4 + i);
            sprintf(file_name, IMAGE"%s.jpg", index_str); //strcat(file_name,".yuv");
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
            fwrite(my_buffers[i].start, IMAGEHEIGHT * IMAGEWIDTH * 2, 1, file);
            fclose(file);
            printf("save %s OK\n", file_name);

//处理完后，应用程序的将该帧缓冲区重新排入输入队列,这样便可以循环采集数据
            ioctl(fd, VIDIOC_QBUF, &buf); //放回帧
        }

        loop++;
    }
    return TRUE;
}

int v4l2_release()
{

//停止视频的采
    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    ioctl(fd, VIDIOC_STREAMOFF, &type);

//释放申请的视频帧缓冲区 unmap，
    for (unsigned int i = 0; i < FRAME_NUM; i++)
    {
        munmap(my_buffers[i].start, my_buffers[i].length);
    }
    free(my_buffers);

//关闭视频设备文件 close(fd_v4l)。
    close(fd);
    return TRUE;
}

//int v4l2_video_input_output()
//{
//    struct v4l2_input input;
//    struct v4l2_standard standard;
//
//    //首先获得当前输入的index,注意只是index，要获得具体的信息，就的调用列举操作
//    memset(&input, 0, sizeof(input));
//    if (-1 == ioctl(fd, VIDIOC_G_INPUT, &input.index))
//    {
//        printf("VIDIOC_G_INPUT\n");
//        return FALSE;
//    }
//    //调用列举操作，获得 input.index 对应的输入的具体信息
//    if (-1 == ioctl(fd, VIDIOC_ENUMINPUT, &input))
//    {
//        printf("VIDIOC_ENUM_INPUT \n");
//        return FALSE;
//    }
//    printf("Current input %s supports:\n", input.name);
//
//
//    //列举所有的所支持的 standard，如果 standard.id 与当前 input 的 input.std 有共同的
//    //bit flag，意味着当前的输入支持这个 standard,这样将所有驱动所支持的 standard 列举一个
//    //遍，就可以找到该输入所支持的所有 standard 了。
//
//    memset(&standard, 0, sizeof(standard));
//    standard.index = 0;
//    while (0 == ioctl(fd, VIDIOC_ENUMSTD, &standard))
//    {
//        if (standard.id & input.std)
//        {
//            printf("%s\n", standard.name);
//        }
//        standard.index++;
//    }
//    // EINVAL indicates the end of the enumeration, which cannot be empty unless this device falls under the USB exception.
//
//    if (errno != EINVAL || standard.index == 0)
//    {
//        printf("VIDIOC_ENUMSTD\n");
//        return FALSE;
//    }
//
//}


int main(int argc, char const *argv[])
{
    // sleep(10);
    //打开摄像头设备
    if ((fd = open(FILE_VIDEO, O_RDWR)) == -1)
    {
        printf("Error opening V4L interface\n");
        return FALSE;
    }

    query_capacity(fd);
    try_fmt(fd);
    set_fmt(fd);
    set_s_parm_g_parm(fd);
    printf("==============================\n");

    get_buffer();
    printf("malloc....\n");
    //sleep(10);

    v4l2_frame_process();
    printf("process....\n");

    v4l2_release();
    printf("release\n");

    return TRUE;
}
