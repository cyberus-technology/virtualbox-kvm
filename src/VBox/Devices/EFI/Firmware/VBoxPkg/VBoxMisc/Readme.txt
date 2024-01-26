Some drivers and applications we don't wont to include into fw
but we might want to load/run from time to time for debug reasons
or even use on real hw. To do it it required add inf file to
VBoxPkg/VBoxPkg.dsc and don't forget add to SOURCES variable in
Makefile.

# build
# make -C VBoxPkg/VBoxMisc

Result iso is RockRidge image attachable to VM and readable
with our EFI.
