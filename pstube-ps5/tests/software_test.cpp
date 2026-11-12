#include "software_benchmark.h"
#include <assert.h>
#include <stdarg.h>
#include <unistd.h>
static unsigned completed=0;
static void logger(const char* format,...) {
    char line[2048];va_list args;va_start(args,format);vsnprintf(line,sizeof(line),format,args);va_end(args);
    puts(line);if(strstr(line,"SWBENCH result")&&strstr(line,"complete=1")) ++completed;
}
int main() {
    probe::SoftwareBenchmark benchmark(logger);assert(benchmark.start());
    double start=probe::benchmarkTime();while(!benchmark.snapshot().done && probe::benchmarkTime()-start<90) usleep(10000);
    assert(benchmark.snapshot().done);benchmark.cancelAndJoin();assert(completed==6);
    probe::SoftwareBenchmark cancelled(logger);assert(cancelled.start());cancelled.cancelAndJoin();assert(cancelled.snapshot().cancelled);
}
