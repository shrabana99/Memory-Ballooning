# OS

### Downloading kernel code repo:
```
wget https://cdn.kernel.org/pub/linux/kernel/v5.x/linux-5.11.5.tar.xz
```

### Extracting source code
```
sudo tar -xvf linux-5.11.5.tar.xz 
```
### Writing system call
```
https://medium.com/anubhav-shrimal/adding-a-hello-world-system-call-to-linux-kernel-dad32875872
```

### Building filesystem (Buildroot)
```
https://medium.com/@daeseok.youn/prepare-the-environment-for-developing-linux-kernel-with-qemu-c55e37ba8ade
```

### Using buildroot files
```
cd buildroot
copy xyz

make menuconfig
	Goto "System Configuration"
	Goto "Root filesystem overlay directories"
	"name of folder:" xyz

Compile buildroot: make -j4
```

> :warning: c files to be compiled statically using --static flag

### Using patchfiles
```
cd linux-5.11.5
git init
git add .
git commit -am "Initial Commit"
git am 19529_e0253_0
```

### Compiling kernel
```
make ARCH=x86_64 x86_64_defconfig
make menuconfig

Goto "Memory Management options"
Enable "Idle page tracking"

Compile kernel: make -j4
```

### Running QEMU
```
qemu-system-x86_64 -kernel  linux-5.11.5/arch/x86/boot/bzImage -hda     buildroot/output/images/rootfs.ext4 -append  "root=/dev/sda rw console=ttyS0,115200 acpi=off nokaslr" -boot    c -m    2.5G -serial  stdio -display none
```

### Using bashfile
For making the filesystem and kernel + running QEMU
```
bash make_run.sh
```

For running QEMU
```
bash run.sh
```

### Making Patch
Follow instruction mentioned in assignment
