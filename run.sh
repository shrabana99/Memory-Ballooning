make -j10 -C linux-5.11.5/
qemu-system-x86_64 -kernel  linux-5.11.5/arch/x86/boot/bzImage -hda     buildroot/output/images/rootfs.ext4 -append  "root=/dev/sda rw console=ttyS0,115200 acpi=off nokaslr" -boot    c -m    4.5G -serial  stdio -display none
