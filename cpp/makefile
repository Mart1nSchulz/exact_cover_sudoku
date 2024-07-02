EXE := ss
CC := g++
CFLAGS := -O3 -mavx -Wall -Wextra -DNDEBUG
#CFLAGS := -g -Og -mavx -Wall -Wextra
#CFLAGS := -g -mavx -Wall -Wextra
#CFLAGS := -pg -Og -mavx -Wall -Wextra
LFLAGS := -lm
SRC := .cpp

OBJS := $(patsubst %$(SRC), %.o, $(wildcard *$(SRC)))
DEPS := $(OBJS:.o=.d)

run: $(EXE)
	./$(EXE)

$(EXE): $(OBJS)
	$(CC) $(CFLAGS) -o $(EXE) $(OBJS) $(LFLAGS)

-include $(DEPS)
%.o: %$(SRC)
	$(CC) $(CFLAGS) -c -MMD -o $@ $<

clean:
	rm $(EXE) $(OBJS) $(DEPS)
