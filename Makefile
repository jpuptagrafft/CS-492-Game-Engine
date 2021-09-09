LDFLAGS = -lglut -lGL -lGLU -lm -pthread -lSOIL
CPPFLAGS = -std=c++17 -Wno-narrowing -g 
CC = g++
OBJS = defs.o main.o LoadedObject.o tiny_obj_loader.o Wall.o

ALL : ${OBJS}
	${CC}  ${OBJS} ${LDFLAGS} -o teapotgame

clean:
	rm ${OBJS} teapotgame

${OBJS} : game.h
