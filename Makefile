# Just a wrapper to copy boot.bin and generate boot.fbi

all: boot.bin boot.fbi boot/boot.bin

boot.fbi: boot.bin
	printf '\x60\x43\x0c\x00\xfd\x44\x36\xac' | cat - boot.bin > boot.fbi

boot.bin: boot/boot.bin
	cp boot/boot.bin ./

boot/boot.bin:
	make -C boot boot.bin

clean:
	make -C boot clean
	rm -f boot.bin boot.fbi

.PHONY: boot/boot.bin
