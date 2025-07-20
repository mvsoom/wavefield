run:
	g++ -c -std=c++11 -O3 *.cpp
	g++ -w -o wavefield -O3 *.o -lsndfile -lsamplerate
	./wavefield -t 1.2 -r 6000 ../6b.WAVE
