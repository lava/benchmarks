import matplotlib.pyplot as plt

x = [(i*8092)*59.5 / 1024. for i in range(1, 10)]
json = [17.5, 35.4, 50.4, 68.9, 87.2, 101.2, 116.5, 148.4, 166.8]
json2 = [16.7, 32.7, 49.8, 68.1, 87.8, 102.9, 127.0, 134.1, 156.5]
json3 = [16.6, 34.0, 50.4, 67.5, 84.3, 100.0, 122.8, 135.3, 157.0]
recordio = [12.8, 23.7, 36.2, 48.1, 63.4, 76.3, 87.1, 95.6, 114.7]
zipped = [32.1, 62.6, 91.5, 123.6, 156.6, 185.8, 217.4, 252.9, 288.1]
json_stringify = [13.9, 25.7, 39.1, 51.8, 68.1, 77.1, 88.1, 101.7, 118.0]
json_stringify2 = [14.0, 27.3, 39.3, 51.7, 65.7, 77.3, 89.4, 103.6, 117.2]

#plt.title('Time to serialize and transmit JSON data over loopback interface')
plt.title('Overhead of RecordIO wrapping over ')
plt.xlabel('Response size [KiB]')
plt.ylabel('Time [ms]')
plt.plot(x, json, label="return http::OK(*json)")
#plt.plot(x, json2, label="json2")
#plt.plot(x, json3, label="uncompressed")
plt.plot(x, json_stringify, label="return http::OK(stringify(*json))")
#plt.plot(x, json_stringify2, label="json_stringify2")
plt.plot(x, recordio, label="writer.write(encoder.encode(*json))")
#plt.plot(x, zipped, label="gzipped")
plt.legend()
plt.show()
