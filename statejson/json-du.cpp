#include <fstream>

#define PICOJSON_USE_INT64
#include <stout/json.hpp>

#define ensure(cond) if (!(cond)) { std::cout << "Expected: " << #cond << std::endl; return 1; }


std::string format_bytes(size_t n) {
	// todo - switch based on -h option
	return stringify(n);


	if (n < 1024) {
		return stringify(n) + " bytes";
	}

	n /= 1024;

	if (n < 1024) {
		return stringify(n) + " KiB";
	}

	n /= 1024;

	if (n < 1024) {
		return stringify(n) + " MiB";
	}

	n /=1024;

	return stringify(n) + " GiB";
}

void print(const JSON::Object& object, int max_depth, int depth = 1, std::string prefix = "")
{
	for (auto& kv : object.values) {
		const JSON::Value& value = kv.second;

		std::string name = prefix + "/" + kv.first;
		std::string content = jsonify(value);
		std::cout << name << "\t" << format_bytes(content.size()) << "\n";

		if (depth < max_depth) {
			if (value.is<JSON::Object>()) {
				print(value.as<JSON::Object>(), max_depth, depth+1, name);
			} else if (value.is<JSON::Array>()) {
				const JSON::Array& arr = value.as<JSON::Array>();
				for (int i=0; i<arr.values.size(); ++i) {
					if (arr.values[i].is<JSON::Object>()) {
						print(arr.values[i].as<JSON::Object>(), max_depth, depth+1, name + "[" + stringify(i) + "]");
					}
				}
			}
		}
	}

}

int main(int argc, char* argv[]) {

	ensure(argc > 1);	

	int depth = 1;

	const char* filename = NULL;
	for (int i=1; i<argc; ++i) {
		if (argv[i][0] != '-') {
			filename = argv[i];
		} else if (!strcmp(argv[i], "--depth")) {
			ensure(i < argc-1);
			depth = atoi(argv[i+1]);
			++i; // skip number
		} else {
			ensure(false && "unknown arg");
		}
	}

	ensure(filename);

	std::ifstream f(filename);
	std::string content;

	f.seekg(0, std::ios::end);
	content.reserve(f.tellg());
	std::cout << "/ " << f.tellg() << std::endl;
	f.seekg(0, std::ios::beg);


	content.assign((std::istreambuf_iterator<char>(f)),
	                std::istreambuf_iterator<char>());

	Try<JSON::Value> trv = JSON::parse(content);

	ensure(!trv.isError());

	print(trv.get().as<JSON::Object>(), depth);
}
