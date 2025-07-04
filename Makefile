CC = gcc
CFLAGS = -Wall -Iglad/include -I../utils -I../world -I../adts
LDFLAGS = -lglfw -ldl -lm

SRC = main.c glad/glad.c utils/shader.c utils/math.c world/chunk.c world/camera.c adts/hash.c utils/stringManipulate.c world/world.c utils/texture.c world/physics.c
OBJ = $(SRC:.c=.o)
OUT = main

all: $(OUT)

# Link object files into the final binary
$(OUT): $(OBJ)
	$(CC) $(OBJ) -o $(OUT) $(LDFLAGS)

# Compile .c files into .o object files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OUT) $(OBJ)