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
���õ�VIDIOC���
?1. VIDIOC_QUERYCAP ����ѯ�豸���ԣ�
?2. VIDIOC_ENUM_FMT (��ʾ����֧�ֵĸ�ʽ)
?3. VIDIOC_S_FMT ��������Ƶ�����ʽ��
?4. VIDIOC_G_FMT ����ȡӲ�����ڵ���Ƶ�����ʽ��
?5. VIDIOC_TRY_FMT ������Ƿ�֧��ĳ��֡��ʽ��
?6. VIDIOC_ENUM_FRAMESIZES ��ö���豸֧�ֵķֱ�����Ϣ��
?7. VIDIOC_ENUM_FRAMEINTERVALS (��ȡ�豸֧�ֵ�֡���)
?8. VIDIOC_S_PARM && VIDIOC_G_PARM (���úͻ�ȡ������)
?9. VIDIOC_QUERYCAP (��ѯ�������޼�����)
?10. VIDIOC_S_CROP (������Ƶ�źŵı߿�)
?11. VIDIOC_G_CROP (��ȡ�豸�źŵı߿�)
?12. VIDIOC_REQBUFS (���豸���뻺����)
?13. VIDIOC_QUERYBUF (��ȡ����֡�ĵ�ַ������)
?14. VIDIOC_QBUF (��֡�������)
?15. VIDIOC_DQBUF (�Ӷ�����ȡ��֡)
?16. VIDIOC_STREAMON && VIDIOC_STREAMOFF (����/ֹͣ��Ƶ������)
*/

int v4l2_init()
{
    struct v4l2_capability cap;
    struct v4l2_fmtdesc fmtdesc;
    struct v4l2_format fmt;
    struct v4l2_streamparm stream_para;

    //������ͷ�豸
    if ((fd = open(FILE_VIDEO, O_RDWR)) == -1)
    {
        printf("Error opening V4L interface\n");
        return FALSE;
    }

    //��ѯ�豸����
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
          __u8  driver[16];        // ��������
          __u8  card[32];          // �豸����
          __u8  bus_info[32];      // �豸��ϵͳ�е�λ��
          __u32 version;           // �����汾��
          __u32 capabilities;      // �豸֧�ֵĲ���
          __u32 reserved[4];       // �����ֶ�
        };
        */
        printf("driver:\t\t%s\n", cap.driver);
        printf("card:\t\t%s\n", cap.card);
        printf("bus_info:\t%s\n", cap.bus_info);
        printf("version:\t%d\n", cap.version);
        printf("capabilities:\t%x\n", cap.capabilities);
//��ʵ�ʲ��������У����Խ�ȡ�õ� capabilites ����Щ�������Զ�����ж��豸�Ƿ�֧����Ӧ�Ĺ��ܡ�

//V4L2_CAP_VIDEO_CAPTURE �Ƿ�֧��ͼ���ȡ
        if ((cap.capabilities & V4L2_CAP_VIDEO_CAPTURE) == V4L2_CAP_VIDEO_CAPTURE)
        {
            printf("Device %s: Is a video capture device.\n", FILE_VIDEO);
        }

// ����豸֧�� video output �Ľӿڣ�������豸�߱� video output �Ĺ���
        if ((cap.capabilities & V4L2_CAP_VIDEO_OUTPUT) == V4L2_CAP_VIDEO_OUTPUT)
        {
            printf("Device %s: Is a video output device.\n", FILE_VIDEO);
        }

// ����豸֧�� video overlay �Ľӿڣ�������豸�߱� video overlay �Ĺ��ܣ�
//imag ����Ƶ�豸�� meomory �б��棬��ֱ������Ļ����ʾ��������Ҫ���������Ĵ���
        if ((cap.capabilities & V4L2_CAP_VIDEO_OVERLAY) == V4L2_CAP_VIDEO_OVERLAY)
        {
            printf("Device %s: Can do video overlay.\n", FILE_VIDEO);
        }

// ����豸�Ƿ�֧�� read() �� write() I/O ��������
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

// ����豸�Ƿ�֧�� streaming I/O ��������
        if ((cap.capabilities & V4L2_CAP_STREAMING) == V4L2_CAP_STREAMING)
        {
            printf("Device %s: supports streaming.\n", FILE_VIDEO);
        }
    }

    /*
    //��صĽṹ�壺
    struct v4l2_fmtdesc
    {
      __u32  index;                // Ҫ��ѯ�ĸ�ʽ��ţ�Ӧ�ó�������
      enum   v4l2_buf_type type;   // ֡���ͣ�Ӧ�ó�������
      __u32  flags;                // �Ƿ�Ϊѹ����ʽ
      __u8   description[32];      // ��ʽ����
      __u32  pixelformat;          // ��ʽ
      __u32  reserved[4];         // ��������ʹ������Ϊ0
    };
    ����������
    ??1. index ��������ȷ����ʽ��һ������������������V4L2��ʹ�õ�����һ�������Ҳ�Ǵ�0��ʼ���������������ֵΪֹ��Ӧ�ÿ���ͨ��һֱ��������ֵ֪������-EINVAL�ķ�ʽö������֧�ֵĸ�ʽ��
    ??2. type�����������������͵��ֶΡ�������Ƶ�����豸������ͷ���г������˵������V4L2_BUF_TYPE_VIDEO_CAPTURE��
    ??3. flags: ֻ������һ��ֵ����V4L2_FMT_FLAG_COMPRESSED����ʾ����һ��ѹ������Ƶ��ʽ��
    ??4. description: �Ƕ������ʽ��һ�ּ�̵��ַ���������
    ??5. pixelformat: Ӧ��������Ƶ���ַ�ʽ�����ַ���
    */
    //��ʾ����֧��֡��ʽ
    fmtdesc.index = 0;
    fmtdesc.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    printf("Support format:\n");
    while (ioctl(fd, VIDIOC_ENUM_FMT, &fmtdesc) != -1)
    {
        printf("\t%d.%s\n", fmtdesc.index + 1, fmtdesc.description);
        fmtdesc.index++;
    }

    //VIDIOC_TRY_FMT ����Ƿ�֧��ĳ֡��ʽ
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

    // VIDIO_S_FMT ��������������Ƶ��׽��ʽ��
    printf("set fmt...\n");
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_RGB32; //jpg��ʽ V4L2_PIX_FMT_RGB32 V4L2_PIX_FMT_YUYV;//yuv��ʽ
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

    //.VIDIOC_G_FMT ��ȡӲ�����ڵ���Ƶ�����ʽ
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

    //���ü��鿴֡���ʣ�����ֻ����30֡������1��ɼ�30��ͼ
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

    //����֡����   VIDIOC_REQBUFS ���豸���뻺����
    req.count = FRAME_NUM;                  // ����������Ҳ����˵�ڻ�������ﱣ�ֶ�������Ƭ
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE; //���������ͣ�����Ƶ�����豸Ӧ��V4L2_BUF_TYPE_VIDEO_CAPTURE
    req.memory = V4L2_MEMORY_MMAP;          // V4L2_MEMORY_MMAP �� V4L2_MEMORY_USERPTR
    if (ioctl(fd, VIDIOC_REQBUFS, &req) == -1)
    {
        printf("request for buffers error\n");
        return FALSE;
    }

    // �����û��ռ�ĵ�ַ��
    buffers = malloc(req.count * sizeof(*buffers));
    if (!buffers)
    {
        printf("out of memory!\n");
        return FALSE;
    }

    // �����ڴ�ӳ��
    for (n_buffers = 0; n_buffers < FRAME_NUM; n_buffers++)
    {
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = n_buffers;
        //��ѯ ȡ����֡�ĵ�ַ������
        if (ioctl(fd, VIDIOC_QUERYBUF, &buf) == -1)
        {
            printf("query buffer error\n");
            return FALSE;
        }

        //ӳ�� ��bufӳ�䵽�ڴ� buffers �û��ռ��һ���ڴ�����ֱ��ӳ�䵽�ں˿ռ䡣����Ч�ʸ�
        buffers[n_buffers].length = buf.length;
        buffers[n_buffers].start = mmap(NULL, buf.length, PROT_READ | PROT_WRITE, MAP_SHARED, fd, buf.m.offset);

        //��ʾ����֡��Ϣ
        printf("Frame buffer %d: address=0x%p, length=%d\n", n_buffers, buffers[n_buffers].start, buffers[n_buffers].length);
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

    //��ӺͿ����ɼ�
    for (n_buffers = 0; n_buffers < FRAME_NUM; n_buffers++)
    {
        buf.index = n_buffers;
        ioctl(fd, VIDIOC_QBUF, &buf); //��֡������� ������
    }
    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    ioctl(fd, VIDIOC_STREAMON, &type);

    //���ӣ�����д��yuv�ļ�����ӣ�ѭ������
    int loop = 0;
    while (loop < 6)
    {
        for (n_buffers = 0; n_buffers < FRAME_NUM; n_buffers++)
        {
            //����
            buf.index = n_buffers;
            ioctl(fd, VIDIOC_DQBUF, &buf); //ȡ��֡

            //�鿴�ɼ����ݵ�ʱ���֮���λΪ΢��
            buffers[n_buffers].timestamp = buf.timestamp.tv_sec * 1000000 + buf.timestamp.tv_usec;
            cur_time = buffers[n_buffers].timestamp;
            extra_time = cur_time - last_time;
            last_time = cur_time;
            printf("time_deta:%lld\n\n", extra_time);
            printf("buf_len:%d\n", buffers[n_buffers].length);

            //��������ֻ�Ǽ�д���ļ���������loop�Ĵ�����֡������Ŀ�й�
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
            ��1��buffer����һ��ָ�룬��fwrite��˵����Ҫ��ȡ���ݵĵ�ַ��
            ��2��size��Ҫд�����ݵĵ��ֽ�����
            ��3��count:Ҫ����д��size�ֽڵ�������ĸ�����
            ��4��stream:Ŀ���ļ�ָ�룻
            ��5������ʵ��д������������count��

            */

            fwrite(buffers[n_buffers].start, IMAGEHEIGHT * IMAGEWIDTH * 2, 1, file);
            fclose(file);
            printf("save %s OK\n", file_name);

            //���ѭ��
            ioctl(fd, VIDIOC_QBUF, &buf); //�Ż�֡
        }

        loop++;
    }
    return TRUE;
}

int v4l2_release()
{
    unsigned int n_buffers;
    enum v4l2_buf_type type;

    //�ر���
    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    ioctl(fd, VIDIOC_STREAMON, &type);

    //�ر��ڴ�ӳ��
    for (n_buffers = 0; n_buffers < FRAME_NUM; n_buffers++)
    {
        munmap(buffers[n_buffers].start, buffers[n_buffers].length);
    }

    //�ͷ��Լ�������ڴ�
    free(buffers);

    //�ر��豸
    close(fd);
    return TRUE;
}

int v4l2_video_input_output()
{
    struct v4l2_input input;
    struct v4l2_standard standard;

    //���Ȼ�õ�ǰ�����index,ע��ֻ��index��Ҫ��þ������Ϣ���͵ĵ����оٲ���
    memset(&input, 0, sizeof(input));
    if (-1 == ioctl(fd, VIDIOC_G_INPUT, &input.index))
    {
        printf("VIDIOC_G_INPUT\n");
        return FALSE;
    }
    //�����оٲ�������� input.index ��Ӧ������ľ�����Ϣ
    if (-1 == ioctl(fd, VIDIOC_ENUMINPUT, &input))
    {
        printf("VIDIOC_ENUM_INPUT \n");
        return FALSE;
    }
    printf("Current input %s supports:\n", input.name);


    //�о����е���֧�ֵ� standard����� standard.id �뵱ǰ input �� input.std �й�ͬ��
    //bit flag����ζ�ŵ�ǰ������֧����� standard,����������������֧�ֵ� standard �о�һ��
    //�飬�Ϳ����ҵ���������֧�ֵ����� standard �ˡ�

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
