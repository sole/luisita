OBJECTS = main.o luisita.o
 
TARGET = luisita
 
linux: $(OBJECTS)
	g++ -O3 -o ../$(TARGET) $(OBJECTS) ./lua/src/liblua.a -pthread `sdl-config --cflags --libs` -lGL -lGLU -lGLEW -Wl,-R.
 
clean:
	rm $(OBJECTS)
