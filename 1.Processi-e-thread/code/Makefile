all: reactivity-processes reactivity-threads

reactivity-processes: reactivity-processes.c performance.c
	gcc -o reactivity-processes reactivity-processes.c performance.c -lrt -lm

reactivity-threads: reactivity-threads.c performance.c
	gcc -o reactivity-threads reactivity-threads.c performance.c -lpthread -lrt -lm

clean:
	rm -f reactivity-threads reactivity-processes