build:
	mpic++ tema3.cpp -o tema3

run:
	mpirun --oversubscribe -np 8 tema3 24 0

clean:
	rm -rf tema3