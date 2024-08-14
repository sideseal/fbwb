#ifndef FBWB_H
# define FBWB_H

# include <fcntl.h>
# include <linux/fb.h>
# include <stdio.h>
# include <stdlib.h>
# include <string.h>
# include <signal.h>
# include <sys/ioctl.h>
# include <sys/mman.h>
# include <sys/stat.h>
# include <syslog.h>
# include <time.h>
# include <unistd.h>

# include "font8x8.h"

# define SUCCESS 1
# define FAILURE 0
# define TRUE 1
# define FALSE 0

typedef struct s_fb
{
	unsigned char*				frame_buf;
	unsigned char*				local_buf;
	int							fd;
	struct fb_var_screeninfo	info;
	unsigned int				size;
} t_fb;

#endif
