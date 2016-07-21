current_dir = $(shell pwd)

benchmphf: main.cpp emphf/emphf_config.hpp libcmph/include/cmph.h
	g++ main.cpp -std=c++11 -O3 -o benchmphf -lcmph -L./libcmph/lib

emphf/emphf_config.hpp: emphf/emphf_config.hpp.in
	cd emphf/ && cmake . && cd ..

libcmph/include/cmph.h: 
	cd cmph && ./configure --prefix=${current_dir}/libcmph && make && make install

clean:
	rm -f benchmphf emphf/emphf_config.hpp emphf/CMakeCache.txt cmake_install.cmake
	rm -Rf emphf/CMakeFiles 
	rm -rf libcmph
	cd cmph && make clean
