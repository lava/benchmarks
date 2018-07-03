#!/bin/bash

trap 'kill %1' EXIT

LIBPROCESS_PORT=1337 ./server &

for size in 8092 16184 24276 32368 40460 48552 56644 64736 72828;
do
  echo ${size}
  hyperfine \
	-p "wget http://localhost:1337/my_process/prepare-json-cache?size=${size}" \
	-- \
	"wget http://localhost:1337/my_process/json-cached-stringify?size=${size}"
done
