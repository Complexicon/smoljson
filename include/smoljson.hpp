#ifndef SMOLJSON_HPP
#define SMOLJSON_HPP

#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <variant>
#include <string>
#include <string_view>
#include <vector>
#include <unordered_map>
#include <memory>
#include <algorithm>
#include <type_traits>
#include <cmath>
#include <functional>
#include <array>

class smoljson {

	/// UTILITIES

	static constexpr size_t total_length() { return 0; }
	static inline size_t total_length(const std::string_view& s) { return s.size(); }

	template <typename... Args>
	static size_t total_length(const std::string_view& first, const Args&... args) { return first.size() + total_length(args...); }

	template <typename... Args>
	static std::string concat(const Args&... args) {
		std::string result;
		result.reserve(total_length(std::string_view(args)...));
		(result.append(args), ...);
		return result;
	}

	template <typename Container, typename UnaryFunction>
	static auto map_container(const Container& input, UnaryFunction func) {
		std::vector<decltype(func(*input.begin()))> result(input.size());
		std::transform(input.begin(), input.end(), result.begin(), func);
		return result;
	}

	static std::string join(const std::vector<std::string>& vec, const std::string& sep) {
		if (vec.empty()) return "";
		size_t prealloc = vec[0].size();

		for (size_t i = 1; i < vec.size(); i++) {
			prealloc += sep.size() + vec[i].size();
		}
		
		std::string result;
		result.reserve(prealloc);

		result.append(vec[0]);

		for (size_t i = 1; i < vec.size(); i++) {
			result.append(sep);
			result.append(vec[i]);
		}

		return result;
	}

	static constexpr std::array<const char*, 32> control_escapes = {
		"\\u0000", "\\u0001", "\\u0002", "\\u0003", "\\u0004", "\\u0005", "\\u0006", "\\u0007",
		"\\b",     "\\t",     "\\n",     "\\u000b", "\\f",     "\\r",     "\\u000e", "\\u000f",
		"\\u0010", "\\u0011", "\\u0012", "\\u0013", "\\u0014", "\\u0015", "\\u0016", "\\u0017",
		"\\u0018", "\\u0019", "\\u001a", "\\u001b", "\\u001c", "\\u001d", "\\u001e", "\\u001f"
	};

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
			case ARRAY: value = std::get<array_t>(other.value); break;
			case OBJECT: {
				value = object_t{};
				object_t& temp = std::get<object_t>(value);
				const object_t& other_obj = std::get<object_t>(other.value);
				temp.reserve(other_obj.size());
				for (const auto& [k, val_ptr] : other_obj) {
					temp.emplace(k, std::make_unique<smoljson>(*val_ptr));
				}
				break;
			}
		}
		return *this;
	}

	static smoljson array(std::initializer_list<smoljson> items) {
		smoljson j;
		j.type = ARRAY;
		j.value = array_t(items);
		return j;
	}

	static smoljson object(std::initializer_list<std::pair<std::string, smoljson>> items) {
		smoljson j;
		j.type = OBJECT;
		j.value = object_t{};
		object_t& temp = std::get<object_t>(j.value);
		temp.reserve(items.size());
		for (auto& [k, v] : items) {
			temp.emplace(k, std::make_unique<smoljson>(std::move(v)));
		}
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

				str.erase(str.find_last_not_of('0') + 1, std::string::npos);
				if (str.back() == '.') {
					str.pop_back();
				}

				return str;
			}
		};

		auto escaped_string = [&]() {
			const std::string& s = std::get<std::string>(value);
			std::string result;
			result.reserve(s.size()*2); // guess
			result += '"';
			for (unsigned char c : s) {
				if (c < 0x20) {
					result.append(control_escapes[c]);
					continue;
				}

				if (c == '\\' || c == '"') {
					result += '\\'; // add escaping backslash for \ and "
				}
				
				result += c;
			}
			result += '"';
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
				return concat("[", join(elements, ","), "]");
			}
			case OBJECT: {
					auto elements = map_container(
					std::get<object_t>(value), 
					[](const auto& entry) {
						auto& [key, value] = entry;
						return concat(smoljson(key).serialize(), ":", (*value).serialize());
					}
				);
				return concat("{", join(elements, ","), "}");
			}
		}

		return ""; // unreachable, but compiler complains
    }

	static smoljson parse(const std::string& json_literal) {
		// this function is a minor clusterfuck but faster
		// than my previous attempt using an actual
		// tokenizer and parser.
		size_t i = 0;

		auto parser_err = [&](const char* message) -> std::runtime_error {
			size_t offset = i - 20 > json_literal.size() ? 0 : i - 20;
			auto offending_json = json_literal.substr(offset, 40);
			offending_json.erase(std::remove(offending_json.begin(), offending_json.end(), '\n'), offending_json.end());
			offending_json.erase(std::remove(offending_json.begin(), offending_json.end(), '\r'), offending_json.end());
			return std::runtime_error(concat(
				message,
				" at position: ",
				std::to_string(i),
				" see here:\n",
				offending_json
			));
		};

		auto is_char = [&](char c) { return i < json_literal.size() && json_literal[i] == c; };

		auto skip_whitespace = [&]() {
			while (i < json_literal.size() && std::isspace(json_literal[i])) ++i;
		};

		auto parse_number = [&]() -> double {
			size_t start = i;
			auto consoom_numbers = [&] { while (i < json_literal.size() && std::isdigit(json_literal[i])) ++i; };
			
			if (is_char('-')) ++i;
			consoom_numbers();
			if (i < json_literal.size() && json_literal[i] == '.') {
				++i;
				consoom_numbers();
			}
			
			// scientific notation
			if (is_char('e') || is_char('E')) {
				++i;
				if (is_char('-') || is_char('+')) ++i;

				consoom_numbers();

				if (is_char('.')) {
					++i;
					consoom_numbers();
				}
			}

			std::string num = json_literal.substr(start, i - start);
			return std::stod(num); // let the exception bubble on invalid numbers
		};

		auto parse_string = [&]() -> std::string {
			++i; // skip the opening quote
			std::string result;
			result.reserve(100); // based on statistically accurate heuristic (i guessed)
			while (i < json_literal.size()) {
				char c = json_literal[i++];
				if (c == '"') {
					break;
				} else if (c == '\\') {
					if (i >= json_literal.size()) throw parser_err("Invalid escape sequence");
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
							if (i + 4 > json_literal.size()) throw parser_err("Invalid unicode escape");
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
							throw parser_err("Unknown escape character");
					}
				} else {
					result += c;
				}
			}
			return result;
		};

		std::function<smoljson()> parse_value = [&]() -> smoljson {
			skip_whitespace();
			if (i >= json_literal.size()) throw parser_err("Unexpected end of input");

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
				++i;
				skip_whitespace();
				smoljson instance = array({});
				array_t& arr = instance.as_vector();
				if (is_char(']')) {
					++i;
					return instance;
				}
				while (true) {
					arr.push_back(std::move(parse_value()));
					skip_whitespace();
					if (is_char(',')) { ++i;skip_whitespace(); continue; }
					if (is_char(']')) { ++i; break; }
					throw parser_err("Expected ',' or ']'");
				}
				return instance;
			}
			if (c == '{') {
				++i; skip_whitespace();
				smoljson obj = object({});
				if (is_char('}')) {
					++i;
					return obj;
				}
				while (true) {
					if (i < json_literal.size() && json_literal[i] != '"') throw parser_err("Expected string key");
					std::string key = parse_string();
					skip_whitespace();
					if (i < json_literal.size() && json_literal[i] != ':') throw parser_err("Expected ':'");
					++i; skip_whitespace();
					obj[key] = parse_value();
					skip_whitespace();
					if (is_char(',')) { ++i; skip_whitespace(); continue; }
					if (is_char('}')) { ++i; break; }
					throw parser_err("Expected ',' or '}'");
				}
				return obj;
			}

			throw parser_err("Unexpected character");
		};

		return parse_value();
	}

};

#endif