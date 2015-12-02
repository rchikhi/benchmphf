// this code is from GATB-core, more specifically SystemInfoCommon.cpp

/*****************************************************************************
 *   GATB : Genome Assembly Tool Box
 *   Copyright (C) 2014  INRIA
 *   Authors: R.Chikhi, G.Rizk, E.Drezen
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Affero General Public License as
 *  published by the Free Software Foundation, either version 3 of the
 *  License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Affero General Public License for more details.
 *
 *  You should have received a copy of the GNU Affero General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*****************************************************************************/

#include <cstring>
using namespace std;

/*********************************************************************
                #        ###  #     #  #     #  #     #
                #         #   ##    #  #     #   #   #
                #         #   # #   #  #     #    # #
                #         #   #  #  #  #     #     #
                #         #   #   # #  #     #    # #
                #         #   #    ##  #     #   #   #
                #######  ###  #     #   #####   #     #
*********************************************************************/

#ifdef __linux__

#include "sys/sysinfo.h"
#include <unistd.h>
#include <sys/resource.h>
#include <sys/times.h>
#include <sys/vtimes.h>

/********************************************************************************/
size_t getNbCores ()
{
    size_t result = 0;

    /** We open the "/proc/cpuinfo" file. */
    FILE* file = fopen ("/proc/cpuinfo", "r");
    if (file)
    {
        char buffer[256];
        while (fgets(buffer, sizeof(buffer), file))  {  if (strstr(buffer, "processor") != NULL)  { result ++;  }  }
        fclose (file);
    }

    if (result==0)  { result = 1; }

    return result;
}

/********************************************************************************/
string getHostName ()
{
    string result;

    char hostname[1024];
    hostname[1023] = '\0';
    gethostname (hostname, sizeof(hostname)-1);
    result.assign (hostname, strlen(hostname));

    return result;
}

/********************************************************************************/
u_int64_t getMemoryPhysicalTotal ()
{
    struct sysinfo memInfo;
    sysinfo (&memInfo);
    u_int64_t result = memInfo.totalram;
    result *= memInfo.mem_unit;
    return result;
}

/********************************************************************************/
u_int64_t getMemoryPhysicalUsed ()
{
    struct sysinfo memInfo;
    sysinfo (&memInfo);
    u_int64_t result = memInfo.totalram - memInfo.freeram;
    result *= memInfo.mem_unit;
    return result;
}

/********************************************************************************/
u_int64_t getMemoryBuffers ()
{
    struct sysinfo memInfo;
    sysinfo (&memInfo);
    u_int64_t result = memInfo.bufferram;
    result *= memInfo.mem_unit;
    return result;
}

/********************************************************************************/
u_int64_t getMemorySelfUsed ()
{
    FILE* file = fopen("/proc/self/status", "r");
    u_int64_t result = 0;
    char line[128];

    while (fgets(line, 128, file) != NULL)
    {
        if (strncmp(line, "VmRSS:", 6) == 0)
        {
            char* loop = line;
            result = strlen(line);
            while (*loop < '0' || *loop > '9') loop++;
            loop[result-3] = '\0';
            result = atoi(loop);
            break;
        }
    }
    fclose(file);
    return result;
}

/********************************************************************************/
u_int64_t getMemorySelfMaxUsed ()
{
    u_int64_t result = 0;
    struct rusage usage;
    if (getrusage(RUSAGE_SELF, &usage)==0)  {  result = usage.ru_maxrss;  }
    return result;
}
#endif

/*********************************************************************
            #     #     #      #####   #######   #####
            ##   ##    # #    #     #  #     #  #     #
            # # # #   #   #   #        #     #  #
            #  #  #  #     #  #        #     #   #####
            #     #  #######  #        #     #        #
            #     #  #     #  #     #  #     #  #     #
            #     #  #     #   #####   #######   #####
*********************************************************************/

#ifdef __APPLE__

#include <sys/types.h>
#include <sys/sysctl.h>

#include <mach/vm_statistics.h>
#include <mach/mach_types.h>
#include <mach/mach_init.h>
#include <mach/mach_host.h>
#include <mach/mach.h>

/********************************************************************************/
size_t getNbCores ()
{
    int numCPU = 0;

    int mib[4];
    size_t len = sizeof(numCPU);

    /* set the mib for hw.ncpu */
    mib[0] = CTL_HW;
    mib[1] = HW_AVAILCPU;  // alternatively, try HW_NCPU;

    /* get the number of CPUs from the system */
    sysctl (mib, 2, &numCPU, &len, NULL, 0);

    if (numCPU < 1)
    {
        mib[1] = HW_NCPU;
        sysctl (mib, 2, &numCPU, &len, NULL, 0 );
    }

    if (numCPU < 1)  {  numCPU = 1;  }

    return numCPU;
}

/********************************************************************************/
string getHostName ()
{
    string result;

    char hostname[1024];
    hostname[1023] = '\0';
    gethostname (hostname, sizeof(hostname)-1);
    result.assign (hostname, strlen(hostname));

    return result;
}

/********************************************************************************/
u_int64_t getMemoryPhysicalTotal ()
{
    int mib[2] = { CTL_HW, HW_MEMSIZE };
    size_t namelen = sizeof(mib) / sizeof(mib[0]);
    u_int64_t size;
    size_t len = sizeof(size);

    if (sysctl(mib, namelen, &size, &len, NULL, 0) < 0)
    {
        fprintf (stderr,"Unable to get physical memory");
		return 0;
    }
    else
    {
        return size;
    }
}

/********************************************************************************/
u_int64_t getMemoryPhysicalUsed ()
{
    // see http://stackoverflow.com/questions/63166/how-to-determine-cpu-and-memory-consumption-from-inside-a-process
    vm_size_t page_size;
    mach_port_t mach_port;
    mach_msg_type_number_t count;
    vm_statistics_data_t vm_stats;

    mach_port = mach_host_self();
    count = sizeof(vm_stats) / sizeof(natural_t);
    if (KERN_SUCCESS == host_page_size(mach_port, &page_size) &&
        KERN_SUCCESS == host_statistics(mach_port, HOST_VM_INFO,
                                        (host_info_t)&vm_stats, &count))
    {
        int64_t myFreeMemory = (int64_t)vm_stats.free_count * (int64_t)page_size;

        int64_t used_memory = ((int64_t)vm_stats.active_count +
                       (int64_t)vm_stats.inactive_count +
                       (int64_t)vm_stats.wire_count) *  (int64_t)page_size;
        return myFreeMemory + used_memory;
    }
    else
    {
        fprintf (stderr,"Unable to get free memory");
		return 0;
    }
}

/********************************************************************************/
u_int64_t getMemorySelfUsed ()
{
    struct task_basic_info t_info;
    mach_msg_type_number_t t_info_count = TASK_BASIC_INFO_COUNT;

    if (KERN_SUCCESS != task_info (mach_task_self(), TASK_BASIC_INFO, (task_info_t)&t_info, &t_info_count))
    {
        return -1;
    }

    return t_info.resident_size / 1024;
}

u_int64_t getMemorySelfMaxUsed()   { return 0; }

#endif


