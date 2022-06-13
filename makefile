CFLAGS = -std=gnu11 -g -Wall -Wextra
VFLAGS = --leak-check=full --track-origins=yes --show-leak-kinds=all --malloc-fill=0x40 --free-fill=0x23
BIN = t client server 

all: $(BIN)

t:
	touch server.c && touch client.c
	

client: send_packet.c client.c 
	gcc $(CFLAGS) $^ -o $@

server: send_packet.c server.c
	gcc $(CFLAGS) $^ -o $@

clean:
	rm -rf *.o
	rm -f $(BIN)