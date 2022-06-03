make -j12 -C linux-5.11.5/
cd buildroot/xyz/
gcc -static -O3 -pthread userApp.c -o userApp
cd ../..
make -j12 -C buildroot/
qemu-system-x86_64 -kernel  linux-5.11.5/arch/x86/boot/bzImage -hda     buildroot/output/images/rootfs.ext4 -append  "root=/dev/sda rw console=ttyS0,115200 acpi=off nokaslr" -boot    c -m    2.5G -serial  stdio -display none
