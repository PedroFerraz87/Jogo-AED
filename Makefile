# Makefile baseado no guia: https://gist.github.com/Jeraross/89915262d018b95da218ace0d769beff

TARGET = crossy
SRC_DIR = .
RELEASE_DIR = bin
LIB_DIR = lib_raylib

SOURCES = \
	$(SRC_DIR)/main.c \
	$(SRC_DIR)/game.c \
	$(SRC_DIR)/fila.c \
	$(SRC_DIR)/ranking.c \
	$(SRC_DIR)/utils.c \
	$(SRC_DIR)/raylib_view.c

CFLAGS = -Wall -std=c99 -DENABLE_RAYLIB -I$(LIB_DIR) -I$(SRC_DIR)
LIBS = -L$(LIB_DIR) -lraylib -lopengl32 -lgdi32 -lwinmm

$(RELEASE_DIR)/$(TARGET).exe: $(SOURCES)
	@mkdir -p $(RELEASE_DIR)
	gcc $(CFLAGS) $^ -o $@ $(LIBS)
	-@cp $(LIB_DIR)/raylib.dll $(RELEASE_DIR)/
	@echo "Compilacao concluida. Executavel em bin/"

run: $(RELEASE_DIR)/$(TARGET).exe
	./bin/$(TARGET).exe

clean:
	rm -rf $(RELEASE_DIR)/$(TARGET).exe

.PHONY: run clean




