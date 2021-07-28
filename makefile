main: main.cpp
	g++ -std=c++17 main.cpp -o main.x

pipe: pipe_demo.c
	gcc pipe_demo.c -o pipe.x

twopipes: twoPipes.c
	gcc twoPipes.c -o twoPipes.x

runmain: main
	clear
	./main.x

runpipe: pipe
	clear
	./pipe.x

runtwopipes: twopipes
	clear
	./twoPipes.x input.txt

clean:
	rm *.x