LIBS  = -lX11 -lXext -lXss -lcurl -lm
CFLAGS = -O3
SRC = $(wildcard source/*.c)

make:
	gcc $(SRC) -o NIMBYDaemon $(CFLAGS) $(LIBS)
