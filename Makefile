TARGET		:= ft_ls

SRC_FILES	:= main.c
OBJ_FILES	:= main.o

CFLAGS		:= -Wall -Wextra -pedantic -std=c11 -g3 -fsanitize=address,undefined

all: $(TARGET)

$(TARGET): $(OBJ_FILES)
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f $(OBJ_FILES)
