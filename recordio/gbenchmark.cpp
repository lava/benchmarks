#include <benchmark/benchmark.h>

#include <curl/curl.h>

#include <chrono>
#include <iostream>


size_t noop_cb(void *ptr, size_t size, size_t nmemb, void *data) {
  // printf("RECV: %.*s\n", size*nmemb, ptr);
  *(uint64_t*)data += size * nmemb;
  return size * nmemb;
}

static void BM_JsonCall(benchmark::State& state) {

	for (auto _ : state) {
                state.PauseTiming();
		std::string size = std::to_string(state.range(0));
		CURL* prepareHandle = curl_easy_init();
		curl_easy_setopt(prepareHandle, CURLOPT_URL, ("http://localhost:1337/my_process/prepare-json-cache?size=" + size).c_str());
		curl_easy_setopt(prepareHandle, CURLOPT_WRITEFUNCTION, noop_cb);
		curl_easy_perform(prepareHandle);
		curl_easy_cleanup(prepareHandle);

		uint64_t bytes;

		CURL* jsonHandle = curl_easy_init();
		curl_easy_setopt(jsonHandle, CURLOPT_URL, ("http://localhost:1337/my_process/json-cached?size=" + size).c_str());
		curl_easy_setopt(jsonHandle, CURLOPT_WRITEFUNCTION, noop_cb);
		curl_easy_setopt(jsonHandle, CURLOPT_FAILONERROR, true);
		curl_easy_setopt(jsonHandle, CURLOPT_WRITEDATA, &bytes);

		state.ResumeTiming();
		if (curl_easy_perform(jsonHandle) != CURLE_OK) {
			state.SkipWithError("Failed request for json");
			break;
		}
                state.PauseTiming();

		state.counters["bytes"] = bytes;

		curl_easy_cleanup(jsonHandle);
		state.ResumeTiming();
	}
}

static void BM_RecordioCall(benchmark::State& state) {
	for (auto _ : state) {
                state.PauseTiming();
		std::string size = std::to_string(state.range(0));
		CURL* prepareHandle = curl_easy_init();
		curl_easy_setopt(prepareHandle, CURLOPT_URL, ("http://localhost:1337/my_process/prepare-json-cache?size=" + size).c_str());
		curl_easy_setopt(prepareHandle, CURLOPT_WRITEFUNCTION, noop_cb);
		curl_easy_perform(prepareHandle);
		curl_easy_cleanup(prepareHandle);

		CURL* recordioHandle = curl_easy_init();
		curl_easy_setopt(recordioHandle, CURLOPT_URL, ("http://localhost:1337/my_process/recordio-cached?size=" + size).c_str());
		curl_easy_setopt(recordioHandle, CURLOPT_WRITEFUNCTION, noop_cb);
		curl_easy_setopt(recordioHandle, CURLOPT_FAILONERROR, true);

		state.ResumeTiming();
		if (curl_easy_perform(recordioHandle) != CURLE_OK) {
			state.SkipWithError("Failed request for recordio");
			break;
		}
                state.PauseTiming();

		curl_easy_cleanup(recordioHandle);
		state.ResumeTiming();
	}
}


BENCHMARK(BM_JsonCall)
	->UseRealTime()
	->RangeMultiplier(2)
	->Range(2, 524288)
	->Unit(benchmark::kMillisecond);

BENCHMARK(BM_RecordioCall)
	->UseRealTime()
	->RangeMultiplier(2)
	->Range(8, 524288)
	->Unit(benchmark::kMillisecond);

BENCHMARK_MAIN();
