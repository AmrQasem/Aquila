#include <stdarg.h>
#include <stdint.h>

#define TIOCGPTN	0x80045430

#define SYS_FORK	2	
#define SYS_OPEN	3
#define SYS_READ	4
#define SYS_WRITE	5
#define SYS_IOCTL	6

int syscall(int s, long arg1, long arg2, long arg3)
{
	int ret;
	asm("int $0x80;":"=a"(ret):"a"(s), "b"(arg1), "c"(arg2), "d"(arg3));
	return ret;
}

int fork()
{
	return syscall(SYS_FORK, 0, 0, 0);
}

int open(const char *fn, int flags)
{
	return syscall(SYS_OPEN, fn, flags, 0);
}

int read(int fd, void *buf, int size)
{
	return syscall(SYS_READ, fd, buf, size);
}

int write(int fd, void *buf, int size)
{
	return syscall(SYS_WRITE, fd, buf, size);
}

int ioctl(int fd, unsigned long request, void *argp)
{
	return syscall(SYS_IOCTL, fd, request, argp);
}

char kbd_us[] = 
{
	'\0', 0, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
	'\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
	'\0', 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'',
	'\0', '\0', '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/',
	'\0', '\0', '\0', ' ',
};

int printf(char *fmt, ...);

void _start()
{
	open("/dev/kbd", 0);		/* stdin */
	open("/dev/console", 0);	/* stdout */
	open("/dev/console", 0);	/* stderr */

	int pty = open("/dev/ptmx", 0);

	int pid = fork();
	if(pid)	// parent
	{
		char buf[] = "Hello, TTY!";
		printf("parent write %d\n", write(pty, buf, 12));
	} else
	{
		int pts_id;
		ioctl(pty, TIOCGPTN, &pts_id);

		char pts_fn[] = "/dev/pts/ ";
		pts_fn[9] = '0' + pts_id;

		int pts_fd = open(pts_fn, 0);
		printf("child pts_fd %d\n", pts_fd);

		char buf[11] = {0};
		
		int r;
		while((r = read(pts_fd, buf, 12) != 12));
		
		printf("read %d buf %s\n", r, buf);
	}

	for(;;);
}

static int putc(char c)
{
	return write(1, &c, 1);
}

int strlen(char *s)
{
	int i;
	for(i = 0; s[i] != 0; ++i);
	return i;	
}

static int puts(char *s)
{
	return write(1, s, strlen(s));
}

static int putx(uint32_t val)
{
	char *enc = "0123456789ABCDEF";
	char buf[9];
	buf[8] = '\0';
	uint8_t i = 8;
	while(i)
	{
		buf[--i] = enc[val&0xF];
		val >>= 4;
	}
	return puts(buf);
}

static int putlx(uint64_t val)
{
	char *enc = "0123456789ABCDEF";
	char buf[17];
	buf[16] = '\0';
	uint8_t i = 16;
	while(i)
	{
		buf[--i] = enc[val&0xF];
		val >>= 4;
	}
	return puts(buf);
}

static int putud(uint32_t val)
{
	char buf[11];
	buf[10] = '\0';
	if(!val) { buf[9] = '0'; return puts(&buf[9]); }
	uint8_t i = 10;
	while(val)
	{
		buf[--i] = val%10 + '0';
		val = (val-val%10)/10;
	}
	return puts(buf+i);
}

static int putul(uint64_t val)
{
	char buf[21];
	buf[20] = '\0';
	uint8_t i = 20;
	while(val)
	{
		buf[--i] = val%10 + '0';
		val = (val-val%10)/10;
	}
	return puts(buf+i);
}

static int putb(uint8_t val)
{
	char buf[9];
	buf[8] = '\0';
	uint8_t i = 8;
	while(i)
	{
		buf[--i] = '0' + (val & 1);
		val >>= 1;
	}
	return puts(buf);
}

int vprintf(char *fmt, va_list args)
{
	int ret = 0;
	while(*fmt)
	switch(*fmt)
	{
		case '%':
			++fmt;
			switch(*fmt)
			{
				case 'c':	/* char */
					ret += putc((char)va_arg(args, int));
					break;
				case 's':	/* char * */
					ret += puts((char*)va_arg(args, char*));
					break;
				case 'd': /* decimal */
					ret += putud((uint32_t)va_arg(args, uint32_t));
					break;
				case 'l':	/* long */
					switch(*++fmt)
					{
						case 'x':	/* long hex */
							ret += putlx((uint64_t)va_arg(args, uint64_t));
							break;
						case 'd':
							ret += putul((uint64_t)va_arg(args, uint64_t));
							break;
						default:
							ret += putc(*--fmt);
					}
					break;
				
				case 'b': /* binary */
					ret += putb((uint8_t)(uint32_t)va_arg(args, uint32_t));
					break;
				case 'x': /* Hexadecimal */
					ret += putx((uint32_t)va_arg(args, uint32_t));
					break;
				default:
					ret += putc(*(--fmt));
			}
			++fmt;
			break;
		default:
			ret += putc(*fmt);
			++fmt;
	}

	return ret;
}

int printf(char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	int ret = vprintf(fmt, args);
	va_end(args);
	return ret;
}