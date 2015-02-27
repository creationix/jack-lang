SOURCE_FILES=api.c intern.c main.c

jack: $(SOURCE_FILES)
	musl-gcc -static $(SOURCE_FILES) -Wall -Werror -std=c99 -o jack -g

initramfs: jack
	cp jack init
	echo init|cpio -o --format=newc > initramfs
	rm init

boot: initramfs
	qemu-system-x86_64 -enable-kvm -initrd initramfs -kernel /boot/vmlinuz-linux

clean:
	rm -f initramfs jack
