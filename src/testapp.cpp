#include <iostream>
#include "smoljson.hpp"

void test_basic_construction() {
    smoljson j_null;
    smoljson j_true = true;
    smoljson j_false = false;
    smoljson j_number = 3.1415;
    smoljson j_string = "hello world";

    std::cout << "Basic Types:\n";
    std::cout << "null: " << j_null.serialize() << "\n";
    std::cout << "true: " << j_true.serialize() << "\n";
    std::cout << "false: " << j_false.serialize() << "\n";
    std::cout << "number: " << j_number.serialize() << "\n";
    std::cout << "string: " << j_string.serialize() << "\n\n";
}

void test_array_and_object() {
    smoljson j_array = smoljson::array({ 1, 2, 3, "four" });
    smoljson j_obj = smoljson::object({
        {"a", 1},
        {"b", true},
        {"c", smoljson::array({ "x", "y", "z" })}
    });

    std::cout << "Array: " << j_array.serialize() << "\n";
    std::cout << "Object: " << j_obj.serialize() << "\n\n";
}

void test_index_access() {
    smoljson j;
	// shoutout to
    j["name"] = "ChatGPT";
	// for writing this basic testapp lol
    j["age"] = 2023;
    j["is_ai"] = true;
    j["languages"] = smoljson::array({ "C++", "Python", "English" });
    j["array"][5] = 42;

    std::cout << "Object with various fields: " << j.serialize() << "\n";

    std::cout << "\nAccess by index and key:\n";
    std::cout << "Name: " << j["name"].get<std::string>() << "\n";
    std::cout << "First language: " << j["languages"][0].get<std::string>() << "\n";
    std::cout << "empty_array[0]: " << j["empty_array"][0].get<int>() << "\n\n";
}

void test_copy_move() {
    smoljson original = smoljson::object({ {"key", "value"} });
    smoljson copy = original;
    smoljson moved = std::move(original);

    std::cout << "Copy: " << copy.serialize() << "\n";
    std::cout << "Moved: " << moved.serialize() << "\n\n";
}

void test_get_vs_strict_get() {
    smoljson j = 123;

    std::cout << "get<int> (should succeed): " << j.get<int>() << "\n";
    std::cout << "get<std::string> (should serialize): " << j.get<std::string>() << "\n";

    try {
        std::cout << "strict_get<std::string> (should throw): ";
        std::cout << j.strict_get<std::string>() << "\n";
    } catch (const std::exception& e) {
        std::cout << "Caught exception: " << e.what() << "\n";
    }

    std::cout << "\n";
}

void test_parsing() {
    std::string raw = R"({
        "msg": "hello",
        "value": 123,
        "array": [true, null, "text"],
        "object": { "nested": false }
    })";

    smoljson parsed = smoljson::parse(raw);
    std::cout << "Parsed: " << parsed.serialize() << "\n";
    std::cout << "Access nested object: " << parsed["object"]["nested"].get<bool>() << "\n\n";
}

void test_edge_cases() {
    smoljson j;
    try {
        std::cout << "Accessing non-existent key (const): ";
        std::cout << j["missing"].strict_get<int>() << "\n";
    } catch (const std::exception& e) {
        std::cout << "Caught exception: " << e.what() << "\n";
    }

    try {
        smoljson arr = smoljson::array({1, 2});
        std::cout << "Out-of-bounds array access: ";
        std::cout << arr[5].get<int>() << "\n";
    } catch (const std::exception& e) {
        std::cout << "Caught exception: " << e.what() << "\n";
    }

    try {
        smoljson invalid = smoljson::parse("{ invalid json ");
    } catch (const std::exception& e) {
        std::cout << "Invalid JSON parse error: " << e.what() << "\n";
    }

    std::cout << "\n";
}

int main() {
    test_basic_construction();
    test_array_and_object();
    test_index_access();
    test_copy_move();
    test_get_vs_strict_get();
    test_parsing();
    test_edge_cases();

    std::cout << "All tests complete.\n";
    return 0;
}
