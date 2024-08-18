NAME = fbwb
CC = gcc
CPPFLAGS = -Wall -Wextra -Werror
CPPFLAGS = -MMD -MP
CPPFLAGS += -I.

CP = cp
IF = if
RM = rm
SED = sed
CHMOD = chmod
CHOWN = chown
SYSTEMCTL = systemctl

SRCS = fbwb.c
OBJS = $(SRCS:.c=.o)
DEPS = $(SRCS:.c=.d)

SERVICE_NAME = $(NAME).service
SERVICE_USER = root
SERVICE_PATH = /etc/systemd/system
INSTALL_PATH = /usr/local/sbin
FRAMEBUFFER_PATH= /dev/fb0

%.o: %.c
	$(COMPILE.c) $(OUTPUT_OPTION) $<

$(NAME): $(OBJS)
	$(LINK.c) $^ $(LOADLIBS) $(LDLIBS) $(OUTPUT_OPTION)

-include $(DEPS)

.PHONY: all clean fclean re install uninstall

all: $(NAME)

clean:
	$(RM) -r $(OBJS) $(DEPS)

fclean:
	$(RM) -rf $(OBJS) $(DEPS) $(NAME)

re: fclean
	$(MAKE) all

install: all
	$(IF) [ -z "$(FRAMEBUFFER_PATH)" ]; then \
		echo "Error: FRAMEBUFFER_PATH required for install"; \
		exit 1; \
	fi
		
	$(CP) $(NAME) $(INSTALL_PATH)
	$(CHOWN) $(SERVICE_USER):$(SERVICE_USER) $(INSTALL_PATH)/$(NAME)

	$(CP) $(SERVICE_NAME) $(SERVICE_PATH)
	$(SED) -i 's|^ExecStart=.*|ExecStart=$(INSTALL_PATH)/$(NAME) $(FRAMEBUFFER_PATH)|' $(SERVICE_PATH)/$(SERVICE_NAME)
	$(CHOWN) $(SERVICE_USER):$(SERVICE_USER) $(SERVICE_PATH)/$(SERVICE_NAME)
	$(CHMOD) 644 $(SERVICE_PATH)/$(SERVICE_NAME)
	
	$(SYSTEMCTL) daemon-reload
	$(SYSTEMCTL) enable $(NAME)
	$(SYSTEMCTL) start $(NAME)
	$(SYSTEMCTL) status $(NAME) --no-pager

uninstall:
	$(SYSTEMCTL) stop $(NAME)
	$(SYSTEMCTL) disable $(NAME)
	$(RM) $(SERVICE_PATH)/$(SERVICE_NAME)
	$(SYSTEMCTL) daemon-reload
	$(SYSTEMCTL) reset-failed
	$(RM) $(INSTALL_PATH)/$(NAME)


