#include "fbwb.h"

volatile sig_atomic_t	running = TRUE;

void	log_message(char const* msg)
{
	syslog(LOG_INFO, "%s", msg);
}

void	error_exit(char const* msg)
{
	write(2, msg, strlen(msg));
	exit(EXIT_FAILURE);
}

int	daemonize(void)
{
	pid_t	pid;

	pid = fork();
	if (pid < 0)
		return FAILURE;

	if (pid > 0)
		exit(EXIT_SUCCESS);

	if (setsid() < 0)
		return FAILURE;

	signal(SIGCHLD, SIG_IGN);
	signal(SIGHUP, SIG_IGN);

	pid = fork();
	if (pid < 0)
		return FAILURE;

	if (pid > 0)
		exit(EXIT_SUCCESS);

	umask(0);

	chdir("/");

	printf("fbwb daemon is starting...\n");

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

void	scroll_up(t_fb* fb)
{
	memmove(fb->local_buf, fb->local_buf + fb->info.xres, (fb->info.yres - 8) * fb->info.xres);
	memset(fb->local_buf + (fb->info.yres - 8) * fb->info.xres, 0, fb->info.xres);
}

void	write_char(char* bitmap, unsigned int xpos, unsigned int ypos, t_fb* fb)
{
	int	x;
	int	y;
	int	set;

	for (y = 0; y < 8; y++)
	{
		for (x = 0; x < 8; x++)
		{
			set = bitmap[y] & (1 << x);
			if (set)
			{
				int location = ((xpos + x) + ((ypos + y) * fb->info.xres));
				fb->local_buf[location / 8] |= (1 << (location % 8));
			}
		}
	}
}

void	draw_sentence(char* sentence, t_fb* fb, unsigned int* xpos, unsigned int* ypos)
{
	if (!sentence)
		return;

	while (*sentence)
	{
		if (*sentence == '\n')
		{
			*xpos = 0;
			*ypos += 8;
			sentence++;
			continue;
		}
	
		if (*xpos >= fb->info.xres)
		{
			*xpos = 0;
			*ypos += 8;
		}

		if (*ypos >= fb->info.yres)
		{
			scroll_up(fb);
			*ypos -= 8;
		}

		write_char(font8x8_basic[(unsigned char)*sentence++], *xpos, *ypos, fb);
		*xpos += 8;
	}
}

void	display(char** result, t_fb* fb)
{
	unsigned int	xpos;
	unsigned int	ypos;

	xpos = 0;
	ypos = 0;
	while (*result)
	{
		draw_sentence(*result, fb, &xpos, &ypos);
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
		return FAILURE;

	temperature = 0.0;
	humidity = 0.0;
	while (fgets(temp, sizeof(temp), fp))
	{
		if (strstr(temp, "shtc1-i2c-0-70"))
		{
			fgets(temp, sizeof(temp), fp);
			if (fgets(temp, sizeof(temp), fp))
				sscanf(temp, "temp1: +%f°C", &temperature);
			if (fgets(temp, sizeof(temp), fp))
				sscanf(temp, "humidity1: %f %%RH", &humidity);
			break;
		}
	}

	pclose(fp);

	snprintf(result[0], 42, "\nTemp : %.1f C\n", temperature);
	snprintf(result[1], 42, "\nHumid: %.1f %%RH", humidity);
	
	return SUCCESS;
}

int	draw_background(t_fb* fb)
{
	unsigned int	i;
	unsigned int	j;
	unsigned int	bytes_per_pixel;
	unsigned int	location;

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
				return FAILURE;
		}
	}

	return SUCCESS;
}

void	display_end_screen(t_fb* fb)
{
	char				msg[17] = "Program stopped";
	unsigned int		x;
	unsigned int		y;

	x = 0;
	y = 0;
	draw_background(fb);
	draw_sentence(msg, fb, &x, &y);
	memcpy(fb->frame_buf, fb->local_buf, fb->size); // 힘이 빠진다..
}


int	display_sensor_info(t_fb* fb)
{
	int		flag;
	char**	sensor_info;

	sensor_info = malloc(sizeof(char*) * 3);
	if (!sensor_info)
		return FAILURE;
	
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
	
	flag = SUCCESS;
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

	if (argc < 2 || argc > 2)
		error_exit("Usage: ./fbwb <framebuffer path>\n");

	openlog("fbwb - display weather board with framebuffer", LOG_PID, LOG_DAEMON);

	fb.fd = open(argv[1], O_RDWR);
	if (fb.fd < 0)
		error_exit("Failed to open framebuffer\n");

	if (ioctl(fb.fd, FBIOGET_VSCREENINFO, &(fb.info)) < 0)
		error_exit("Failed to get framebuffer info\n");

	fb.size = fb.info.xres * fb.info.yres * fb.info.bits_per_pixel / 8;

	fb.frame_buf = mmap(0, fb.size, PROT_READ | PROT_WRITE, MAP_SHARED, fb.fd, 0);
	if (fb.frame_buf == MAP_FAILED)
		error_exit("Failed to map framebuffer\n");
	
	fb.local_buf = malloc(fb.size);
	if (!fb.local_buf)
	{
		fb.local_buf = NULL;
		clean_up(&fb);
		error_exit("Failed to malloc local buffer\n");
	}
	else
		memset(fb.local_buf, 0, fb.size);

	signal(SIGTERM, signal_handler);
	signal(SIGINT, signal_handler);

	if (!daemonize())
	{
		log_message("Failed to daemonize program\n");
		goto end;
	}
	else
		log_message("fbwb daemon is starting...");

	if (!display_sensor_info(&fb))
	{
		display_end_screen(&fb);
		log_message("Failed to display info\n");
		goto end;
	}

end:
	clean_up(&fb);
	closelog();

	return 0;
}
