benchmphf: main.cpp emphf/emphf_config.hpp
	g++ main.cpp -std=c++11 -O3 -o benchmphf

emphf/emphf_config.hpp: emphf/emphf_config.hpp.in
	cd emphf/ && cmake . && cd ..

clean:
	rm -f benchmphf emphf/emphf_config.hpp emphf/CMakeCache.txt cmake_install.cmake
	rm -Rf emphf/CMakeFiles 
