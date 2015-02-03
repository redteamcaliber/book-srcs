#include <stdio.h>
#include <string.h>

#if defined(_WIN32) || defined(_WIN64)
  #include <windows.h>
  #include <psapi.h>
#elif defined(__unix__) || defined(__unix) || defined(unix) || (defined(__APPLE__) && defined(__MACH__))
  #include <unistd.h>
  #include <sys/resource.h>
  #include <sys/sysinfo.h>  

  #if defined(__APPLE__) && defined(__MACH__)
    #include <mach/mach.h>
  #elif (defined(_AIX) || defined(__TOS__AIX__)) || (defined(__sun__) || defined(__sun) || defined(sun) && (defined(__SVR4) || defined(__svr4__)))
    #include <fcntl.h>
    #include <procfs.h>
  #endif
#else
  #error "Cannot define getPeakRSS( ) or getCurrentRSS( ) for an unknown OS."
#endif

/**
 * Returns the peak (maximum so far) resident set size (physical
 * memory use) measured in bytes, or zero if the value cannot be
 * determined on this OS.
 */
size_t getPeakRSS( )
{
#if defined(_WIN32) || defined(_WIN64)
  /* Windows -------------------------------------------------- */
  PROCESS_MEMORY_COUNTERS info;

  GetProcessMemoryInfo( GetCurrentProcess( ), &info, sizeof(info) );

  return (size_t)info.PeakWorkingSetSize;
#elif (defined(_AIX) || defined(__TOS__AIX__)) || (defined(__sun__) || defined(__sun) || defined(sun) && (defined(__SVR4) || defined(__svr4__)))
  /* AIX and Solaris ------------------------------------------ */
  struct psinfo psinfo;
  int fd = -1;

  if ( (fd = open( "/proc/self/psinfo", O_RDONLY )) == -1 )
    return (size_t)0L;      /* Can't open? */

  if ( read( fd, &psinfo, sizeof(psinfo) ) != sizeof(psinfo) )
  {
    close( fd );
    return (size_t)0L;      /* Can't read? */
  }

  close( fd );

  return (size_t)(psinfo.pr_rssize * 1024L);
#elif defined(__unix__) || defined(__unix) || defined(unix) || (defined(__APPLE__) && defined(__MACH__))
  /* BSD, Linux, and OSX -------------------------------------- */
  struct rusage rusage;

  getrusage( RUSAGE_SELF, &rusage );
#if defined(__APPLE__) && defined(__MACH__)
  return (size_t)rusage.ru_maxrss;
#else
  return (size_t)(rusage.ru_maxrss * 1024L);
#endif

#else
  /* Unknown OS ----------------------------------------------- */
  return (size_t)0L;          /* Unsupported. */
#endif
}

/**
 * Returns the current resident set size (physical memory use) measured
 * in bytes, or zero if the value cannot be determined on this OS.
 */
size_t getCurrentRSS( )
{
#if defined(_WIN32) || defined(_WIN64)
  /* Windows -------------------------------------------------- */
  PROCESS_MEMORY_COUNTERS info;

  GetProcessMemoryInfo( GetCurrentProcess( ), &info, sizeof(info) );

  return (size_t)info.WorkingSetSize;

#elif defined(__APPLE__) && defined(__MACH__)
  /* OSX ------------------------------------------------------ */
  struct mach_task_basic_info info;

  mach_msg_type_number_t infoCount = MACH_TASK_BASIC_INFO_COUNT;

  if ( task_info( mach_task_self( ), MACH_TASK_BASIC_INFO,
        (task_info_t)&info, &infoCount ) != KERN_SUCCESS )
    return (size_t)0L;      /* Can't access? */

  return (size_t)info.resident_size;
#elif defined(__linux__) || defined(__linux) || defined(linux) || defined(__gnu_linux__)
  /* Linux ---------------------------------------------------- */
  long rss = 0L;
  FILE* fp = NULL;

  if ( (fp = fopen( "/proc/self/statm", "r" )) == NULL )
    return (size_t)0L;      /* Can't open? */

  if ( fscanf( fp, "%*s%ld", &rss ) != 1 )
  {
    fclose( fp );
    return (size_t)0L;      /* Can't read? */
  }

  fclose( fp );

  return (size_t)rss * (size_t)sysconf( _SC_PAGESIZE);
#else
  /* AIX, BSD, Solaris, and Unknown OS ------------------------ */
  return (size_t)0L;          /* Unsupported. */
#endif
}

const char *human_readable(unsigned long x)
{
  static char buffer[64];
  float y;

  if (x < 1024)
    sprintf(buffer, "%lu Bytes", x);
  else if (x < 1024UL*1024UL)
  {
    y = (float)x / 1024.0f;
    sprintf(buffer, "%.1f kBytes", y);
  }
  else if (x < 1024UL*1024UL*1024UL)
  {
    y = (float)x / (1024.0f * 1024.0f);
    sprintf(buffer, "%.1f MBytes", y);
  }
  else
  {
    y = (float)x / (1024.0f * 1024.0f * 1024.0f);
    sprintf(buffer, "%.1f GBytes", y);
  }

  return buffer;
}

void main(void)
{
#if defined(__linux)
  struct sysinfo si;

  sysinfo(&si);   
  printf("System wide available memory: %s\n", human_readable(si.freeram));
#endif

  printf("RSS: %s\n"
         "Peak RSS: %s\n",
         human_readable(getCurrentRSS()), human_readable(getPeakRSS()));
}