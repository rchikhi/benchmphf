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

    $ ./benchmphf 100000000
    Constructing a MPHF with n=100000000 elements
    initial data allocation    memory [current, maximum (maxRSS)]: [ 765,  765] MB
    
    Construction with 'emphf' library..
    [...]
    benchmphf: [vanilla emphf scan] constructed perfect hash for 100000000 keys in 1765.302888s
    after emphf construction    memory [current, maximum (maxRSS)]: [ 810, 3018] MB 
    Very rough estimation of memory used by the MPHF constructed by emphf : 45 MB (3.77487 bits per elt)

    Construction with 'phf' library.. 
    benchmphf: [phf] found perfect hash for 100000000 keys in 232.410405s
    after phf construction    memory [current, maximum (maxRSS)]: [ 938, 3515] MB 
    Very rough estimation of memory used by the MPHF constructed by phf : 128 MB (10.7374 bits per elt)

What's interesting here is the time (1765 seconds) and peak memory (3018 MB) taken
for constructing the MPHF using emphf, and similarly for phf. Note that peak memory
include the data size (here, 765 MB). Hence construction space for emphf is around 3018-765=2253 MB.
The estimation of memory taken by the final MPHF (45 MB for emphf) is inaccurate, see below.



So, which library is better?
----------------------------

Note that _emphf_ actually implements [four complementary algorithms](https://github.com/ot/emphf): _seq_, _scan_, _scan_mmap_, and _HEM_. By default, this benchmark uses the _scan\_mmap_ algo. But this can be changed by tweaking the source code (in main.cpp).

_emphf's scan\_mmap_ is slower than _phf_ but constructs more compact MPHFs, and uses less memory during construction. This is partly because _scan\_mmap_ implements an external memory algorithm, and _phf_ does not. The final MPHFs constructed by _phf_  are relatively larger because the library seems to not be performing some of the usual final compression steps (e.g. reduce the range from 1.23n to n using a succinct bit array).

However, I found that the _seq_ and the _HEM_ algorithms are faster than _phf_ during construction, and use less memory. For 1 billion 64-bits random integers, construction using _HEM_ takes 500 seconds and 12 GB of memory, whereas with _phf_ it takes 1790 seconds and 26 GB of memory.

All algorithms, except _scan\_mmap_, require in the order of 30 bytes per element during construction. _scan\_mmap_ is therefore only one capable of constructing large MPHFs in situations where memory is limited.

Notes
-----

- The program also provides an estimation of the space used by the structure itself,
via measuring deltas in process memory usage, so it will be extremely inaccurate 
for small values of n  (less than tens of millions). Do not report this value 
in any serious experiment; rather, you should carefully estimate the size of the 
objects that represent the MPHF created by each library.


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
