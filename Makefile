all: rowhammer hammer dumbp
clean: rm -f rowhammer hammer dumbp
rowhammer: rowhammer.cc
    g++ -std=c++11 -O3 -o rowhammer rowhammer.cc
hammer: hammer.cc
    g++ -std=c++11 -O3 -o hammer hammer.cc memutils.cc
dumbp: dumbp.cc
    g++ -std=c++11 -O3 -o dumbp dumbp.cc
