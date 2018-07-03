
import matplotlib.pyplot as plt
import os
import re
import sys


# Parse datapoints from a single file.
def extract_datapoints(filepath):
    datapoints = []
    pattern = re.compile("request (\w+) \((\d+) bytes\) - (\d+) (\d+) (\d+)")
    with open(filepath) as file:
        for cnt, line in enumerate(file):
            matched = pattern.match(line)
            datapoint = {"id" :         int(matched.group(1), 16),
                         "size" :       int(matched.group(2)),
                         "received":    int(matched.group(3)),
                         "processing" : int(matched.group(4)),
                         "finished":    int(matched.group(5)),
                        }
            datapoints.append(datapoint)
    return datapoints


# Entry point.
if (len(sys.argv) < 2):
    print("Provide the file name to parse")
    sys.exit(1)

# Get target files.
target_folder = sys.argv[1]
filepaths = []
for (dirpath, dirnames, filenames) in os.walk(target_folder):
    filepaths.extend([os.path.join(target_folder, f) for f in filenames])
    break

print(filepaths)

# Combine data from multiple files and note master failovers.
datapoints = []
master_restarts = []
for f in filepaths:
    datapoints.extend(extract_datapoints(f))
    master_restarts.append(len(datapoints) - 1)
master_restarts.pop() # Last item is not a failover but cluster termination.

x_axis = [d["received"] for d in datapoints]
finished = [d["finished"] for d in datapoints]
size_over_time = [(d["size"] / 1024) for d in datapoints]
time_in_queue = [((d["processing"] - d["received"])) for d in datapoints]
time_in_crunching = [((d["finished"] - d["processing"])) for d in datapoints]
time_total = [((d["finished"] - d["received"])) for d in datapoints]
ratio_state_other = float(len(datapoints)) / (datapoints[-1]["id"] - datapoints[0]["id"] + 1)

# TODO: Number of requests per second / minute, variance in queue / crunch time.

# Figure 1.
plt.figure(1)
plt.suptitle("state.json: {0:.0%} of all HTTP requests".format(ratio_state_other))

plt.subplot(211)
plt.plot(finished, size_over_time, "m--")
plt.xlabel("timestamp request received")
plt.ylabel("response size in KB")
plt.grid(True)
plt.ticklabel_format(style = "plain")
for r in master_restarts:
    plt.axvline(x=datapoints[r]["finished"])

plt.subplot(212)
plt.plot(finished, time_in_queue, "r,", label="red: queued")
plt.plot(finished, time_in_crunching, "b,", label="blue: crunching")
plt.fill_between(finished, time_total, color="gray", alpha="0.5")
plt.xlabel("timestamp request finished")
plt.ylabel("time (in ms)")
plt.legend()
plt.grid(True)
plt.ticklabel_format(style = "plain")
for r in master_restarts:
    plt.axvline(x=datapoints[r]["finished"])

plt.show()

