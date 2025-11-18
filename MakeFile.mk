all: seq par_critical par_atomic

seq: analyzer_seq.o hash_table.o
	gcc -o seq analyzer_seq.o hash_table.o

par_critical: analyzer_par_critical.o hash_table.o
	gcc -fopenmp -o par_critical analyzer_par_critical.o hash_table.o

par_atomic: analyzer_par_atomic.o hash_table.o
	gcc -fopenmp -o par_atomic analyzer_par_atomic.o hash_table.o

%.o: %.c
	gcc -c $<

clean:
	rm -f *.o seq par_critical par_atomic