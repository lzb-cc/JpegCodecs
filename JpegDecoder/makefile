

main: main.cpp JpegDecoder.o
	g++ -I src/  main.cpp JpegDecoder.o -o main

	
JpegDecoder.o : src/JpegDecoder.cpp
	g++ -c src/JpegDecoder.cpp
	
clean:
	rm *.o main.exe