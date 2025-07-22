#include "smoljson.hpp"
#include <chrono>
#include <iostream>
#include <functional>
#include <fstream>
#include <string>

static std::string read_file_to_string(const std::string& filepath) {
    std::ifstream file(filepath, std::ios::in | std::ios::binary);

    if (!file) {
        throw std::runtime_error("Failed to open file: " + filepath);
    }

    // Seek to end to get size
    file.seekg(0, std::ios::end);
    size_t size = file.tellg();
    std::string content(size, '\0'); // Allocate string with required size

    // Seek back and read
    file.seekg(0);
    file.read(&content[0], size);

    return content;
}

template <typename T>
inline static T benchmark(const char* name, std::function<T()> func) {
	auto start = std::chrono::high_resolution_clock::now();

    auto result = func();

    auto end = std::chrono::high_resolution_clock::now();

    std::chrono::duration<double, std::milli> duration = end - start;

    std::cout << name << " took " << duration.count() << " ms\n";

    return result;
}

// these two are nicely instrumentable
// as seperate functions for flamegraphing, profiling, etc

inline static smoljson parse(const std::string& data) {
    return benchmark<smoljson>("parsing", [&]() {
        return smoljson::parse(data);
    });
}

inline static std::string serialize(const smoljson& d) {
    return benchmark<std::string>("serializing", [&]() {
        return d.serialize();
    });
}

int main() {
    std::string dummy_data = read_file_to_string("..\\benchmark.json");
    std::cout << "\n";
    try {
        std::ofstream result("test.json");
        result << serialize(parse(dummy_data));
    }
    catch (std::exception e) {
        std::cout << e.what();
    }
	return 0;
}