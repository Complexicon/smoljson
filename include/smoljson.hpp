#ifndef SMOLJSON_HPP
#define SMOLJSON_HPP

#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <variant>
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <algorithm>
#include <type_traits>
#include <numeric>
#include <cmath>
#include <functional>

class smoljson {

	/// UTILITIES
	template <typename Container, typename UnaryFunction>
	static auto map_container(const Container& input, UnaryFunction func) {
		std::vector<decltype(func(*input.begin()))> result;
		result.reserve(input.size());
		std::transform(input.begin(), input.end(), std::back_inserter(result), func);
		return result;
	}

	static std::string join(const std::vector<std::string>& vec, const std::string& sep) {
		return vec.empty() ? "" :std::accumulate(std::next(vec.begin()), vec.end(), vec[0],
			[&sep](const std::string& a, const std::string& b) { return a + sep + b; }
		);
	}

	// hack to get template instatiation that failed
	template<typename> static inline constexpr bool always_false_v = false;

	using object_t = std::unordered_map<std::string, std::unique_ptr<smoljson>>;
	using array_t = std::vector<smoljson>;

	// DATA MEMBERS

	enum json_type {
		NULL_TYPE,
		STRING,
		NUMBER,
		BOOLEAN,
		ARRAY,
		OBJECT
	} type;

	std::variant<
		std::monostate,
		std::string,
		double,
		bool,
		array_t,
		object_t
	> value;

public:

	/// CONSTRUCTORS	

	smoljson() : type(NULL_TYPE), value(std::monostate{}) {}
	smoljson(std::nullptr_t) : type(NULL_TYPE), value(std::monostate{}) {}
	smoljson(const std::string& s) : type(STRING), value(s) {}
	smoljson(const char* s) : smoljson(std::string(s)) {}
	smoljson(bool b) : type(BOOLEAN), value(b) {}

	template<typename T, typename = std::enable_if_t<std::is_arithmetic_v<T>>>
	smoljson(T num) : type(NUMBER), value(static_cast<double>(num)) {}

	smoljson(const smoljson& other) { *this = other; }
	smoljson(smoljson&&) noexcept = default;

	smoljson& operator=(const smoljson& other) {
		if (this == &other) return *this;

		type = other.type;

		switch (type) {
			case NULL_TYPE: value = std::monostate{}; break;
			case STRING: value = std::get<std::string>(other.value); break;
			case NUMBER: value = std::get<double>(other.value); break;
			case BOOLEAN: value = std::get<bool>(other.value); break;
			case ARRAY: value = std::get<std::vector<smoljson>>(other.value); break;
			case OBJECT: {
				object_t temp;
				for (const auto& [k, val_ptr] : std::get<object_t>(other.value)) {
					temp.emplace(k, std::make_unique<smoljson>(*val_ptr));
				}
				value = std::move(temp);
				break;
			}
		}
		return *this;
	}

	static smoljson array(std::initializer_list<smoljson> items) {
		smoljson j;
		j.type = ARRAY;
		j.value = std::vector<smoljson>(items);
		return j;
	}

	static smoljson object(std::initializer_list<std::pair<std::string, smoljson>> items) {
		smoljson j;
		j.type = OBJECT;
		std::unordered_map<std::string, std::unique_ptr<smoljson>> temp;
		for (const auto& [k, v] : items) {
			temp.emplace(k, std::make_unique<smoljson>(v));
		}
		j.value = std::move(temp);
		return j;
	}

	static const smoljson& null() {
		static const smoljson null;
		return null;
	}

	/// ASSIGNMENT

	smoljson& operator=(std::nullptr_t) {
		type = NULL_TYPE;
		value = std::monostate{};
		return *this;
	}

	smoljson& operator=(const std::string& s) {
		type = STRING;
		value = s;
		return *this;
	}

	smoljson& operator=(const char* s) {
		return operator=(std::string(s));
	}

	smoljson& operator=(bool b) {
		type = BOOLEAN;
		value = b;
		return *this;
	}

	template<typename T, typename = std::enable_if_t<std::is_arithmetic_v<T>>>
	smoljson& operator=(T num) {
		type = NUMBER;
		value = static_cast<double>(num);
		return *this;
	}

	/// ACCESSORS

	smoljson& operator[](const std::string& key) {
	if (type != OBJECT) {
			type = OBJECT;
			value = object_t{};
		}

		auto& map = std::get<object_t>(value);
		auto it = map.find(key);

		if (it == map.end()) {
			it = map.emplace(key, std::make_unique<smoljson>()).first;
		}

		return *(it->second);
	}
	
	const smoljson& operator[](const std::string& key) const {
		if (type != OBJECT) {
			throw std::runtime_error("Attempted to access non-object as object");
		}
		const auto& map = std::get<object_t>(value);
		auto it = map.find(key);
		if (it == map.end()) {
			throw std::out_of_range("Key not found in object");
		}
		return *(it->second);
	}

	smoljson& operator[](size_t index) {
		if (type != ARRAY) {
			type = ARRAY;
			value = array_t{};
		}

		auto& arr = std::get<array_t>(value);

		if (index >= arr.size()) {
			arr.resize(index + 1);
		}

		return arr[index];
	}
	
	const smoljson& operator[](size_t index) const {
		if (type != ARRAY) {
			throw std::runtime_error("Attempted to access non-array as array");
		}

		auto& arr = std::get<array_t>(value);

		if (index >= arr.size()) {
			throw std::out_of_range("index out of bounds");
		}

		return arr[index];
	}

	template<typename T>
	T get() const {
		if constexpr (std::is_same_v<T, bool>) {
			switch (type) {
				case BOOLEAN: return std::get<bool>(value);
				case NUMBER: return std::get<double>(value) != 0.0;
				case NULL_TYPE: return false;
				case STRING: {
					auto& str = std::get<std::string>(value);
					return !str.empty() && str != "false" && str != "0";
				}
				default: return true; // array/object: treat as truthy
			}
		}
		else if constexpr (std::is_arithmetic_v<T>) {
			switch (type) {
				case BOOLEAN: return static_cast<T>(std::get<bool>(value) ? 1 : 0);
				case NUMBER: return static_cast<T>(std::get<double>(value));
				case STRING: {
					try {
						return static_cast<T>(std::stod(std::get<std::string>(value)));
					} catch (...) {
						return static_cast<T>(0); // or throw, based on preference
					}
				}
				default: return static_cast<T>(0);
			}
		}
		else if constexpr (std::is_same_v<T, std::string>) {
			switch (type) {
				case STRING: return std::get<std::string>(value);
				default: return serialize();
			}
		}
		else {
			static_assert(always_false_v<T>, "get<T>() is not implemented for this type");
		}
	}

	template<typename T>
	T strict_get() const {
		if constexpr (std::is_same_v<T, bool>) {
			if (type != BOOLEAN)
				throw std::runtime_error("Attempted to access non-boolean as boolean");
			return std::get<bool>(value);
		} else if constexpr (std::is_arithmetic_v<T>) {
			if (type != NUMBER)
				throw std::runtime_error("Attempted to access non-number as number");
			return static_cast<T>(std::get<double>(value));
		} else if constexpr (std::is_same_v<T, std::string>) {
			if (type != STRING)
				throw std::runtime_error("Attempted to access non-string as string");
			return std::get<std::string>(value);
		} else {
			static_assert(always_false_v<T>, "get<T>() is not implemented for this type");
		}
	}

	/// HELPERS

	bool is_array() const { return type == ARRAY; }
	bool is_object() const { return type == OBJECT; }
	bool is_null() const { return type == NULL_TYPE; }
	size_t size() const { return is_array() ? std::get<array_t>(value).size() : 0; }

	// throws if not an array!
	array_t& as_vector() { return std::get<array_t>(value); } 
	const array_t& as_vector() const { return std::get<array_t>(value); } 

	// throws if not an object!
	object_t& as_map() { return std::get<object_t>(value); } 
	const object_t& as_map() const { return std::get<object_t>(value); } 

	/// SERIALIZATION
	
	std::string serialize() const {
		static const std::string null_str = "null";
		static const std::string true_str = "true";
		static const std::string false_str = "false";

		auto num_to_string = [&]() {
			const double& dbl = std::get<double>(value);
			if (std::floor(dbl) == dbl) { // value is whole integer
				return std::to_string(static_cast<long long>(dbl));
			} else {
				std::ostringstream oss;
				oss << std::setprecision(15) << dbl;
				std::string str = oss.str();

				if (str.find('.') != std::string::npos) {
					str.erase(str.find_last_not_of('0') + 1);
					if (str.back() == '.') {
						str.pop_back();
					}
				}

				return str;
			}
		};

		auto escaped_string = [&]() {
			const std::string& s = std::get<std::string>(value);
			std::string result = "\"";
			for (char c : s) {
				switch (c) {
					case '\"': result += "\\\""; break;
					case '\\': result += "\\\\"; break;
					case '\b': result += "\\b"; break;
					case '\f': result += "\\f"; break;
					case '\n': result += "\\n"; break;
					case '\r': result += "\\r"; break;
					case '\t': result += "\\t"; break;
					default:
						if (static_cast<unsigned char>(c) < 0x20) {
							// escape control characters -> \u00XX
							char buf[7];
							snprintf(buf, sizeof(buf), "\\u%04x", c & 0xFF);
							result += buf;
						} else {
							result += c;
						}
				}
			}
			result += "\"";
			return result;
		};

		switch (type) {
			case NULL_TYPE: return null_str;
			case STRING: return escaped_string();
			case NUMBER: return num_to_string();
			case BOOLEAN: return std::get<bool>(value) ? true_str : false_str;
			case ARRAY: {
				auto elements = map_container(
					std::get<array_t>(value),
					[](const smoljson& json) { return json.serialize(); }
				);
				return std::string("[") + join(elements, ",") + "]";
			}
			case OBJECT: {
					auto elements = map_container(
					std::get<object_t>(value), 
					[](const auto& entry) {
						auto& [key, value] = entry;
						return smoljson(key).serialize() + ":" + (*value).serialize();
					}
				);
				return std::string("{") + join(elements, ",") + "}";
			}
		}

		return ""; // unreachable, but compiler complains
    }

	static smoljson parse(const std::string& json_literal) {
		// this function is a minor clusterfuck but faster
		// than my previous attempt using an actual
		// tokenizer and parser.
		size_t i = 0;

		auto skip_whitespace = [&]() {
			while (i < json_literal.size() && std::isspace(json_literal[i])) ++i;
		};

		auto parse_number = [&]() -> double {
			size_t start = i;
			if (json_literal[i] == '-') ++i;
			while (i < json_literal.size() && std::isdigit(json_literal[i])) ++i;
			if (i < json_literal.size() && json_literal[i] == '.') {
				++i;
				while (i < json_literal.size() && std::isdigit(json_literal[i])) ++i;
			}
			std::string num = json_literal.substr(start, i - start);
			return std::stod(num); // let the exception bubble on invalid numbers
		};

		auto parse_string = [&]() -> std::string {
			if (json_literal[i] != '"') {
				throw std::runtime_error("Expected opening quote for string");
			}
			++i; // skip the opening quote
			std::string result;
			while (i < json_literal.size()) {
				char c = json_literal[i++];
				if (c == '"') {
					break;
				} else if (c == '\\') {
					if (i >= json_literal.size()) throw std::runtime_error("Invalid escape sequence");
					char esc = json_literal[i++];
					switch (esc) {
						case '"': result += '"'; break;
						case '\\': result += '\\'; break;
						case '/': result += '/'; break;
						case 'b': result += '\b'; break;
						case 'f': result += '\f'; break;
						case 'n': result += '\n'; break;
						case 'r': result += '\r'; break;
						case 't': result += '\t'; break;
						case 'u': {
							if (i + 4 > json_literal.size()) throw std::runtime_error("Invalid unicode escape");
							std::string hex = json_literal.substr(i, 4);
							i += 4;
							char16_t unicode_char = static_cast<char16_t>(std::stoi(hex, nullptr, 16));
							// note: basic implementation; UTF-16 to UTF-8 conversion not fully handled
							if (unicode_char < 0x80) {
								result += static_cast<char>(unicode_char);
							} else {
								result += '?';
							}
							break;
						}
						default:
							throw std::runtime_error(std::string("Unknown escape character: \\") + esc);
					}
				} else {
					result += c;
				}
			}
			return result;
		};

		std::function<smoljson()> parse_value = [&]() -> smoljson {
			skip_whitespace();
			if (i >= json_literal.size()) throw std::runtime_error("Unexpected end of input");

			char c = json_literal[i];
			if (c == '"') return smoljson(parse_string());
			if (std::isdigit(c) || c == '-') return smoljson(parse_number());

			if (c == 't' && json_literal.substr(i, 4) == "true") {
				i += 4; return smoljson(true);
			}
			if (c == 'f' && json_literal.substr(i, 5) == "false") {
				i += 5; return smoljson(false);
			}
			if (c == 'n' && json_literal.substr(i, 4) == "null") {
				i += 4; return smoljson(nullptr);
			}
			if (c == '[') {
				++i; skip_whitespace();
				smoljson instance = array({});
				array_t& arr = instance.as_vector();
				if (i < json_literal.size() && json_literal[i] == ']') {
					++i;
					return instance;
				}
				while (true) {
					arr.push_back(parse_value());
					skip_whitespace();
					if (i < json_literal.size() && json_literal[i] == ',') { ++i; skip_whitespace(); continue; }
					if (i < json_literal.size() && json_literal[i] == ']') { ++i; break; }
					throw std::runtime_error("Expected ',' or ']'");
				}
				return instance;
			}
			if (c == '{') {
				++i; skip_whitespace();
				smoljson instance = object({});
				object_t& obj = instance.as_map();
				if (i < json_literal.size() && json_literal[i] == '}') {
					++i;
					return instance;
				}
				while (true) {
					if (i < json_literal.size() && json_literal[i] != '"') throw std::runtime_error("Expected string key");
					std::string key = parse_string();
					skip_whitespace();
					if (i < json_literal.size() && json_literal[i] != ':') throw std::runtime_error("Expected ':'");
					++i; skip_whitespace();
					obj[key] = std::make_unique<smoljson>(parse_value());
					skip_whitespace();
					if (i < json_literal.size() && json_literal[i] == ',') { ++i; skip_whitespace(); continue; }
					if (i < json_literal.size() && json_literal[i] == '}') { ++i; break; }
					throw std::runtime_error("Expected ',' or '}'");
				}
				return instance;
			}

			throw std::runtime_error(std::string("Unexpected character: ") + c);
		};

		return parse_value();
	}

};

#endif