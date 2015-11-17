a benchmark tool for constructing minimal perfect hash functions

the phf library is: https://github.com/wahern/phf
emphf is: https://github.com/ot/emphf

the aim of this program is to evaluate the time and memory used by
those MPHF libraries during construction of the structure.

it creates a minimal perfact hash function over the following input keys:
a random set of 64-bits integers
(however both libraries support more versatile input, e.g. strings)


How to run 
----------

  git clone --recursive https://github.com/rchikhi/benchmphf
  cd benchmphf
  make
  ./benchmphf [number of keys, default: 100,000,000]



Example output
--------------






Notes
-----

- The code contains many #ifdef's, because it can also be used in conjunction with the GATB-core library.
 See below for compiling with GATB-core. It enables to test the emphf library that has been integrated inside GATB-core.

- There seems to be a bug with the HEM algorithm for tiny MPHF's (in the order of 1000 elements or less).
(terminate called after throwing an instance of 'std::length_error')

- In mphf_hem.cpp of emphf/, I'm not sure, but I'm tempted to change "typedef uint32_t node_t" to the uint64_t type.
Will only affect MPHF of more than 2^32 (~4 billion) elements though.



Advanced usage for GATB-core developers (https://github.com/GATB/gatb-core/)
---------------------------------------

- Put the gatb-core library in thirdparty/gatb-core/
- Comment the "#define VANILLA_EMPHF" line in main.cpp
- Compile as follows:

    mkdir build && cmake .. && make -j 4 
    bin/benchmphf


Author
------

Rayan Chikhi
rayan.chikhi@univ-lille1.fr
