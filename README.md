a benchmark tool for the construction of [minimal perfect hash functions](https://en.wikipedia.org/wiki/Perfect_hash_function#Minimal_perfect_hash_function)

From Wikipedia: A perfect hash function for a set S is a hash function that maps distinct elements in S to a set of integers, with no collisions. A minimal perfect hash function is a perfect hash function that maps n keys to n consecutive integers—usually [0..n−1] or [1..n].

the phf library is: https://github.com/wahern/phf

emphf is: https://github.com/ot/emphf

the aim of this program is to evaluate the time and memory used by
those MPHF libraries during _construction_ of the structure.

it creates a minimal perfect hash function over the following input keys:
a random set of 64-bits integers
(however both libraries support more versatile input, e.g. strings)

Requirements
------------

A compiler that supports C++11 features, e.g. GCC >= 4.9.


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

- The program also provides an estimation of the space used by the structure itself,
via measuring the process memory usage, so it will be extremely inaccurate 
for small values of n  (less than tens of millions). Do not use this value 
in your experiments.


- The code contains many #ifdef's, because it can also be used in conjunction with the GATB-core library.
 See below for compiling with GATB-core. It enables to test the emphf library that has been integrated inside GATB-core.

- There seems to be a bug with the HEM algorithm for tiny MPHF's (in the order of 1000 elements or less).
(terminate called after throwing an instance of 'std::length_error')

- In mphf_hem.cpp of emphf/, I'm not sure, but I'm tempted to change "typedef uint32_t node_t" to the uint64_t type.
Will only affect MPHF of more than 2^32 (~4 billion) elements though.



Advanced usage
--------------

This is for GATB-core developers (https://github.com/GATB/gatb-core/).

- Put the gatb-core library in thirdparty/gatb-core/
- Comment the "#define VANILLA_EMPHF" line in main.cpp
- Compile as follows:

    mkdir build && cmake .. && make -j 4 
    bin/benchmphf


Author
------

Rayan Chikhi
rayan.chikhi@univ-lille1.fr
