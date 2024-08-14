NAME = fbwb
CC = gcc
CPPFLAGS = -Wall -Wextra -Werror
CPPFLAGS = -MMD -MP
CPPFLAGS += -I.
RM = rm

SRCS = fbwb.c
OBJS = $(SRCS:.c=.o)
DEPS = $(SRCS:.c=.d)

%.o: %.c
	$(COMPILE.c) $(OUTPUT_OPTION) $<

$(NAME): $(OBJS)
	$(LINK.c) $^ $(LOADLIBS) $(LDLIBS) $(OUTPUT_OPTION)

-include $(DEPS)

.PHONY: all clean fclean re

all: $(NAME)

clean:
	$(RM) $(OBJS) $(DEPS)

fclean:
	$(RM) -rf $(OBJS) $(DEPS) $(NAME)

re: fclean
	$(MAKE) all
