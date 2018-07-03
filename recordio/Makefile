server: server.cpp
	g++ -O2 -g3 -std=c++11 $+ -o $@  -lmesos

benchmark: nihbenchmark.cpp
	g++ nihbenchmark.cpp


trapped-run = bash -c "trap 'trap - 2; jobs; kill %1; kill %2' 2; $(1)"

run:
	$(call trapped-run,sleep 100 & sleep 200)


# We use '2' over 'SIGINT' below, because for some unknown reason the signal
# name resolution doesn't work on my machine.
#run:
#	bash -c "trap 'jobs; trap - 2; kill %1; kill %2' SIGINT; $(MAKE) _run"

_run:
	LIBPROCESS_PORT=1337 GLOG_v=1 ./server & \
	./node_modules/.bin/webpack --watch & \
	jobs
	python3.6 -m http.server


gbenchmark: gbenchmark.cpp
	g++ -g3 gbenchmark.cpp -lbenchmark -lcurl	
