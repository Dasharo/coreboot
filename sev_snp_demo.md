# SEV-SNP demo on MZ33-AR1

## Introduction

The demo showcases the AMD SEV-SNP technology on open-source firmware based on
coreboot and [AMD OpenSIL](https://github.com/openSIL/openSIL/tree/turin_poc)
on an AMD EPYC Turin platform, the Gigabyte MZ33-AR1.

> NOTE: AMD OpenSIL is pre-production, evaluation software, not intended for
> production use. AMD does not provide any support for OpenSIL solutions.

Perform the following steps:

1. [Building and flashing firmware](#building-and-flashing-firmware)
2. [Host preparation](#host-preparation)
3. [Guest preparation](#guest-preparation)
4. [SEV-SNP guest attestation](#sev-snp-guest-attestation)

## Building and flashing firmware

1. Ensure you have [docker installed](https://docs.docker.com/engine/install/)
   on your building machine.
2. Clone the coreboot repository with its submodules:

    ```sh
    git clone https://github.com/Dasharo/coreboot.git
    cd coreboot
    git submodule update --init --checkout
    git checkout sev-snp-demo
    git submodule update --init --checkout
    ```

3. Clone the `3mdeb/coreboot-site-local` repository to `site-local` in the
   coreboot root directory.
4. Launch the coreboot SDK version `2025-07-04_a9e97268fe`:

    ```sh
    docker run --rm -it -v $PWD:/home/coreboot/coreboot \
    	-w /home/coreboot/coreboot\
    	coreboot/coreboot-sdk:2025-07-04_a9e97268fe \
    	/bin/bash
    ```

5. Copy the config and start build process:

    ```sh
    (docker) cp configs/config.gigabyte_mz33-ar1 .config
    (docker) make olddefconfig
    (docker) make -j$(nproc)
    ```

6. When build is finished, use `coreboot/build/coreboot.rbu` to flash the
   firmware using the BMC web interface under Maintenance -> Firmware Update.
7. Boot the machine and check if you are running coreboot:

    ```sh
    sudo dmidecode -t bios
    ```

## Host preparation

This demo relies on Ubuntu 25.04 or newer to be installed as the host
operating system on Gigabyte MZ33-AR1. If the system is installed, do:

1. Install QEMU and other utilities:

    ```sh
    sudo apt-get install \
        qemu-system-x86 libguestfs-tools cloud-image-utils
    ```

2. Enable SEV features in `kvm_amd`:

    - permanently via `/etc/modprobe.d/kvm_amd.conf` file:

    ```sh
    echo "options kvm_amd sev-snp=1 sev=1 sev-es=1" | \
    	sudo tee /etc/modprobe.d/kvm_amd.conf
    sudo reboot
    ```

    - by reloading `kvm_amd` module with the following parameters:

    ```sh
    sudo rmmod kvm_amd
    sudo modprobe kvm_amd sev-snp=1 sev=1 sev-es=1
    ```

3. Confirm SEV-SNP is enabled:

    ```sh
    $ dmesg | grep -i -e rmp -e sev -e ccp

    SEV-SNP: RMP table physical range [0x0000000843a00000 - 0x000000084bffffff]
    ccp 0000:a4:00.5: sev enabled
    ccp 0000:a4:00.5: psp enabled
    ccp 0000:a4:00.5: SEV API:1.58 build:4
    ccp 0000:a4:00.5: SEV-SNP API:1.58 build:4
    kvm_amd: SEV enabled (ASIDs 512 - 1006)
    kvm_amd: SEV-ES enabled (ASIDs 1 - 511)
    kvm_amd: SEV-SNP enabled (ASIDs 1 - 511)

    $ cat /sys/module/kvm_amd/parameters/sev
    Y

    $ cat /sys/module/kvm_amd/parameters/sev_es
    Y

    $ cat /sys/module/kvm_amd/parameters/sev_snp
    Y
    ```

    > SEV-SNP RMP table address range and number of ASIDs may be different on your
    > system. The Gigabyte M33-AR1 build is configured for 511 SEV-ES/SEV-SNP
    > guests.

## Guest preparation

1. Download [Ubuntu 25.04
   cloudimg](https://cloud-images.ubuntu.com/releases/plucky/release/ubuntu-25.04-server-cloudimg-amd64.img).
2. Resize the image to make space for additional packages, dependencies and
   utilities:

    ```sh
    qemu-img resize ubuntu-25.04-server-cloudimg-amd64.img +10G
    ```

3. Prepare user data to change Ubuntu VM default user password:

    ```sh
    cat >user-data <<EOF
    #cloud-config
    password: ubuntu
    chpasswd: { expire: False }
    ssh_pwauth: True
    EOF

    cloud-localds user-data.img user-data
    ```

    > This will change VM login credentials to `ubuntu:ubuntu`.

4. Launch guest without SEV-SNP (installing `linux-generic` crashes SEV-SNP
   guest):

    ```sh
    qemu-system-x86_64 \
      -enable-kvm \
      -nographic \
      -machine q35 -smp 6 -m 6G \
      -drive "if=virtio,format=qcow2,file=ubuntu-25.04-server-cloudimg-amd64.img" \
      -hdb user-data.img \
      -net nic,model=e1000 -net user,hostfwd=tcp::2222-:22 \
      -cpu EPYC-v4 \
      -bios /usr/share/ovmf/OVMF.amdsev.fd
    ```

5. Install dependencies:

    ```sh
    (guest VM)$ sudo apt install git build-essential linux-generic
    (guest VM)$ curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh
    ```

6. Build [snpguest](https://github.com/virtee/snpguest) utility:

    ```sh
    (guest VM)$ git clone https://github.com/virtee/snpguest.git
    (guest VM)$ cd snpguest
    (guest VM)$ source "$HOME/.cargo/env"
    (guest VM)$ cargo build -r
    ```

7. Poweroff the VM:

    ```sh
    (guest VM)$ sudo poweroff
    ```

## SEV-SNP guest attestation 

1. Launch the VM again, with SEV-SNP this time:

    ```sh
    qemu-system-x86_64 \
      -enable-kvm \
      -nographic \
      -machine q35 -smp 6 -m 6G \
      -drive "if=virtio,format=qcow2,file=ubuntu-25.04-server-cloudimg-amd64.img" \
      -hdb user-data.img \
      -net nic,model=e1000 -net user,hostfwd=tcp::2222-:22 \
      -cpu EPYC-v4 \
      -machine memory-encryption=sev0,vmport=off \
      -object memory-backend-memfd,id=ram1,size=6G,share=true,prealloc=false \
      -machine memory-backend=ram1 \
      -object sev-snp-guest,id=sev0,cbitpos=51,reduced-phys-bits=6 \
      -bios /usr/share/ovmf/OVMF.amdsev.fd
    ```

2. Check if kernel detects SEV features:

    ```sh
    (guest VM) sudo dmesg |grep -i sev

    Memory Encryption Features active: AMD SEV SEV-ES SEV-SNP
    SEV: Status: SEV SEV-ES SEV-SNP 
    SEV: APIC: wakeup_secondary_cpu() replaced with wakeup_cpu_via_vmgexit()
    SEV: Using SNP CPUID table, 29 entries present.
    SEV: SNP running at VMPL0.
    SEV: SNP guest platform device initialized.
    kvm_amd: KVM is unsupported when running as an SEV guest
    ```

3. Perform [SEV-SNP guest
   attestation](https://www.vpsbg.eu/docs/how-to-perform-amd-sev-snp-attestation-inside-a-guest-virtual-machine):

    ```sh
    (guest VM)$ sudo modprobe sev-guest
    (guest VM)$ cd snpguest/target/release
    (guest VM)$ sudo ./snpguest report report.bin request-file.txt --random
    (guest VM)$ sudo ./snpguest fetch ca -r report.bin pem ./
    (guest VM)$ sudo ./snpguest fetch vcek pem ./ ./report.bin
    (guest VM)$ sudo ./snpguest verify certs ./

    (guest VM)  The AMD ARK was self-signed!
    (guest VM)  The AMD ASK was signed by the AMD ARK!
    (guest VM)  The VCEK was signed by the AMD ASK!

    (guest VM)$ sudo ./snpguest verify attestation ./ ./report.bin

    (guest VM)  Reported TCB Boot Loader from certificate matches the attestation report.
    (guest VM)  Reported TCB TEE from certificate matches the attestation report.
    (guest VM)  Reported TCB SNP from certificate matches the attestation report.
    (guest VM)  Reported TCB Microcode from certificate matches the attestation report.
    (guest VM)  Reported TCB FMC from certificate matches the attestation report.
    (guest VM)  VEK signed the Attestation Report!
    ```

Result should be identical to above output.
