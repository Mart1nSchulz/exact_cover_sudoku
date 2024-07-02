#include <stdio.h>
#include <cstring>
#include <thread>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/mman.h>
#include "blockingQ.hpp"
#include "dlinks_matrix.hpp"

class Buf {
	public:
	unsigned char *puzzle;		// batchsize puzzles are spaced 82 bytes.
	unsigned char *solution;	// batchsize solutions are spaced 164 bytes. 
	unsigned int batchsize;

	Buf(unsigned char *puzzle, unsigned char *solution, unsigned int batchsize) {
		this->puzzle = puzzle;
		this->solution = solution;
		this->batchsize = batchsize;
	}
};

unsigned int	batchsize = 16;
const int 		nthreads  = 8;
std::thread *threads[nthreads];

void thread_loop(BlockingQueue<class Buf> *bq) {
	DLinks *dl = new DLinks;
	Buf b = { 0,0,0 };
	while ( true ) {
		bq->take(b);
		if ( b.batchsize == 0 ) {
			break;
		}
		for ( unsigned int i=0; i<b.batchsize; i++ ) {
			memcpy(b.solution+i*164, b.puzzle+i*82, 81);
			b.solution[i*164+81] = ',';
			b.solution[i*164+163] = '\n';
	        if(solve_puzzle(dl, b.puzzle+i*82)) {
				int index, value;
				for(int j=0; j<81; j++) {
					int row = dl->solution_stack[j]->row;
					index = row / 9;
					value = (row % 9) + '1';
					b.solution[i*164+82+index] = value;
				}
			} else {
				sprintf((char *)b.solution+i*164+82, "%-81s\n", "No solution");
			}
		}
	}
	delete dl;
}


//accepts up to 2 arguments: 
//  1 - file of puzzles, puzzles are a new-line delimited string of numbers with 0 representing empty boxes
//      defaults to "puzzles.txt"
//  2 - output file, solution will be written to the output file in the same format as the input file
//      defaults to "solutions.txt"
int main(int argc, char *argv[]) {

	argv++; argc--;

	const char *ifn = argc > 0? argv[0] : "puzzles.txt";
	int fdin = open(ifn, O_RDONLY);
	if ( fdin == -1 ) {
		if (errno ) {
			printf("Error opening file %s: %s\n", ifn, strerror(errno));
			exit(0);
		}
	}

    // get size of file
	struct stat sb;
	fstat(fdin, &sb);
    size_t fsize = sb.st_size;

	// map the input file
	unsigned char *puzzlez = (unsigned char *)mmap((void*)0, fsize, PROT_READ, MAP_PRIVATE, fdin, 0);
	if ( puzzlez == MAP_FAILED ) {
		if (errno ) {
			printf("Error mmap of input file %s: %s\n", ifn, strerror(errno));
			exit(0);
		}
	}

	// get and check the number of puzzles
	size_t npuzzlesin = (fsize+1) / 82;
	size_t npuzzlesread = 0;
	if ( npuzzlesin * 82 != fsize+1 && npuzzlesin * 82 != fsize) {
		printf("found %ld puzzles, but the file %s has %ld extra characters!\n",
			npuzzlesin, argc?argv[0]:"puzzles.txt", (fsize+1 - npuzzlesin * 82));
	}

	const char *ofn = argc > 1? argv[1] : "solutions.txt";
	int fdout = open(ofn, O_RDWR|O_CREAT, 0775);
	if ( fdout == -1 ) {
		if (errno ) {
			printf("Error opening output file %s: %s\n", ofn, strerror(errno));
			exit(0);
		}
	}
	if ( ftruncate(fdout, (size_t)npuzzlesin*164) == -1 ) {
		if (errno ) {
			printf("Error setting size (ftruncate) on output file %s: %s\n", ofn, strerror(errno));
		}
		exit(0);
	}

	// map the output file
	unsigned char *solvedout = (unsigned char *)mmap((void*)0, npuzzlesin*164, PROT_WRITE, MAP_SHARED, fdout, 0);
	if ( solvedout == MAP_FAILED ) {
		if (errno ) {
			printf("Error mmap of output file %s: %s\n", ofn, strerror(errno));
			exit(0);
		}
	}
	close(fdout);

	BlockingQueue<class Buf> bQ(64);

	for (unsigned int i=0; i<nthreads; i++) {
		auto bqp = &bQ;
		threads[i] = new std::thread( [=]{ thread_loop(bqp); } );
	}

	unsigned int remaining;
	unsigned int syncat = 0x2000;
    while( (remaining = npuzzlesin-npuzzlesread) > 0 ) {
		if ( remaining < batchsize ) {
			batchsize = remaining;
		}
		if ( npuzzlesread > syncat ) {
			if ( msync(solvedout, (syncat-64)*164-0x1000, MS_ASYNC) == -1 ) {
				if (errno ) {
					printf("Error msync of file %s: %s\n", ofn, strerror(errno));
				}
			}
			syncat += 0x2000;
		}

		bQ.emplace_back(puzzlez+npuzzlesread*82, solvedout+npuzzlesread*164, batchsize);
		// the following is actually a 'prefetch' of the file content
		// for the threads benefit.  The message, while sensible, should never display
		unsigned char x = puzzlez[npuzzlesread*82+81];
		if ( x != '\n' ) {
			printf("puzzle not NL-terminated\n");
		}
						
		npuzzlesread += batchsize;
	}

	for (unsigned int i=0; i<nthreads; i++) {
		bQ.put(Buf(0, 0, 0));
	}

	for (unsigned int i=0; i<nthreads; i++) {
		threads[i]->join();
	}

	int err = munmap(puzzlez, fsize);
	if ( err == -1 ) {
		if (errno ) {
			printf("Error munmap file %s: %s\n", ifn, strerror(errno));
		}
	}
	err = munmap(solvedout, (size_t)npuzzlesin*164);
	if ( err == -1 ) {
		if (errno ) {
			printf("Error munmap file %s: %s\n", ofn, strerror(errno));
		}
	}
    return 0;
}