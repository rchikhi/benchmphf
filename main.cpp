/*
 * benchmark of two mphf libraries:
 * - phf
 * - emphf
 *
 */

#include <unistd.h>

#include <cstdlib> // stdtoul
#include <random>
#include <err.h>       /* for warnx() */
#include "phf/phf.cc" /* phf library */
#include "libcmph/include/cmph.h" /* cmph library */
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
bool from_disk = false;
u_int64_t nb_in_bench_file;

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




//taken from emphf for consistent bench
struct stats_accumulator {
	stats_accumulator()
	: m_n(0)
	, m_mean(0)
	, m_m2(0)
	{}
	
	void add(double x)
	{
		m_n += 1;
		auto delta = x - m_mean;
		m_mean += delta / m_n;
		m_m2 += delta * (x - m_mean);
	}
	
	double mean() const
	{
		return m_mean;
	}
	
	double variance() const
	{
		return m_m2 / (m_n - 1);
	}
	
	double relative_stddev() const
	{
		return std::sqrt(variance()) / mean() * 100;
	}
	
private:
	double m_n;
	double m_mean;
	double m_m2;
};


u_int64_t _previousMem = 0;
unsigned long memory_usage(string message="", string mphftype="")
{
    // from Progress.cpp of gatb-core

    /** We get the memory used by the current process. */
    u_int64_t mem = getMemorySelfUsed() / 1024;

    u_int64_t memMaxProcess = getMemorySelfMaxUsed() / 1024;

    /** We format the string to be displayed. */
    char tmp[128];
    snprintf (tmp, sizeof(tmp), "   memory [current, maximum (maxRSS)]: [%4llu, %4llu] MB ",
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


// iterator from disk file of u_int64_t with buffered read,  todo template
class bfile_iterator : public std::iterator<std::forward_iterator_tag, u_int64_t>{
public:
	
	bfile_iterator()
	: _is(nullptr)
	, _pos(0) ,_inbuff (0), _cptread(0)
	{
		_buffsize = 10000;
		_buffer = (u_int64_t *) malloc(_buffsize*sizeof(u_int64_t));
	}
	
	bfile_iterator(const bfile_iterator& cr)
	{
		_buffsize = cr._buffsize;
		_pos = cr._pos;
		_is = cr._is;
		_buffer = (u_int64_t *) malloc(_buffsize*sizeof(u_int64_t));
		memcpy(_buffer,cr._buffer,_buffsize*sizeof(u_int64_t) );
		_inbuff = cr._inbuff;
		_cptread = cr._cptread;
		_elem = cr._elem;
	}
	
	bfile_iterator(FILE* is): _is(is) , _pos(0) ,_inbuff (0), _cptread(0)
	{
		_buffsize = 10000;
		_buffer = (u_int64_t *) malloc(_buffsize*sizeof(u_int64_t));
		int reso = fseek(_is,0,SEEK_SET);
		advance();
	}
	
	~bfile_iterator()
	{
		if(_buffer!=NULL)
			free(_buffer);
	}
	
	
	u_int64_t const& operator*()  {  return _elem;  }
	
	bfile_iterator& operator++()
	{
		advance();
		return *this;
	}
	
	friend bool operator==(bfile_iterator const& lhs, bfile_iterator const& rhs)
	{
		if (!lhs._is || !rhs._is)  {  if (!lhs._is && !rhs._is) {  return true; } else {  return false;  } }
		assert(lhs._is == rhs._is);
		return rhs._pos == lhs._pos;
	}
	
	friend bool operator!=(bfile_iterator const& lhs, bfile_iterator const& rhs)  {  return !(lhs == rhs);  }
private:
	void advance()
	{
		_pos++;
		if(_cptread >= _inbuff)
		{
			int res = fread(_buffer,sizeof(u_int64_t),_buffsize,_is);
			_inbuff = res; _cptread = 0;
			
			if(res == 0)
			{
				_is = nullptr;
				_pos = 0;
				return;
			}
		}
		
		_elem = _buffer[_cptread];
		_cptread ++;
	}
	u_int64_t _elem;
	FILE * _is;
	unsigned long _pos;
	
	u_int64_t * _buffer; // for buffered read
	int _inbuff, _cptread;
	int _buffsize;
};


class file_binary{
public:
	
	file_binary(const char* filename)
	{
		_is = fopen(filename, "rb");
		if (!_is) {
			throw std::invalid_argument("Error opening " + std::string(filename));
		}
	}
	~file_binary()
	{
		fclose(_is);
	}
	
	bfile_iterator begin() const
	{
		return bfile_iterator(_is);
	}
	
	bfile_iterator end() const {return bfile_iterator(); }
	
	size_t        size () const  {  return 0;  }//todo ?
private:
	FILE * _is;
};









u_int64_t *data;
std::vector<u_int64_t> data_bench;
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
	
    begin = clock();
    PHF::compact(&phf);
    end = clock();
    warnx("compacted displacement map in %fs", (double)(end - begin) / CLOCKS_PER_SEC);

    int d_bits = ffsl((long)phf_powerup(phf.d_max));
    double k_bits = ((double)phf.r * d_bits) / n;
    double g_load = (double)n / phf.r;
    warnx("r:%zu m:%zu d_max:%zu d_bits:%d k_bits:%.2f g_load:%.2f", phf.r, phf.m, phf.d_max, d_bits, k_bits, g_load);


	
	if(bench_lookup)
	{
		
	
		
		//bench procedure taken from emphf
		stats_accumulator stats;
		double tick = emphf::get_time_usecs();
		size_t lookups = 0;
		static const size_t lookups_per_sample = 1 << 16;
		u_int64_t dumb=0;
		double elapsed;
		size_t runs = 10;
		u_int64_t mphf_value;

		for (size_t run = 0; run < runs; ++run) {
			for (size_t ii = 0; ii < data_bench.size(); ++ii) {
				
				mphf_value =  PHF::hash<u_int64_t>(&phf,data_bench[ii]);

				//do some silly work
				dumb+= mphf_value;
				
				if (++lookups == lookups_per_sample) {
					elapsed = emphf::get_time_usecs() - tick;
					stats.add(elapsed / (double)lookups);
					tick = emphf::get_time_usecs();
					lookups = 0;
				}
			}
		}
		printf("PHF bench lookups average %.2f ns +- stddev  %.2f %%   (fingerprint %llu)  \n", 1000.0*stats.mean(),stats.relative_stddev(),dumb);
		
		
	}
	
	
//	if(bench_lookup)
//	{
//		u_int64_t dumb=0;
//		u_int64_t mphf_value;
//		begin = clock();
//		for (u_int64_t i = 0; i < n; i++)
//		{
//			mphf_value =  PHF::hash<u_int64_t>(&phf,data[i]);
//			//do some silly work
//			dumb+= mphf_value;
//		}
//		
//		end = clock();
//		printf("PHF %lu lookups in  %.2fs,  approx  %.2f ns per lookup   (fingerprint %llu)  \n", n, (double)(end - begin) / CLOCKS_PER_SEC,  ((double)(end - begin) / CLOCKS_PER_SEC)*1000000000/n,dumb);
//	}

	
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
	
	
	
	
	
	//auto data_iterator = emphf::range(static_cast<const u_int64_t*>(data), static_cast<const u_int64_t*>(data+n));
	//auto data_iterator = wrap_array(static_cast<const u_int64_t*>(data),n); // would also work, just to show off that i now know two solutions to wrap an iterator for emphf
	
	
#ifdef VANILLA_EMPHF
#ifdef EMPHF_HEM
	emphf::mmap_memory_model mm;
	auto data_iterator = emphf::range(static_cast<const u_int64_t*>(data), static_cast<const u_int64_t*>(data+n));
    //from_disk not implemented
	mphf_t(mm, n, data_iterator, adaptor).swap(mphf); // TODO: ajouter gamma, log2_expected_bucket (controle nb buckets)
	string emphf_type = "vanilla emphf HEM";
#else
#ifdef EMPHF_SCAN
	string emphf_type = "vanilla emphf scan";
#else
#ifdef EMPHF_SEQ
	string emphf_type = "vanilla emphf seq";
#endif
#endif
	if(from_disk)
	{
		auto data_iterator = file_binary("keyfile");
		mphf_t(sorter, n, data_iterator, adaptor).swap(mphf); 
	}
	else
	{
		auto data_iterator = emphf::range(static_cast<const u_int64_t*>(data), static_cast<const u_int64_t*>(data+n));
		mphf_t(sorter, n, data_iterator, adaptor).swap(mphf);
	}
#endif
#else
	gatb::core::tools::dp::IteratorListener* progress = nullptr; // added by gatb in emphf
	mphf_t(sorter, n, data_iterator, adaptor, progress).swap(mphf);
	string emphf_type = "gatb-core emphf";
#endif
	
	end = clock();
	
	warnx("[%s] constructed perfect hash for %zu keys in %fs", emphf_type.c_str(), n, (double)(end - begin) / CLOCKS_PER_SEC);
	
	
	if(bench_lookup && !from_disk)
	{
		u_int64_t dumb=0;
		u_int64_t mphf_value;
		begin = clock();
		for (u_int64_t i = 0; i < n; i++)
		{
			mphf_value = mphf.lookup(data[i],adaptor);
			//do some silly work
			dumb+= mphf_value;
		}
		
		end = clock();
		printf("emphf %lu lookups in  %.2fs,  approx  %.2f ns per lookup   (fingerprint %llu)  \n", n, (double)(end - begin) / CLOCKS_PER_SEC,  ((double)(end - begin) / CLOCKS_PER_SEC)*1000000000/n,dumb);
	}
	
	if (bench_lookup && from_disk)
	{
		auto input_range = file_binary("benchfile");

		vector<u_int64_t> sample;
		u_int64_t mphf_value;
		
		//copy sample in ram
		for (auto const& key: input_range) {
			sample.push_back(key);
		}
		
		//bench procedure taken from emphf
		stats_accumulator stats;
		double tick = emphf::get_time_usecs();
		size_t lookups = 0;
		static const size_t lookups_per_sample = 1 << 16;
		u_int64_t dumb=0;
		double elapsed;
		size_t runs = 10;
		
		for (size_t run = 0; run < runs; ++run) {
			for (size_t ii = 0; ii < sample.size(); ++ii) {
				
				mphf_value = mphf.lookup(sample[ii],adaptor);

				//do some silly work
				dumb+= mphf_value;
				
				if (++lookups == lookups_per_sample) {
					elapsed = emphf::get_time_usecs() - tick;
					stats.add(elapsed / (double)lookups);
					tick = emphf::get_time_usecs();
					lookups = 0;
				}
			}
		}
		printf("EMPHF bench lookups average %.2f ns +- stddev  %.2f %%   (fingerprint %llu)  \n", 1000.0*stats.mean(),stats.relative_stddev(),dumb);
	
	}
	
    // get true mphf size
    // a bit of a dirty hack, could be made cleaner
    std::ofstream os("dummy_emphf", std::ios::binary);
    mphf.save(os);
    std::ifstream is("dummy_emphf", std::ios::binary);
    mphf.load(is);
    size_t file_size = (size_t)is.tellg();
    double bits_per_key = 8.0 * (double)file_size / (double)mphf.size();
    std::cout << "EMPHF bits_per_key: " << bits_per_key << std::endl;


}

void do_chd()
{
	clock_t begin, end;
	begin = clock();
	
	//Open file with newline separated list of keys
	FILE * keys_fd = fopen("keyfile_chd.txt", "r");
	cmph_t *hash = NULL;
	if (keys_fd == NULL)
	{
		fprintf(stderr, "File \"keys_chd.txt\" not found\n");
		exit(1);
	}
	// Source of keys
	cmph_io_adapter_t *source = cmph_io_nlfile_adapter(keys_fd);
	
	cmph_config_t *config = cmph_config_new(source);
	cmph_config_set_algo(config, CMPH_CHD);
	hash = cmph_new(config);
	cmph_config_destroy(config);
	
	
	end = clock();
	
	warnx("[%s] constructed perfect hash for %zu keys in %fs", "CMPH_CHD", n, (double)(end - begin) / CLOCKS_PER_SEC);
	
	
	if (bench_lookup && from_disk)
	{
		
		
		
		
		vector<std::string> sample;
		u_int64_t mphf_value;
		
		FILE * keyb = fopen ("benchfile_chd.txt","r");
		
		//copy sample in ram
		char *line = NULL;
		size_t linecap = 0;
		ssize_t linelen;
		while ((linelen = getline(&line, &linecap, keyb)) > 0)
		{
			sample.push_back(line);
			//printf("%s\n",sample[sample.size()-1].c_str());
		
		}
		
		
		fclose(keyb);

		printf("sample size %lu \n",sample.size());

		
		//bench procedure taken from emphf
		stats_accumulator stats;
		double tick = emphf::get_time_usecs();
		size_t lookups = 0;
		static const size_t lookups_per_sample = 1 << 16;
		u_int64_t dumb=0;
		double elapsed;
		size_t runs = 10;
		
		for (size_t run = 0; run < runs; ++run) {
			for (size_t ii = 0; ii < sample.size(); ++ii) {
				
				mphf_value = cmph_search(hash, sample[ii].c_str(), (cmph_uint32)sample[ii].size());

				//do some silly work
				dumb+= mphf_value;
				
				if (++lookups == lookups_per_sample) {
					elapsed = emphf::get_time_usecs() - tick;
					stats.add(elapsed / (double)lookups);
					tick = emphf::get_time_usecs();
					lookups = 0;
				}
			}
		}
		printf("CMPH_CHD bench lookups average %.2f ns +- stddev  %.2f %%   (fingerprint %llu)  \n", 1000.0*stats.mean(),stats.relative_stddev(),dumb);
		

	}

	
	FILE * mphf_fd = fopen("chd_file", "w");

	 cmph_dump(hash, mphf_fd);

	fclose(mphf_fd);

	FILE * f = fopen("chd_file", "r");
	fseek(f, 0, SEEK_END);
	unsigned long len = (unsigned long)ftell(f);
	fclose(f);
	printf("CHD bits_per_key %.3f \n",len*8/(double)n);
	
	
	//Destroy hash
	cmph_destroy(hash);
	cmph_io_nlfile_adapter_destroy(source);
	fclose(keys_fd);
	
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
		if(!strcmp("-fromdisk",argv[ii])) from_disk= true;
	}

	FILE * key_file = NULL;
	FILE * key_file_chd = NULL;

	FILE * bench_file = NULL;
	FILE * bench_file_chd = NULL;
	
	if(from_disk)
	{
		key_file = fopen("keyfile","w+");
		key_file_chd = fopen("keyfile_chd.txt","w+");
		//simple mehtod to ensure all elements are unique, but not random
		u_int64_t step = ULLONG_MAX / n;
		u_int64_t current = 0;
		fwrite(&current, sizeof(u_int64_t), 1, key_file);
		for (u_int64_t i = 1; i < n; i++)
		{
			current = current + step;
			fwrite(&current, sizeof(u_int64_t), 1, key_file);
			fprintf(key_file_chd,"%llx\n",current);
		}
		fclose(key_file);
		fclose(key_file_chd);

		printf("key file generated \n");
		
		if(bench_lookup)
		{
			bench_file = fopen("benchfile","w+");
			bench_file_chd = fopen("benchfile_chd.txt","w+");

			//create a test file
			//if n < 10 M take all elements, otherwise regular sample to have 10 M elements
			u_int64_t stepb =  n  / 10000000;
			if(stepb==0) stepb=1;
			auto data_iterator = file_binary("keyfile");
			u_int64_t cpt = 0;
			nb_in_bench_file = 0;
			for (auto const& key: data_iterator) {
				if( (cpt % stepb) == 0)
				{
					fwrite(&key, sizeof(u_int64_t), 1, bench_file);
					fprintf(bench_file_chd,"%llx\n",key);

					nb_in_bench_file ++;
				}
				cpt++;
			}
			
			fclose(bench_file);
			fclose(bench_file_chd);

		}
	}
	else
	{
		// create a bunch of sorted 64-bits integers (it doesnt matter if they were sorted actually)
		// adapted from http://stackoverflow.com/questions/14009637/c11-random-numbers
		static std::mt19937_64 rng;
		rng.seed(std::mt19937_64::default_seed);
		data = new u_int64_t[n];
		for (u_int64_t i = 1; i < n; i++)
			data[i] = rng();
		
		
		if(bench_lookup)
		{
			
			u_int64_t stepb =  n  / 10000000;
			if(stepb==0) stepb=1;
			for (int ii=0; ii<n; ii++) {
				if( (ii % stepb) == 0)
				{
					data_bench.push_back(data[ii]);
				}
			}
			
		}
	}
    memory_usage("initial data allocation");

//    cout << endl << "Construction with 'emphf' library.. " << endl;
//    do_emphf();
//    memory_usage("after emphf construction", "emphf");


	if(from_disk)
	{
		cout << endl << "Construction with 'chd' library.. " << endl;
		do_chd();
		memory_usage("after chd construction", "chd");
	}

	
	if(!from_disk)
    {
		cout << endl << "Construction with 'phf' library.. " << endl;
		do_phf();
		memory_usage("after phf construction","phf");
	}

}
