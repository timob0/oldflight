CC ?= gcc
CFLAGS ?= -Wno-deprecated -Wno-visibility -Wno-pointer-to-int-cast -Wno-int-to-pointer-cast -Wno-absolute-value -Wno-comment -Wno-static-in-inline -Wno-pointer-bool-conversion
CPPFLAGS ?= -MMD -MP
LDFLAGS ?= -framework OpenGL -framework GLUT -lm
TARGET ?= oldflight-bin

BASKET_TEXTURE_SRC := obj/korb.jpg
BASKET_TEXTURE_PPM := obj/processed-korb-256.ppm

SRCS := $(wildcard *.c)
OBJS := $(SRCS:.c=.o)
DEPS := $(OBJS:.o=.d)

all: $(TARGET)

$(TARGET): $(BASKET_TEXTURE_PPM) $(OBJS)
	$(CC) $(OBJS) -o $@ $(LDFLAGS)

$(BASKET_TEXTURE_PPM): $(BASKET_TEXTURE_SRC)
	ffmpeg -y -i $(BASKET_TEXTURE_SRC) -vf scale=256:-1 -frames:v 1 -update 1 $@

%.o: %.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(OBJS) $(DEPS) $(TARGET) $(BASKET_TEXTURE_PPM)

-include $(DEPS)

.PHONY: all run clean
