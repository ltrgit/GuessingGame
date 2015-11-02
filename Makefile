COMPILER=gcc
CFLAGS=-Wall -pedantic -std=gnu99
DFLAGS=-g
SRC=guess_server.c
OUT=guessserver


client:
	$(COMPILER) $(CFLAGS) guess_client.c -o guessclient

all:
	$(COMPILER) $(CFLAGS) $(SRC) -o $(OUT)

debug:
	$(COMPILER) $(CFLAGS) $(DFLAGS) $(SRC) -o $(OUT)
clean:
	rm $(OUT)
