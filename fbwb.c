#include "fbwb.h"

volatile sig_atomic_t	running = TRUE;

void	log_message(char const* msg)
{
	syslog(LOG_INFO, "%s", msg);
}

void	error_exit(char const* msg)
{
	fprintf(stderr, "%s: %s\n", msg, strerror(errno));
	exit(EXIT_FAILURE);
}

int	daemonize(void)
{
	pid_t	pid;

	pid = fork();
	if (pid < 0)
	{
		return FAILURE;
	}

	if (pid > 0)
	{
		exit(EXIT_SUCCESS);
	}

	if (setsid() < 0)
	{
		return FAILURE;
	}

	signal(SIGCHLD, SIG_IGN);
	signal(SIGHUP, SIG_IGN);

	pid = fork();
	if (pid < 0)
	{
		return FAILURE;
	}

	if (pid > 0)
	{
		exit(EXIT_SUCCESS);
	}

	chdir("/");

	printf("\nfbwb daemon is starting...\n");

	close(STDIN_FILENO);
	close(STDOUT_FILENO);
	close(STDERR_FILENO);

	return SUCCESS;
}

void	signal_handler(int sig)
{
	switch (sig)
	{
		case SIGTERM:
		case SIGINT:
			log_message("fbwb daemon is stopping...");
			running = FALSE;
			break;
		default:
			break;
	}
}

void	clean_up(t_fb* fb)
{
	munmap(fb->frame_buf, fb->size);
	free(fb->local_buf);
	close(fb->fd);
}

void	write_char_16x16(char* bitmap, unsigned int xpos, unsigned int ypos,
						 t_fb* fb)
{
	unsigned int	x;
	unsigned int	y;
	unsigned int	location;
	unsigned short	set;
	unsigned short	row;

	if (!bitmap || !fb)
	{
		return;
	}

	for (y = 0; y < FONT_HEIGHT; y++)
	{
		row = (bitmap[y * 2] << 8) | bitmap[y * 2 + 1];

		for (x = 0; x < FONT_WIDTH; x++)
		{
			set = row & (1 << (15 - x));
			if (set)
			{
				location = ((xpos + x) + ((ypos + y) * fb->info.xres));
				fb->local_buf[location / 8] |= (1 << (location % 8));
			}
		}
	}
}

void	write_sentence(char* sentence, unsigned int* xpos, unsigned int* ypos,
					  t_fb* fb)
{
	if (!sentence || !fb)
	{
		return;
	}

	while (*sentence)
	{
		if (*sentence == '\n')
		{
			*xpos = 0;
			*ypos += FONT_HEIGHT;
			sentence++;
			continue;
		}
	
		if (*xpos + FONT_WIDTH > fb->info.xres)
		{
			*xpos = 0;
			*ypos += FONT_HEIGHT;
		}

		if (*ypos + FONT_HEIGHT > fb->info.yres)
		{
			break;
		}

		write_char_16x16(font16x16_basic[(char)*sentence++], *xpos, *ypos, fb);
		*xpos += FONT_WIDTH;
	}
}

void	display(char** result, t_fb* fb)
{
	unsigned int	xpos;
	unsigned int	ypos;

	if (!result || !fb)
	{
		return;
	}

	xpos = 0;
	ypos = 0;
	while (*result)
	{
		write_sentence(*result, &xpos, &ypos, fb);
		result++;
	}

	memcpy(fb->frame_buf, fb->local_buf, fb->size);
}

int	get_sensor_info(char** result)
{
	FILE*	fp;
	char	temp[1024];
	float	temperature;
	float	humidity;

	fp = popen("sensors", "r");
	if (!fp)
	{
		return FAILURE;
	}

	temperature = 0.0;
	humidity = 0.0;
	while (fgets(temp, sizeof(temp), fp))
	{
		if (strstr(temp, "shtc1-i2c-0-70")) 
		{
			fgets(temp, sizeof(temp), fp);
			
			if (fgets(temp, sizeof(temp), fp))
			{
				sscanf(temp, "temp1: +%f °C", &temperature);
			}

			if (fgets(temp, sizeof(temp), fp))
			{
				sscanf(temp, "humidity1: %f %%RH", &humidity);
			}
			
			break;
		}
	}

	pclose(fp);

	snprintf(result[0], 42, "\n%.1f \'C\n", temperature);
	snprintf(result[1], 42, "\n%.1f %%RH\n", humidity);
	
	return SUCCESS;
}

int	draw_background(t_fb* fb)
{
	unsigned int	i;
	unsigned int	j;
	unsigned int	bytes_per_pixel;
	unsigned int	location;

	if (!fb)
	{
		return FAILURE;
	}

	bytes_per_pixel = (fb->info.bits_per_pixel + 7) / 8;

	for (i = 0; i < fb->info.yres; i++)
	{
		for (j = 0; j < fb->info.xres; j++)
		{
			location = (j + (i * fb->info.xres)) * bytes_per_pixel;

			if (bytes_per_pixel == 1)
			{
				fb->frame_buf[location / 8] = 0x0;
				fb->local_buf[location / 8] = 0x0;
			}
			else
			{
				return FAILURE;
			}
		}
	}

	return SUCCESS;
}

void	display_end_screen(t_fb* fb)
{
	char				msg[17] = "\nProgram stopped";
	unsigned int		x;
	unsigned int		y;

	x = 0;
	y = 0;
	draw_background(fb);
	write_sentence(msg, &x, &y, fb);
	memcpy(fb->frame_buf, fb->local_buf, fb->size);
}


int	display_sensor_info(t_fb* fb)
{
	int		flag;
	char**	sensor_info;

	sensor_info = malloc(sizeof(char*) * 3);
	if (!sensor_info)
	{
		return FAILURE;
	}
	
	sensor_info[0] = malloc(sizeof(char) * 42);
	if (!sensor_info[0])
	{
		free(sensor_info);
		return FAILURE;
	}

	sensor_info[1] = malloc(sizeof(char) * 42);
	if (!sensor_info[1])
	{
		free(sensor_info[0]);
		free(sensor_info);
		return FAILURE;
	}

	sensor_info[2] = NULL;
	
	flag = TRUE;
	while (running)
	{
		if (!draw_background(fb))
		{
			flag = FAILURE;
			break;
		}

		if (!get_sensor_info(sensor_info))
		{
			flag = FAILURE;
			break;
		}

		display(sensor_info, fb);
		sleep(1);
	}

	free(sensor_info[1]);
	free(sensor_info[0]);
	free(sensor_info);

	return running & flag;
}

int	main(int argc, char* argv[])
{
	t_fb	fb;

	if (argc != 2)
	{
		error_exit("Usage: ./fbwb <framebuffer path>");
	}

	fb.fd = open(argv[1], O_RDWR);
	if (fb.fd < 0)
	{
		error_exit("Failed to open framebuffer");
	}

	if (ioctl(fb.fd, FBIOGET_VSCREENINFO, &(fb.info)) < 0)
	{
		error_exit("Failed to get framebuffer info");
	}

	fb.size = fb.info.xres * fb.info.yres * fb.info.bits_per_pixel / 8;

	fb.frame_buf = mmap(0, fb.size, PROT_READ | PROT_WRITE, MAP_SHARED, fb.fd, 0);
	if (fb.frame_buf == MAP_FAILED)
	{
		error_exit("Failed to map framebuffer");
	}
	
	fb.local_buf = malloc(fb.size);
	if (!fb.local_buf)
	{
		fb.local_buf = NULL;
		clean_up(&fb);
		error_exit("Failed to malloc local buffer");
	}
	else
	{
		memset(fb.local_buf, 0, fb.size);
	}

	openlog("fbwb - display weather board with framebuffer", LOG_PID,
			LOG_DAEMON);

	signal(SIGTERM, signal_handler);
	signal(SIGINT, signal_handler);

	if (!daemonize())
	{
		log_message("Failed to daemonize program\n");
		goto end;
	}
	else
	{
		log_message("fbwb daemon is starting...");
	}

	if (!display_sensor_info(&fb))
	{
		display_end_screen(&fb);
		log_message("Failed to display info\n");
	}

end:
	clean_up(&fb);
	closelog();

	return 0;
}
