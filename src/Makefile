OBJECTS = main.o luisita.o
 
TARGET = luisita
 
linux: $(OBJECTS)
	cp ./bass/libbass.so ../libbass.so
	g++ -O3 -o ../$(TARGET) $(OBJECTS) ./lua/src/liblua.a -L./bass -lbass -pthread `sdl-config --cflags --libs` -lGL -lGLU -lGLEW -Wl,-R.

linux32: $(OBJECTS)
	cp ./bass/libbass_x86_32.so ../libbass.so
	cp ./bass/libbass_x86_32.so ./bass/libbass.so
	g++ -O3 -o ../$(TARGET) $(OBJECTS) -m32 ./lua/src/liblua.a -L./bass -lbass -pthread `sdl-config --cflags --libs` -lGL -lGLU -lGLEW -Wl,-R.
	rm ./bass/libbass.so
	
linux64: $(OBJECTS)
	cp ./bass/libbass_x86_64.so ../libbass.so
	cp ./bass/libbass_x86_64.so ./bass/libbass.so
	g++ -O3 -o ../$(TARGET) $(OBJECTS) -m64 ./lua/src/liblua.a -L./bass -lbass -pthread `sdl-config --cflags --libs` -lGL -lGLU -lGLEW -Wl,-R.
	rm ./bass/libbass.so
 
clean:
	rm $(OBJECTS)
