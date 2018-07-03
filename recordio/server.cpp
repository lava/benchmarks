// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License

#include <iostream>
#include <sstream>
#include <vector>
#include <map>
#include <unordered_set>
#include <random>

#include <process/defer.hpp>
#include <process/dispatch.hpp>
#include <process/future.hpp>
#include <process/http.hpp>
#include <process/process.hpp>

#include <stout/json.hpp>
#include <stout/uuid.hpp>
#include <stout/recordio.hpp>

using namespace process;
using namespace process::http;

using std::string;

enum class CachingBehaviour {
  CLEAR_THEN_DEFAULT,
  FORCE_CACHE,
  DEFAULT,
  NO_CACHE,
};

std::default_random_engine rng;

JSON::Object* make_node()
{
	JSON::Object* node = new JSON::Object();
	node->values["id"] = id::UUID::random().toString();
	node->values["children"] = JSON::Array();
	return node;
}


/* Generate random tree with n nodes. Based on Algorithm W in TAoC 4A, Chapter 7.2.1.6 */
// ( (()) () )
JSON::Object* randomJson(int n, CachingBehaviour behaviour = CachingBehaviour::DEFAULT)
{
	static std::map<int, JSON::Object*> cache;

	if (behaviour == CachingBehaviour::CLEAR_THEN_DEFAULT) {
		for (auto& kv : cache) {
			delete kv.second;
		}
		cache.clear();
		behaviour = CachingBehaviour::DEFAULT;
	}

	if (behaviour == CachingBehaviour::FORCE_CACHE ||
	    behaviour == CachingBehaviour::DEFAULT) {
		auto it = cache.find(n);
		if (it != cache.end())
			return it->second;

		if (behaviour == CachingBehaviour::FORCE_CACHE)
			return nullptr;
	}

	JSON::Object* root = make_node();
	std::vector<JSON::Object*> up;

	JSON::Object* current = root;
	int p=n-1, q=n-1; // we already have our root node, so we only need n-1 further nodes
	while(q != 0) {
		std::uniform_int_distribution<int> dist(0,(q+p)*(q-p+1)-1);
		int x = dist(rng);

		if(x < (q+1)*(q-p)) {
			// up a level
			--q;
			current = up.back();
			up.pop_back();
		} else {
			// down a level
			--p;
			JSON::Value& value = current->values["children"];
			JSON::Array* array = boost::get<JSON::Array>(&value);

			JSON::Object* child = make_node();
			// note: this copies child
			array->values.push_back(*child);
			delete child;

			up.push_back(current);
			current = boost::get<JSON::Object>(&array->values.back());
		}
	}

	// We should have current == root again
	cache[n] = root;

	return root;
}


JSON::Object* randomState(int n, CachingBehaviour behaviour = CachingBehaviour::DEFAULT)
{
	
}


class MyProcess : public Process<MyProcess>
{
public:
  MyProcess():
    ProcessBase("my_process"),
    encoder([](const JSON::Object& json) -> std::string {
      return stringify(json); // 
    })
  {}

  virtual ~MyProcess() {}

  Future<Response> prepareCache(const Request& request)
  {
    std::string requestString = request.url.query.get("size").getOrElse(std::string("5000"));
    //long long size = atoll(request.url.query.get("size").getOrElse(std::string("5000")).c_str());
    long long size = atoll(requestString.c_str());
    // Might be worth investigating:
    //long long depth = atoll(request.url.query.get("depth").getOrElse(std::string("3")).c_str());

    http::OK ok(*randomJson(size, CachingBehaviour::CLEAR_THEN_DEFAULT));
    ok.headers["Access-Control-Allow-Origin"] = "*";
    return ok;
  }

  Future<Response> jsonCached(const Request& request)
  {
    long long size = atoll(request.url.query.get("size").getOrElse(std::string("5000")).c_str());
    // Might be worth investigating:
    //long long depth = atoll(request.url.query.get("depth").getOrElse(std::string("3")).c_str());

    JSON::Object* json = randomJson(size, CachingBehaviour::FORCE_CACHE);
    if (!json) {
      return http::BadRequest();
    }
    
    http::OK ok(*json);
    ok.headers["Access-Control-Allow-Origin"] = "*";
    return ok;
  }

  Future<Response> jsonCachedStringify(const Request& request)
  {
    long long size = atoll(request.url.query.get("size").getOrElse(std::string("5000")).c_str());

    JSON::Object* json = randomJson(size, CachingBehaviour::FORCE_CACHE);
    if (!json) {
      return http::BadRequest();
    }
    
    http::OK ok(stringify(*json));
    ok.headers["Access-Control-Allow-Origin"] = "*";
    return ok;
  }

  Future<Response> recordioCached(const Request& request)
  {
    long long size = atoll(request.url.query.get("size").getOrElse(std::string("5000")).c_str());

    JSON::Object* json = randomJson(size);
    http::Pipe pipe;
    http::OK ok;

    ok.headers["Access-Control-Allow-Origin"] = "*";
    ok.headers["Content-Type"] = "application/recordio";
    ok.type = http::Response::PIPE;
    ok.reader = pipe.reader();

    http::Pipe::Writer writer = pipe.writer();
    writer.write(encoder.encode(*json));
    writer.close();

    return ok;
  }


  // This endpoint uses a huge hashmap<int, Protobuf> as data source, which
  // should be a good model of the actual state since the fields
  // containing >90% of the data are stored in such structures on the master
  Future<Response> hashmapCached(const Request& request)
  {

  }

protected:
  virtual void initialize()
  {
    route("/prepare-json-cache", "Generate json.", &MyProcess::prepareCache);
    route("/json-cached", "Serialize existing json file.", &MyProcess::jsonCached);
    route("/json-cached-stringify", "Serialize existing json file.", &MyProcess::jsonCachedStringify);
    route("/recordio-cached", "Generate recordio.", &MyProcess::recordioCached);
    route("/hashmap-cached", "Generate recordio.", &MyProcess::hashmapCached);
  }

private:
    ::recordio::Encoder<JSON::Object> encoder;
};


int main(int argc, char** argv)
{
  MyProcess process;
  PID<MyProcess> pid = spawn(&process);
  wait(pid);
  return 0;
}

