#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <fcntl.h>
#include <assert.h>
#include <errno.h>
#include <termios.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <pthread.h>
#include <sys/time.h>
#include <signal.h>
#include "../include/liblcd.h"

#define  uchar  unsigned char
#define  uint   unsigned int
#define  ulong   unsigned long
#define msleep(x) usleep(x*1000)
#define UART_DEVICE     "/dev/ttyS1"
#define ATOM_PCIE_NAME "/dev/fpga_pcie"

#define BLOCK_SIZE      3

#define MAX_COLUMN      (21)
#define MAX_STRINGLEN   (1024)

#define TIME2TICK(TM)   ((long)(TM).tv_usec/1000 + (TM).tv_sec*1000)

#define COMMON_EREAD        -2001
#define COMMON_EWRITE       -2002

pthread_t* work_thread = NULL;
pthread_t* work_thread_button = NULL;
int initialized = 0;
int thread_quit = 0;
int thread_button_quit = 0;
int need_on_off = 0;

char g_scrbuffer[MAX_LINES*MAX_COLUMN+1] = {0};
char g_scrbuffer_old[MAX_LINES*MAX_COLUMN+1] = {0};
int g_roll_interval = 500;
int button_interval = 30000;
long button_expired = 0;
int g_fd = -1;
int pin_fd = -1;
typedef struct {
    char mode;
    char disp;
    char src[MAX_STRINGLEN+1];
    long expired;
    int  hd_offset;
    pthread_mutex_t lock;
} LINEBUFFER;
LINEBUFFER g_linebuffer[MAX_LINES];

typedef struct {
    unsigned long index;
    unsigned long data;
} iomsg_t, * piomsg_t;
#define GENERIC_PCIE_TYPE                0xA8
#define IOCTL_READ_ULONG                 _IOWR(GENERIC_PCIE_TYPE,1,piomsg_t)
#define IOCTL_WRITE_ULONG                _IOWR(GENERIC_PCIE_TYPE,2,piomsg_t)

void putbyte(char byte)
{
    write(g_fd, &byte, 1);
}

void putstring(char* string)
{
    for(int i=0; i<strlen(string); i++) {
        putbyte(string[i]);
    }
    usleep(10*1000);
}


int led_set_off (void)
{
    if (pin_fd < 0)
        return 0;
    else {
        int rv = -1;
        iomsg_t iomsg;
        memset (&iomsg, 0, sizeof (iomsg_t));
        iomsg.index = 2;
        iomsg.data = 1;
        rv = ioctl (pin_fd, IOCTL_WRITE_ULONG, &iomsg);
        if (rv) {
            return COMMON_EWRITE;
        }
        iomsg.index = 3;
        iomsg.data = 1;
        rv = ioctl (pin_fd, IOCTL_WRITE_ULONG, &iomsg);
        if (rv) {
            return COMMON_EWRITE;
        }
        return 0;
    }
}

int led_set_red (void)
{

    if (pin_fd < 0)
        return 0;
    else {
        int rv = -1;
        iomsg_t iomsg;
        memset (&iomsg, 0, sizeof (iomsg_t));
        iomsg.index = 2;
        iomsg.data = 0;
        rv = ioctl (pin_fd, IOCTL_WRITE_ULONG, &iomsg);
        if (rv) {
            return COMMON_EWRITE;
        }
        iomsg.index = 3;
        iomsg.data = 1;
        rv = ioctl (pin_fd, IOCTL_WRITE_ULONG, &iomsg);
        if (rv) {
            return COMMON_EWRITE;
        }
        return 0;
    }
}

int led_set_blue (void)
{

    if (pin_fd < 0)
        return 0;
    else {
        int rv = -1;
        iomsg_t iomsg;
        memset (&iomsg, 0, sizeof (iomsg_t));
        iomsg.index = 2;
        iomsg.data = 1;
        rv = ioctl (pin_fd, IOCTL_WRITE_ULONG, &iomsg);
        if (rv) {
            return COMMON_EWRITE;
        }
        iomsg.index = 3;
        iomsg.data = 0;
        rv = ioctl (pin_fd, IOCTL_WRITE_ULONG, &iomsg);
        if (rv) {
            return COMMON_EWRITE;
        }
        return 0;
    }
}

int led_set_red_and_blue (void)
{

    if (pin_fd < 0)
        return 0;
    else {
        int rv = -1;
        iomsg_t iomsg;
        memset (&iomsg, 0, sizeof (iomsg_t));
        iomsg.index = 2;
        iomsg.data = 0;
        rv = ioctl (pin_fd, IOCTL_WRITE_ULONG, &iomsg);
        if (rv) {
            return COMMON_EWRITE;
        }
        iomsg.index = 3;
        iomsg.data = 0;
        rv = ioctl (pin_fd, IOCTL_WRITE_ULONG, &iomsg);
        if (rv) {
            return COMMON_EWRITE;
        }
        return 0;

    }
}

/*
 * * return value
 * * 1: button is press
 * * 0: button is not press
 * * <0: something error
 * * */
#if 0
int button_get_stat (void)
{

    if (pin_fd < 0)
        return 0;
    else {
        int rv = -1;
        int i = 50;
        iomsg_t iomsg;
        memset (&iomsg, 0, sizeof (iomsg_t));

        iomsg.index = 23;
        int is_press_flag = 0;

        while (i--) {
            rv = ioctl (pin_fd, IOCTL_READ_ULONG, &iomsg);
            if (rv) {
                return -1;

            } else {
                if (iomsg.data) {
                    is_press_flag = 0;
                } else {
                    is_press_flag++;
                }

            }
            usleep (1000);
        }
        if (is_press_flag >= 49) {
            return 1;
        } else {
            return 0;
        }

    }
}
#else
int button_get_stat (void)
{
    if (pin_fd < 0)
        return 0;

    int rv = -1;
    iomsg_t iomsg;
    memset (&iomsg, 0, sizeof (iomsg_t));

    iomsg.index = 23;

    rv = ioctl(pin_fd, IOCTL_READ_ULONG, &iomsg);
    return((rv == 0) && (iomsg.data == 0));
}
#endif

void lcd_set_backlight_int(int ms)
{
    button_interval = ms;
}

char serial_init(char* dev_name)
{
    int result = -1;
    struct termios ts;

    g_fd = open(dev_name, O_RDWR/*|O_NONBLOCK*/);
    if(g_fd < 0) {
        printf("Failed to open device: dev_name=%s\n", dev_name);
        return -1;
    }

    result = tcgetattr(g_fd, &ts);
    if(result) {
        printf("tcgetattr: ret=%d\n", result);
        return -1;
    }

    tcflush(g_fd, TCIOFLUSH);

    result = cfsetispeed(&ts, B9600);
    if(result) {
        printf("cfsetispeed: ret=%d\n", result);
        return -1;
    }

    result = cfsetospeed(&ts, B9600);
    if(result) {
        printf("cfsetospeed: ret=%d\n", result);
        return -1;
    }

    ts.c_cflag &= ~(CSIZE | PARENB | CRTSCTS );
    ts.c_iflag &= ~(IXON | IXOFF | IXANY | INLCR | ICRNL | INPCK);
    ts.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG | NOFLSH);
    ts.c_oflag &= ~(OPOST | ONLCR | OCRNL);

    ts.c_cflag |= (CREAD | CS8);
    tcflush(g_fd, TCIFLUSH);
    ts.c_cc[VTIME] = 0;
    ts.c_cc[VMIN] = BLOCK_SIZE;
    tcsetattr(g_fd, TCSANOW, &ts);

    return 0;
}

void lcd_set_mode(char mode)
{
    tcflush(g_fd, TCIOFLUSH);
    putbyte(0x1b);
    putbyte(0x99);
    putbyte(mode);
    sleep(1);
    lcd_cls();
}

void lcd_set_grayscale(char level)
{
    usleep(200*1000); 
    tcflush(g_fd, TCIOFLUSH);
    if(level > 0x3f)
        return;
    putbyte(0x1b);
    putbyte(0x31);
    putbyte(level);
    usleep(100*1000); 
}

void lcd_cls(void)
{
    tcflush(g_fd, TCIOFLUSH);
    memset(g_scrbuffer_old, 0, sizeof(g_scrbuffer_old));
    putbyte(0x1b);
    putbyte(0x32);
}

void lcd_move_cursor(char cx, char cy)
{
    tcflush(g_fd, TCIOFLUSH);
    putbyte(0x1b);
    putbyte(0x33);
    putbyte(cx);
    putbyte(cy-1);
}

void lcd_show_cursor(bool onoff)
{
    usleep(200*1000); 
    tcflush(g_fd,TCIOFLUSH);
    putbyte(0x1b);
    putbyte(0x34);
    putbyte(onoff);
    usleep(100*1000); 
}

void lcd_set_backlight(bool onoff)
{
    usleep(200*1000); 
    tcflush(g_fd,TCIOFLUSH);
    putbyte(0x1b);
    putbyte(0x25);
    putbyte(!onoff);
    usleep(100*1000); 
}

void lcd_set_roll_int(int ms)
{
    g_roll_interval = ms;
}

void lcd_draw_string(bool inverse, char *string)
{
    tcflush(g_fd,TCIOFLUSH);
    putbyte(0x1b);
    putbyte(0x37);
    putbyte(inverse);
    putstring(string);
    putbyte(0x00);
}

void lcd_draw_dot(bool onoff, char x, char y)
{
    tcflush(g_fd, TCIOFLUSH);
    putbyte(0x1b);
    putbyte(0x38);
    putbyte(onoff);
    putbyte(x);
    putbyte(y);
}

void lcd_draw_line(bool onoff, char x0, char y0, char x1, char y1)
{
    tcflush(g_fd, TCIOFLUSH);
    putbyte(0x1b);
    putbyte(0x39);
    putbyte(onoff);
    putbyte(x0);
    putbyte(y0);
    putbyte(x1);
    putbyte(y1);
}

void lcd_draw_circle(bool onoff, char ox, char oy, char rx)
{
    tcflush(g_fd, TCIOFLUSH);
    putbyte(0x1b);
    putbyte(0x41);
    putbyte(onoff);
    putbyte(ox);
    putbyte(oy);
    putbyte(rx);
}

void lcd_draw_linestr(char cy, char disp, char roll, char* string)
{
    int line = cy-1;

    if (line < 0 || line >= MAX_LINES)
        return;

    if (strlen(string) > MAX_STRINGLEN)
        return;

    pthread_mutex_lock(&g_linebuffer[line].lock);

    sprintf(g_linebuffer[line].src, "%-21s", string);
    g_linebuffer[line].mode = roll;
    g_linebuffer[line].disp = disp;
    g_linebuffer[line].expired = 0;
    pthread_mutex_unlock(&g_linebuffer[line].lock);
}

void lcd_erase_linestr(char cy)
{
    int line = cy-1;

    if (line < 0 || line >= MAX_LINES)
        return;
    pthread_mutex_lock(&g_linebuffer[line].lock);
    g_linebuffer[line].src[0] = '\0';
    pthread_mutex_unlock(&g_linebuffer[line].lock);
}

static int get_next_offset(int offset, int wrap)
{
    int ret = offset + 1;
    if (ret >= wrap) {
        ret = 0;
    }
    return(ret);
}

static int get_prev_offset(int offset, int wrap)
{
    int ret = offset - 1;
    if (ret < 0) {
        ret = wrap-1;
    }
    return(ret);
}



void *__work_thread()
{
    struct timeval now;
    int i, j;
    int need_refresh = 0;
    long tick;
    int cpy_offset;

    while(!thread_quit) {
        gettimeofday(&now, NULL);
        tick = TIME2TICK(now);

        need_refresh = 0;
        for(i=0; i<MAX_LINES; i++) {
            if(!strlen(g_linebuffer[i].src))
                continue;

            if (tick > g_linebuffer[i].expired) {
                pthread_mutex_lock(&g_linebuffer[i].lock);
                int srcstr_len = strlen(g_linebuffer[i].src);

                cpy_offset = g_linebuffer[i].hd_offset;
                for(j=0; j<MAX_COLUMN; j++) {
                    char copy_char;
                    copy_char = g_linebuffer[i].src[cpy_offset];

                    if (copy_char < 0) {
                        // chinese
                        if (MAX_COLUMN-j >= 2) {
                            g_scrbuffer[i*MAX_COLUMN+j] = copy_char;
                            cpy_offset = get_next_offset(cpy_offset, srcstr_len);

                            copy_char = g_linebuffer[i].src[cpy_offset];
                            g_scrbuffer[i*MAX_COLUMN+j+1] = copy_char;
                            cpy_offset = get_next_offset(cpy_offset, srcstr_len);
                            j++;
                        } else {
                            g_scrbuffer[i*MAX_COLUMN+j] = ' ';
                        }
                    } else {
                        g_scrbuffer[i*MAX_COLUMN+j] = copy_char;
                        cpy_offset = get_next_offset(cpy_offset, srcstr_len);
                    }
                }
                need_refresh = 1;
                switch(g_linebuffer[i].mode) {
                case LCD_ROLL_L2R:
                    g_linebuffer[i].hd_offset = get_prev_offset(g_linebuffer[i].hd_offset, srcstr_len);
                    if (g_linebuffer[i].src[g_linebuffer[i].hd_offset] < 0)
                        g_linebuffer[i].hd_offset = get_prev_offset(g_linebuffer[i].hd_offset, srcstr_len);
                    break;
                case LCD_ROLL_R2L:
                    if (g_linebuffer[i].src[g_linebuffer[i].hd_offset] < 0) {
                        g_linebuffer[i].hd_offset = get_next_offset(g_linebuffer[i].hd_offset, srcstr_len);
                    }
                    g_linebuffer[i].hd_offset = get_next_offset(g_linebuffer[i].hd_offset, srcstr_len);
                    break;
                default:
                    break;
                }

                g_linebuffer[i].expired = tick + g_roll_interval;
                pthread_mutex_unlock(&g_linebuffer[i].lock);
            }
        }

        if (need_on_off && button_expired == 0) {
            lcd_set_backlight(1);
            button_expired = tick + button_interval;
        } 
        
        if ((button_expired > 0) && (tick > button_expired)) {
            lcd_set_backlight(0);
            button_expired = 0;
        }

        if (need_refresh && strcmp(g_scrbuffer_old, g_scrbuffer) != 0) {
            lcd_move_cursor(0, 1);
            usleep(50*1000);
//printf("###%s###\n", g_scrbuffer);
            lcd_draw_string(0, g_scrbuffer);
	    strcpy(g_scrbuffer_old, g_scrbuffer);
        }
        usleep(100*1000);
    }

    return(NULL);
}


void *__work_thread_button()
{
    while(!thread_button_quit) {
        need_on_off = button_get_stat();
        usleep(100);
    }
    return(NULL);
}

void lcd_init(void)
{
    if (initialized) {
        printf("lcd_init: already initialized.\n");
        return;
    }

    serial_init(UART_DEVICE);
    pin_fd = open(ATOM_PCIE_NAME, O_RDWR);
    if (pin_fd < 0) {
        printf("Can not open the board.\n");
        return;

    }
    lcd_cls();
    memset(g_scrbuffer, 0x20, sizeof(g_scrbuffer));
    g_scrbuffer[sizeof(g_scrbuffer)-1] = '\0';

    for(int i=0; i<MAX_LINES; i++) {
        memset(&g_linebuffer[i], 0, sizeof(LINEBUFFER));
        pthread_mutex_init(&g_linebuffer[i].lock, 0);
    }


    if (!work_thread) {
        thread_quit = 0;
        work_thread = malloc(sizeof(pthread_t));
        pthread_create(work_thread, NULL, __work_thread, NULL);
    }

    if (!work_thread_button) {
        thread_button_quit = 0;
        work_thread_button = malloc(sizeof(pthread_t));
        pthread_create(work_thread_button, NULL, __work_thread_button, NULL);
    }
}


void lcd_term(void)
{
    thread_quit = 1;
    thread_button_quit = 1;
    if (work_thread) {
        pthread_join(*work_thread, NULL);
        free(work_thread);
        work_thread = NULL;
    }

    if (work_thread_button) {
        pthread_join(*work_thread_button, NULL);
        free(work_thread_button);
        work_thread_button = NULL;
    }

    int n = close(pin_fd);
    if (n < 0) {
        printf("pin_fd return error!\n");
        return;
    }
}

