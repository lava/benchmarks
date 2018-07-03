#pragma once


#include <chrono>
#include <functional>
#include <vector>
#include <string>

#ifndef DISABLE_CPU_COUNTERS

#include <string.h>
#include <sys/types.h>
#include <linux/perf_event.h>
#include <sys/syscall.h>
#include <unistd.h>


namespace benchmark {
namespace internal {
namespace perf {


// From tools/perf/sys-perf.h in the linux source tree.
// (basically just exists to provide type-checking for args)
static int
sys_perf_event_open(struct perf_event_attr *attr,
                      pid_t pid, int cpu, int group_fd,
                      unsigned long flags)
{
        return syscall(__NR_perf_event_open, attr, pid, cpu,
                       group_fd, flags);
}


int createHardwareCounter(enum perf_hw_id type)
{
	struct perf_event_attr pea = {0};
	pea.type = PERF_TYPE_HARDWARE;
	pea.config = type;

        // pid == 0: counter is attached to the current task
        //      TODO - does the above mean current thread, or process?
        // cpu == -1: count on all cpu's
        // group_fd == -1: create counter in a new group
	int fd = sys_perf_event_open(&pea, 0, -1, -1, PERF_FLAG_FD_CLOEXEC);
        if (fd < 0) {
                printf("Error %d: %s\n", errno, strerror(errno));
                if (errno == EACCES) {
                        printf("Try adjusting /proc/sys/kernel/perf_event_paranoid\n");
                }
                if (errno == ENOSYS) {
                        printf("No CONFIG_PERF_EVENTS=y kernel support configured?\n");
                }
                return 1;
        }

	return fd;
}


uint64_t readCounter(int fd)
{
	uint64_t value;
	// todo - error handling
	read(fd, &value, sizeof(value));

	return value;
}



}}} // namespace benchmark::internal::perf

#endif // !DISABLE_CPU_COUNTERS

namespace benchmark {

struct DataSeries {
	std::string name;

	std::vector<double> xpos;
	std::vector<uint64_t> cycles;
	std::vector<uint64_t> instructions;
	std::vector<std::chrono::high_resolution_clock::duration> walltime;

	DataSeries(const std::string& name, size_t intendedSize):
	  name(name)
	{
	  // todo - only reserve for the data that's actually used
	  xpos.reserve(intendedSize);
	  cycles.reserve(intendedSize);
	  instructions.reserve(intendedSize);
	  walltime.reserve(intendedSize);
	}
};


struct BenchmarkContext {
	// todo - use more templates to make this type-safe, and don't
	// mix input and output arguments

	// input
	int iteration;

	// output
	double xpos;

	// i/o
	void* opaque;
};


struct BenchmarkOptions {
	// todo - these two should probably a bit more general
	int repetitions;

	bool include_cpu_counters;
	bool include_walltime;

	std::function<void(BenchmarkContext&)> globalSetup;
	std::function<void(BenchmarkContext&)> globalTeardown;

	// This 
	std::function<void(BenchmarkContext&)> iterationSetup;
	std::function<void(BenchmarkContext&)> iterationTeardown;

	// maybe
	//std::function<bool(void)> discardResult;
};

template<typename BenchmarkFn>
DataSeries benchmark_series(const std::string& name, BenchmarkFn f, const BenchmarkOptions& options);

// Convenience function

template<typename BenchmarkFn>
DataSeries benchmark_series(const std::string& name, BenchmarkFn f)
{
	BenchmarkOptions options;
	options.repetitions = 1;
	options.include_walltime = true;
	options.include_cpu_counters = true;
	return benchmark_series(name, f, options);
}

// General case


// The other option (still todo) would be e.g. `benchmark_comparison()`, i.e. where the
// x axis does not represent some changing numeric quantity but rather a set of 
template<typename BenchmarkFn>
DataSeries benchmark_series(const std::string& name, BenchmarkFn f, const BenchmarkOptions& options)
{
	BenchmarkContext context;

	if (options.globalSetup) {
		options.globalSetup(context);
	}

	DataSeries result(name, options.repetitions);
	int cycles_fd = 0;
	int instructions_fd = 0;

	if (options.include_cpu_counters) {
		cycles_fd = internal::perf::createHardwareCounter(PERF_COUNT_HW_CPU_CYCLES);
		instructions_fd = internal::perf::createHardwareCounter(PERF_COUNT_HW_INSTRUCTIONS);
	}

	for (int i=0; i<options.repetitions; ++i) {
		std::chrono::high_resolution_clock::time_point walltime_before, walltime_after;
		uint64_t cycles_before, cycles_after;
		uint64_t instructions_before, instructions_after;

		context.iteration = i;
		context.xpos = i;

		if (options.iterationSetup) {
			options.iterationSetup(context);
		}

		// initializing these in order of sensitivity
		if (options.include_walltime) {
			walltime_before = std::chrono::high_resolution_clock::now();
		}

		if (options.include_cpu_counters) {
			// todo - we could probably get a pretty accurate estimate of instructions
			// it takes to read the cycles counter and subtract that
			cycles_before = internal::perf::readCounter(cycles_fd);
			instructions_before = internal::perf::readCounter(instructions_fd);
		}

		f(context);

		// todo - is it more useful to read these in the same order as they were
		// initialized, or in the opposite order?
		if (options.include_cpu_counters) {
			instructions_after = internal::perf::readCounter(instructions_fd);
			cycles_after = internal::perf::readCounter(cycles_fd);
		}

		if (options.include_walltime) {
			walltime_after = std::chrono::high_resolution_clock::now();
			result.walltime.push_back(walltime_after - walltime_before);
		}

		if (options.iterationTeardown) {
			options.iterationTeardown(context);
		}

		if (options.include_cpu_counters) {
			result.cycles.push_back(cycles_after - cycles_before);
			result.instructions.push_back(instructions_after - instructions_before);
		}

		result.xpos.push_back(context.xpos);

		break;
	}

	if (options.globalTeardown) {
		options.globalTeardown(context);
	}

	return result;
}

} // namespace benchmark

#include <iostream>

namespace benchmark {


std::ostream& operator<<(std::ostream& str, std::chrono::high_resolution_clock::duration duration)
//std::ostream& operator<<(std::ostream& str, std::chrono::duration duration)
{
	return str << std::chrono::duration_cast<std::chrono::duration<double, std::ratio<1>>>(duration).count();
}

void plot_data(const DataSeries& series)
{
	bool walltime = series.walltime.size() > 0;
	bool cycles = series.cycles.size() > 0;
	bool instructions = series.cycles.size() > 0;

	// header
	std::cout << "Iteration\t" << (walltime ? "Time\t" : "") << (cycles ? "Cycles\t" : "") << (instructions ? "Instructions\t" : "") << std::endl;

	for (int i=0; i<series.xpos.size(); ++i) {
		std::cout << series.xpos.at(i) << ":\t";

		if (walltime) {
			std::cout << series.walltime.at(i) << "s\t";
		}

		if (cycles) {
			std::cout << series.cycles.at(i) << "\t";
		}

		if (instructions) {
			std::cout << series.instructions.at(i) << "\t";
		}

		std::cout << std::endl;
	}
}


} // namespace benchmark
