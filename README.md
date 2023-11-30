# SSHRD_x64

restored_external work library and make file
install ldid
go terminal and write: /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)" && brew install ldid

if you have already installed brew only run command: brew install ldid

Banner SSH connection.

# Prerequsites

A computer running macOS
A checkm8 device (A7-A11)

# Usage

If you have cloned this before, run cd SSHRD_Script && git pull to pull new changes
Run ./sshrd.sh or ./sshrdO.sh <iOS version for ramdisk>, without the <>.
The iOS version doesn't have to be the version you're currently on, but it should be close enough, and SEP has to be compatible
If you're on Linux, you will not be able to make a ramdisk for 16.1+, please use something lower instead, like 16.0
This is due to ramdisks switching to APFS over HFS+, and another dmg library would have to be used
Place your device into DFU mode
A11 users, go to recovery first, then DFU.
Run ./sshrd.sh or ./sshrdO.sh boot to boot the ramdisk
Run ./sshrd.sh or ./sshrdO.sh ssh to connect to SSH on your device
Finally, to mount the filesystems, run mount_party
/var is mounted to /mnt2 in the ssh session.
/private/preboot is mounted to /mnt6.
DO NOT RUN THIS IF THE DEVICE IS ON A REALLY OLD VERSION!!!!!!!
Have fun!


# Credits

tihmstar for pzb/original iBoot64Patcher/img4tool

xerub for img4lib and restored_external in the ramdisk

Cryptic for iBoot64Patcher fork

opa334 for TrollStore

Nebula for a bunch of QOL fixes to this script

OpenAI for converting kerneldiff into C

Ploosh for KPlooshFinder

Nathan (verygenericname) SSHRD
