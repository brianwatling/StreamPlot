
streamplot: src/ClientSocket.h src/main.cpp src/ServerSocket.h src/ClientSocket.cpp src/ServerSocket.cpp
	g++ -O2 -Isrc src/main.cpp src/ClientSocket.cpp src/ServerSocket.cpp -lGL -lGLU -lglut -lboost_program_options -o streamplot

clean:
	rm streamplot
