# Written by Jonathan Jauhari 1038331, based on the given sample Makefile

CC=gcc
OBJ=dns_message.o util.o
COPT=-Wall -Wpedantic -g
BIN_PHASE1=phase1
BIN_PHASE2=dns_svr

# Running "make" with no argument will make the first target in the file
all: $(BIN_PHASE1) $(BIN_PHASE2)

$(BIN_PHASE2): dns_svr.c $(OBJ)
	$(CC) -o $(BIN_PHASE2) dns_svr.c $(OBJ) $(COPT)

$(BIN_PHASE1): phase1.c $(OBJ)
	$(CC) -o $(BIN_PHASE1) phase1.c $(OBJ) $(COPT)

# Wildcard rule to make any  .o  file,
# given a .c and .h file with the same leading filename component
%.o: %.c %.h
	$(CC) -c $< $(COPT) -g

clean:
	rm -f $(BIN_PHASE1) $(BIN_PHASE2) *.o *.log
