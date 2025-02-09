# Scheduler-Synchronisation
Dissertation UG4


To run the code:
1) Run `Git submodule update --init --recursive`
2) Then build the `libfiber` library using `make` afer `cd` -ing into that folder.
3) Build the code with the `make` command after `cd` -ing into the parent folder `sched-sync`. ( PS: Please chage the `INCLUDE_DIR`  and `LIB_DIR` in the `Makefile` to where the `libfiber/include`  and `libfiber` directories are. )( I will automate this in the future )  
4) run `export LD_LIBRARY_PATH=/home/souparna/diss/Scheduler-Synchronisation/libfiber-diss:$LD_LIBRARY_PATH`
5) run `./bin/subversion_demo`
