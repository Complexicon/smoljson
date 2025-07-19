# smoljson

**`smoljson`** is a small, opinionated, single-header C++17 JSON library. It's roughly 500 lines of code, self-contained, and designed for developers who want basic JSON manipulation without large libraries like the amazing `nlohmann::json`.

* Header-only
* C++17
* No external dependencies
* Supports parsing, serialization, and convenient access
* Mutable object/array support via `operator[]`

## ✨ Features (or lack there of)

* JSON parsing and serialization
* Object and array creation from initializer_lists
* Type conversion and retrieval (`get<T>()`, `strict_get<T>()`)
* Minimal but ergonomic API
* Safe and strict access patterns
* Omission of nice-to-have features (for example pretty-printing)
* Usage of an ✨"on the fly recursive descent parser"✨ (basically skips tokenization)

---

## 🔧 Installation

Just drop [`smoljson.hpp`](./include/smoljson.hpp) into your project.

```cpp
#include "smoljson.hpp"
```

---

## 📦 Usage Examples

### Constructing JSON

```cpp
smoljson j = smoljson::object({
    {"name", "SmolJSON"},
    {"header_only", true},
    {"github_stars", 0},
    {"tags", smoljson::array({"c++17", "basic", "json"})}
});

std::cout << j.serialize();  // {"name":"SmolJSON","header_only":true,"github_stars":0,"tags":["c++17", "basic", "json"]}
```

### Accessing and Modifying JSON

```cpp
std::string name = j["name"].get<std::string>();
bool active = j["active"].get<bool>();

j["score"] = 99.0;
j["new_field"] = nullptr;
```

### Iterating over an object
```cpp

smoljson obj = smoljson::object({
	{"name", "Alice"},
	{"age", 30},
	{"is_student", false}
});

std::cout << "Object contents:\n";
for (const auto& [key, value_ptr] : obj.as_map()) {
	std::cout << key << ": " << value_ptr->serialize() << "\n";
}
```

### Iterating over an array
```cpp
smoljson arr = smoljson::array({
	"apple",
	42,
	true
});

std::cout << "Array contents:\n";
for (const auto& item : arr.as_vector()) {
	std::cout << item.serialize() << "\n";
}
```

### Array Access

```cpp
smoljson arr = smoljson::array({1, 2, 3});
arr[5] = 42;  // Automatically resizes

for (size_t i = 0; i < arr.size(); ++i) {
    std::cout << arr[i].get<int>() << "\n";
}
```

### Parsing JSON Strings

```cpp
std::string raw = R"({"hello":"world","value":123})";
smoljson parsed = smoljson::parse(raw);

std::cout << parsed["hello"].get<std::string>();  // world
```

---

## 📘 API Reference

### Constructors

```cpp
smoljson();                         // null
smoljson(nullptr_t);               // null
smoljson(const std::string&);      // string
smoljson(const char*);             // string
smoljson(bool);                    // boolean
smoljson(double / int / float);    // number
```

### Static Factory Methods

```cpp
smoljson::array({elem1, elem2, ...});              // Create JSON array
smoljson::object({{"key", value}, ...});           // Create JSON object
smoljson::parse(json_string);                      // Parse JSON string
smoljson::null();                                  // Null singleton
```

### Accessors

```cpp
j["key"]           // Get or create object field
j[index]           // Get or resize array element
// note: keep in mind that accessors will throw on an out of bounds access in a const context
```

### Type Retrieval

```cpp
j.get<T>()         // Flexible conversion (e.g., "123" -> int)
j.strict_get<T>()  // Type-safe strict access
```

### Type Checks

```cpp
j.is_array()
j.is_object()
j.is_null()
```

### Helpers

```cpp
j.size()                   // For arrays only
j.as_vector()              // std::vector<smoljson>&
j.as_map()                 // std::unordered_map<std::string, std::unique_ptr<smoljson>>&
j.serialize()              // Serialize to JSON string
```

---

## 🧠 Design Philosophy

This library is **opinionated**:

* Nulls, numbers, strings, booleans, arrays, and objects only — no support for custom types.
* `get<T>()` offers duck-typed conversions (e.g., `"true"` → `true`), but `strict_get<T>()` enforces correctness.
* Arrays auto-resize on `operator[]`, objects auto-create fields.

Perfect for scripting engines, config files, or small projects.

---

## ⚠️ Limitations

* No schema validation
* No comments or trailing commas in JSON
* UTF-16 escape sequences (`\uXXXX`) only partially supported
* Not optimized for performance-critical scenarios
* Thread-safety is not guaranteed due to possible mutations on access

---

## 📄 License

**MIT** — free to use, modify, and distribute.