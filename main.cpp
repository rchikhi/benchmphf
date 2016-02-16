/*
 * benchmark of two mphf libraries:
 * - phf
 * - emphf
 *
 */

#include <cstdlib> // stdtoul
#include <random>
#include <err.h>       /* for warnx() */
#include "phf/phf.cc" /* phf library */
#include "gatb_system.hpp"

// parameters:

// switch between emphf algorithms
//#define EMPHF_HEM
#define EMPHF_SCAN
//#define EMPHF_SEQ



//** from this point, no need to look unless developer*/


// comment this line to switch to GATB-core's emphf
#define VANILLA_EMPHF

// number of elements in the MPHF. determined at compile-time, so change here.
unsigned long n = 100000000; // [phf]: 4 GB mem , ~2 minutes construction
//unsigned long n = 10000000; // [phf]: 7 seconds construction

bool bench_lookup = false;

#ifdef VANILLA_EMPHF

    // vanilla emphf
    #include "emphf/common.hpp"
    #include "emphf/base_hash.hpp"
    #include "emphf/hypergraph.hpp"
    #include "emphf/mmap_memory_model.hpp"

    #ifdef EMPHF_HEM
        #include "emphf/mphf_hem.hpp" // for hem
        #include "emphf/hypergraph_sorter_seq.hpp"
    #else

        #include "emphf/mphf.hpp"
        #ifdef EMPHF_SCAN
            #include "emphf/hypergraph_sorter_scan.hpp"
        #else
            #include "emphf/hypergraph_sorter_seq.hpp"
        #endif

    #endif

#else

    // emphf from gatb-core
    #include <gatb/gatb_core.hpp>
    #include <emphf/common.hpp>
    #include <emphf/mphf.hpp>
    #include <emphf/base_hash.hpp>
    #include <emphf/mmap_memory_model.hpp>
    #include <emphf/hypergraph_sorter_scan.hpp>


#endif

using namespace std;


u_int64_t _previousMem = 0;
unsigned long memory_usage(string message="", string mphftype="")
{
    // from Progress.cpp of gatb-core

    /** We get the memory used by the current process. */
    u_int64_t mem = getMemorySelfUsed() / 1024;

    u_int64_t memMaxProcess = getMemorySelfMaxUsed() / 1024;

    /** We format the string to be displayed. */
    char tmp[128];
    snprintf (tmp, sizeof(tmp), "   memory [current, maximum (maxRSS)]: [%4lu, %4lu] MB ",
            mem, memMaxProcess
            );

    std::cout << message << " " << tmp << std::endl;

    if (mphftype != "")
    {
        float nbitsMPHF = ((mem - _previousMem ) * (1024.0 * 1024.0 * 8.0)) / n;
        std::cout << "Very rough estimation of memory used by the MPHF constructed by "  << mphftype << " : " << mem - _previousMem << " MB (" <<  nbitsMPHF << " bits per elt)" <<  std::endl; // based on peak RSS, so not very accurate unless a huge amount of elements
    }

    _previousMem = mem;
    return mem;
}





u_int64_t *data;

struct phf phf; // let's put it as global variable, so that phf object isn't destroyed after end of function
void do_phf()
{
	clock_t begin, end;
	begin = clock();
    size_t lambda = 4, alpha = 80, seed = std::time(0); // default phf values
    const bool nodiv = true;
	PHF::init<u_int64_t, nodiv>(&phf, data, n, lambda, alpha, seed);
	end = clock();

	warnx("[phf] found perfect hash for %zu keys in %fs", n, (double)(end - begin) / CLOCKS_PER_SEC);



	if(bench_lookup)
	{
		u_int64_t dumb=0;
		u_int64_t mphf_value;
		begin = clock();
		for (u_int64_t i = 0; i < n; i++)
		{
			mphf_value =  PHF::hash<u_int64_t>(&phf,data[rand()%n]);
			//do some silly work
			dumb+= mphf_value;
		}

		end = clock();
		printf("PHF %lu lookups in  %.2fs,  approx  %.2f ns per lookup   (fingerprint %llu)  \n", n, (double)(end - begin) / CLOCKS_PER_SEC,  ((double)(end - begin) / CLOCKS_PER_SEC)*1000000000/n,dumb);
	}


}

struct uint64_adaptor
{
    emphf::byte_range_t operator()(u_int64_t const& s) const
    {
        const uint8_t* buf = reinterpret_cast<uint8_t const*>(&s);
        const uint8_t* end = buf + sizeof(u_int64_t); // add the null terminator
        return emphf::byte_range_t(buf, end);
    }
};


// code from stackoverflow, thank you stack overflow.
// http://stackoverflow.com/questions/15904896/range-based-for-loop-on-a-dynamic-array
template <typename T>
struct wrapped_array {
    wrapped_array(T* first, T* last) : begin_ {first}, end_ {last} {}
    wrapped_array(T* first, std::ptrdiff_t size)
        : wrapped_array {first, first + size} {}

    T*  begin() const noexcept { return begin_; }
    T*  end() const noexcept { return end_; }

    T* begin_;
    T* end_;
};

template <typename T>
wrapped_array<T> wrap_array(T* first, std::ptrdiff_t size) noexcept
{ return {first, size}; }

#ifdef EMPHF_HEM
    typedef emphf::mphf_hem<emphf::jenkins64_hasher> mphf_t;
#else
    #ifdef EMPHF_SCAN
        typedef emphf::hypergraph_sorter_scan<uint32_t, emphf::mmap_memory_model> HypergraphSorter32; // follows compute_mphf_scan_mmap
        //typedef emphf::hypergraph_sorter_scan<uint32_t, emphf::internal_memory_model> HypergraphSorter32; // follows compute_mphf_scan
    #else
        typedef emphf::hypergraph_sorter_seq<emphf::hypergraph<uint32_t>> HypergraphSorter32; // follows compute_mphf_seq
    #endif
    typedef emphf::jenkins64_hasher BaseHasher;
    typedef emphf::mphf<BaseHasher> mphf_t;
#endif


mphf_t mphf; // let's put it as global variable so that mphf isn't destroyed after end of function
void do_emphf()
{
    clock_t begin, end;
	begin = clock();

#ifndef EMPHF_HEM
    HypergraphSorter32 sorter;
#endif

    uint64_adaptor adaptor;

    auto data_iterator = emphf::range(static_cast<const u_int64_t*>(data), static_cast<const u_int64_t*>(data+n));
    //auto data_iterator = wrap_array(static_cast<const u_int64_t*>(data),n); // would also work, just to show off that i now know two solutions to wrap an iterator for emphf


#ifdef VANILLA_EMPHF
    #ifdef EMPHF_HEM
        emphf::mmap_memory_model mm;
        mphf_t(mm, n, data_iterator, adaptor).swap(mphf);
        string emphf_type = "vanilla emphf HEM";
    #else
        #ifdef EMPHF_SCAN
            string emphf_type = "vanilla emphf scan";
        #else
            #ifdef EMPHF_SEQ
                string emphf_type = "vanilla emphf seq";
            #endif
        #endif
        mphf_t(sorter, n, data_iterator, adaptor).swap(mphf);
    #endif
#else
    gatb::core::tools::dp::IteratorListener* progress = nullptr; // added by gatb in emphf
    mphf_t(sorter, n, data_iterator, adaptor, progress).swap(mphf);
    string emphf_type = "gatb-core emphf";
#endif

	end = clock();

	warnx("[%s] constructed perfect hash for %zu keys in %fs", emphf_type.c_str(), n, (double)(end - begin) / CLOCKS_PER_SEC);


	if(bench_lookup)
	{
		u_int64_t dumb=0;
		u_int64_t mphf_value;
		begin = clock();
		for (u_int64_t i = 0; i < n; i++)
		{
			mphf_value = mphf.lookup(data[rand()%n],adaptor);
			//do some silly work
			dumb+= mphf_value;
		}

		end = clock();
		printf("emphf %lu lookups in  %.2fs,  approx  %.2f ns per lookup   (fingerprint %llu)  \n", n, (double)(end - begin) / CLOCKS_PER_SEC,  ((double)(end - begin) / CLOCKS_PER_SEC)*1000000000/n,dumb);
	}

}



int main (int argc, char* argv[])
{

    if ( argc < 2 )
        cout << "Constructing a MPHF with (default) n=" << n << " elements" << std::endl;
    else
    {
        n = strtoul(argv[1], NULL,0);
        cout << "Constructing a MPHF with n=" << n << " elements" << std::endl;
    }
	for (int ii=2; ii<argc; ii++)
	{
		if(!strcmp("-bench",argv[ii])) bench_lookup= true;
	}

    // create a bunch of sorted 64-bits integers (it doesnt matter if they were sorted actually)
    // adapted from http://stackoverflow.com/questions/14009637/c11-random-numbers
    static std::mt19937_64 rng;
    rng.seed(std::mt19937_64::default_seed);
    data = new u_int64_t[n];
    for (u_int64_t i = 1; i < n; i++)
        data[i] = rng();

    memory_usage("initial data allocation");

    cout << endl << "Construction with 'emphf' library.. " << endl;
    do_emphf();
    memory_usage("after emphf construction", "emphf");

    cout << endl << "Construction with 'phf' library.. " << endl;
    do_phf();
    memory_usage("after phf construction","phf");
}
