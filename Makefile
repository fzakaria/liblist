

all: libpreload.so


libpreload.so: preload.cc
	gcc -I ./include/ELFIO -Wall -fPIC -shared preload.cc -lstdc++ -ldl -o libpreload.so
