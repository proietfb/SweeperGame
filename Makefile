# Project: progettoCar4

CPP  = g++
CC   = gcc
BIN  = cityCleaner

OBJ  = main.o sweeper.o mesh.o heep.o
LINKOBJ  = main.o sweeper.o mesh.o heep.o
LIBS = -L/usr/X11R6 -lGL -lGLU -lSDL2_image -lSDL2 -lm -lGLEW -lglut
INCS = -I. -I/usr/X11R6/include
CXXINCS=#
CXXFLAGS = $(CXXINCS)
CFLAGS = $(INCS)
RM = rm -f

all: $(BIN)


clean:
	${RM} $(OBJ) $(BIN)

$(BIN): $(OBJ)
	$(CPP) $(LINKOBJ) -o $(BIN) $(LIBS)

main.o: main.cpp
	$(CPP) -c main.cpp -o main.o $(CXXFLAGS)

sweeper.o: sweeper.cpp
	$(CPP) -c sweeper.cpp -o sweeper.o $(CXXFLAGS)

mesh.o: mesh.cpp
	$(CPP) -c mesh.cpp -o mesh.o $(CXXFLAGS)

heep.o: heep.cpp
	$(CPP) -c heep.cpp -o heep.o $(CXXFLAGS)
