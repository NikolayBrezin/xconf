CC=x86_64-kgp-mingw32-gcc.exe
STRIP=strip.exe
RM=rm.exe

LIBS=-lglfw3 -lopengl32 -lgdi32

all:
	$(CC) gears.c glad_gl.c $(LIBS) -o gears.exe
	$(CC) boing.c glad_gl.c $(LIBS) -o boing.exe
	$(CC) heightmap.c glad_gl.c $(LIBS) -o heightmap.exe
	$(CC) particles.c tinycthread.c glad_gl.c $(LIBS) -o particles.exe
	$(CC) offscreen.c glad_gl.c $(LIBS) -o offscreen.exe
	$(CC) splitview.c glad_gl.c $(LIBS) -o splitview.exe
	$(CC) triangle-opengl.c glad_gl.c $(LIBS) -o triangle-opengl.exe
	$(CC) wave.c glad_gl.c $(LIBS) -o wave.exe
	$(STRIP) *.exe

clean:
	$(RM) -f *.exe offscreen.png
