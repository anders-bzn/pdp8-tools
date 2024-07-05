all: parse-bootrom.c capture-pdp8-papertapes.c create-bootrom.c
	gcc -o capture-papertape capture-pdp8-papertapes.c -Wall
	gcc -o parse-bootrom parse-bootrom.c -Wall
	gcc -o create-bootrom create-bootrom.c -Wall
	gcc -o put-tape put-tape.c -Wall
	gcc -o serial-dump serial-dump.c -Wall

clean:
	rm capture-papertape
	rm parse-bootrom
	rm create-bootrom
	rm put-tape
	rm serial-dump

