###############################################################################
#  Common settings
###############################################################################
INCLUDE_DIRS = -Iglad/include -Iutils -Iworld -Iadts -Ilibs
CSTD         = -std=gnu99           # <- gives tanf(), sinf() prototypes
CFLAGS       = $(CSTD) -Wall $(INCLUDE_DIRS)

SRC = \
  main.c \
  glad/glad.c \
  utils/shader.c \
  utils/math.c \
  utils/stringManipulate.c \
  utils/texture.c \
  utils/perlin.c \
  adts/hash.c \
  world/camera.c \
  world/chunk.c \
  world/physics.c \
  world/world.c

###############################################################################
#  Native (desktop) build
###############################################################################
HOST_CC   ?= gcc
LDFLAGS   = -lglfw -ldl -lm         # -lm here too

OBJ = $(SRC:.c=.o)
BIN = main

all: $(BIN)

$(BIN): $(OBJ)
	$(HOST_CC) $^ -o $@ $(LDFLAGS)

%.o: %.c
	$(HOST_CC) $(CFLAGS) -c $< -o $@

###############################################################################
#  WebAssembly (Emscripten) build
###############################################################################
WASM_CC   ?= emcc
WEB_DIR    = build
WEB_HTML   = $(WEB_DIR)/game.html

WEB_SRC    = $(filter-out glad/glad.c,$(SRC))

WEB_ASSETS = --preload-file shader --preload-file texture --preload-file imgs
WEB_FLAGS  = -O3                                     \
             -s USE_GLFW=3 -s FULL_ES3=1             \
             -s MIN_WEBGL_VERSION=2 -s MAX_WEBGL_VERSION=2 \
             -s ALLOW_MEMORY_GROWTH=1                \
             -lm $(WEB_ASSETS)

web: $(WEB_HTML)

$(WEB_DIR):
	mkdir -p $@

$(WEB_HTML): $(WEB_SRC) | $(WEB_DIR)
	$(WASM_CC) $(WEB_SRC) $(CFLAGS) $(WEB_FLAGS) -o $@

###############################################################################
#  Helpers
###############################################################################
run: all
	./$(BIN)

run-web: web
	emrun --no_browser --port 8000 $(WEB_HTML)

clean:
	rm -f $(BIN) $(OBJ)
	rm -rf $(WEB_DIR)

.PHONY: all clean run web run-web
