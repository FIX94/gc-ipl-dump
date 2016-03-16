make clean
rm -rf boot.dol gc-ipl-dump.gci
make
dollz3 boot_reg.dol boot.dol
dol2gci boot.dol gc-ipl-dump.gci