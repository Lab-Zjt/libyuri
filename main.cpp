#include <iostream>
#include <variant>
#include "yuri.h"

using namespace std;

template<typename ...Ts, size_t max_len = 1024>
string naive_sprintf(const char *fmt, Ts ... ts) {
  string s;
  s.reserve(max_len);
  snprintf(s.data(), max_len, fmt, ts...);
  return s;
}

struct Object {
 using_reflect(Object);
 reflect_field(int, i, = 2);
 reflect_field(double, d);
};

struct O2 {
 using_reflect(O2);
 reflect_field(int, w);
 reflect_field(Object, obj);
 reflect_field(string, str);
};

/*struct D1 : Object {
 using_reflect_inherit(D1, Object);
};*/

struct Derived : Object, O2 {
 using_reflect_inherit(Derived, Object, O2);
 reflect_field(std::string, msg, { "hello,world" });
 reflect_field(std::vector<Object>, vec, {{ 3, 4.5 }, { 5, 7.5 }, { 7, 10.5 }});
 reflect_field(with_comma(unordered_map<string, Object>), o, {
   { "123", Object{.i = 17, .d = 23.24}},
   { "456", Object{.i = 24, .d = 34.56}}
 });
};

template<typename ...Ts>
struct overloaded : public Ts ... {
  using Ts::operator()...;
};

template<typename ...Ts>
overloaded(Ts...) -> overloaded<Ts...>;

struct TokenBase { std::string str; };

struct LBracket : TokenBase {};
struct RBracket : TokenBase {};

struct LCurly : TokenBase {};
struct RCurly : TokenBase {};

struct String : TokenBase {};

struct Colon : TokenBase {};
struct Comma : TokenBase {};

struct End : TokenBase {};

using Token = variant<LBracket, RBracket, LCurly, RCurly, String, Colon, Comma, End>;

class JsonParser {
  const string &str;
  size_t off = 0;
  bool is_stop = false;
 public:
  JsonParser(const string &s) : str(s) {}
  Token GetNextToken() {
    while (isspace(str[off]) && off < str.size())off++;
    if (off >= str.size())return End{};
    size_t begin = off, end;

    switch (str[off]) {
      case '{': {
        end = ++off;
        return LCurly{str.substr(begin, end - begin)};
      }
      case '}': {
        end = ++off;
        return RCurly{str.substr(begin, end - begin)};
      }
      case ':': {
        end = ++off;
        return Colon{str.substr(begin, end - begin)};
      }
      case '[': {
        end = ++off;
        return LBracket{str.substr(begin, end - begin)};
      }
      case ']': {
        end = ++off;
        return RBracket{str.substr(begin, end - begin)};
      }
      case ',': {
        end = ++off;
        return Comma{str.substr(begin, end - begin)};
      }
      case '"': {
        while (str[off] != '"' && str[off] != '\\' && off < str.size())off++;
        if (off == str.size() && str[off] != '"')throw std::runtime_error("invalid json");
        end = ++off;
        return String{str.substr(begin, end - begin)};
      }
      default: {
        while (!isspace(str[off]) && str[off] != ',')off++;
        end = off;
        return String{str.substr(begin, end - begin)};
      }
    }
  }
  void ParseSpace() {

  }
  template<typename T>
  T ParseNumber() {}
  template<typename T>
  T ParseString() {}
  template<typename T>
  T ParseBool() {}
  template<typename T>
  T ParseArray() {}
  template<typename T>
  T ParseObject() {
    Object obj;
    auto key = ParseString<string>();

  }
};

using parse_func = std::function<bool(const std::string &, void *)>;

inline static unordered_map<reflect::TypeID, parse_func> parser;

string trim(const std::string &str) {
  auto pos1 = str.find_first_not_of("\r\n\t\v\f ");
  auto pos2 = str.find_last_not_of("\r\n\t\v\f ");
  return str.substr(pos1, pos2 - pos1 + 1);
}

inline static bool parse(reflect::TypeID id, const std::string &str, void *ptr) {
  if (auto it = parser.find(id); it != parser.end()) {
    return it->second(trim(str), ptr);
  } else {
    return false;
  }
}

template<typename T>
parse_func get_parse_func() {
  if constexpr (std::is_same_v<T, bool>) {
    return [](const std::string &str, void *ptr) -> bool {
      if (str == "true") {
        *static_cast<bool *>(ptr) = true;
      } else if (str == "false") {
        *static_cast<bool *>(ptr) = false;
      } else {
        return false;
      }
      return true;
    };
  } else if constexpr (std::is_integral_v<T>) {
    return [](const std::string &str, void *ptr) -> bool {
      auto val = strtoll(str.c_str(), nullptr, 10);
      if (errno == ERANGE)return false;
      *static_cast<T *>(ptr) = static_cast<T>(val);
      return true;
    };
  } else if constexpr(std::is_floating_point_v<T>) {
    return [](const std::string &str, void *ptr) -> bool {
      auto val = strtof64(str.c_str(), nullptr);
      if (errno == ERANGE) return false;
      *static_cast<T *>(ptr) = static_cast<T>(val);
      return true;
    };
  } else if constexpr(std::is_same_v<T, std::string>) {
    return [](const std::string &str, void *ptr) -> bool {
      if (str.size() < 2) return false;
      if (str[0] == '"' && str[str.size() - 1] == '"') {
        *static_cast<T *>(ptr) = str.substr(1, str.size() - 2);
        return true;
      }
      return false;
    };
  } else if constexpr (experimental::is_detected_v<reflect::has_begin_t, T>
      && experimental::is_detected_v<reflect::has_end_t, T>) {
    return [](const std::string &str, void *ptr) -> bool {
      if (str.front() != '[' || str.back() != ']')return false;
      auto &vec = *static_cast<T *>(ptr);
      using vtype = typename T::value_type;
      vtype v;
      size_t pos0 = 0, pos1;
      while (true) {
        pos1 = str.find(',', pos0);
        if (!parse(reflect::type_id<vtype>, str.substr(pos0, pos1 - pos0), &v))return false;
        vec.insert(vec.end(), v);
        if (pos1 == string::npos)break;
        pos0 = pos1 + 1;
      }
      return true;
    };
  } else if constexpr (experimental::is_detected_v<reflect::has_first_t, T>
      && experimental::is_detected_v<reflect::has_second_t, T>) {
    return [](const std::string &str, void *ptr) -> bool {
      if (str.front() != '{' || str.back() != '}')return false;
      auto &p = *static_cast<std::pair<typename T::first_type, typename T::second_type> *>(ptr);
      size_t pos0 = 1, pos1;
      pos1 = str.find(':', pos0);
      string key;
      if (!parse(reflect::type_id<string>, str.substr(pos0, pos1 - pos0), &key))return false;
      pos0 = pos1 + 1;
      pos1 = str.find(',', pos0);
      if (!parse(reflect::type_id<typename T::first_type>, str.substr(pos0, pos1 - pos0), &p.first))return false;
      pos0 = pos1 + 1;
      pos1 = str.find(':', pos0);
      if (!parse(reflect::type_id<string>, str.substr(pos0, pos1 - pos0), &key))return false;
      pos0 = pos1 + 1;
      pos1 = str.find('}', pos0);
      if (!parse(reflect::type_id<typename T::second_type>, str.substr(pos0, pos1 - pos0), &p.second))return false;
      return true;
    };

  } else if constexpr (experimental::is_detected_v<reflect::has_get_field_info_t, T>) {
    return [](const std::string &str, void *ptr) -> bool {
      if (str.front() != '{' || str.back() != '}')return false;
      T &t = *static_cast<T *>(ptr);
      size_t pos0 = 1, pos1;
      while (true) {
        pos1 = str.find(':', pos0);
        string key;
        if (!parse(reflect::type_id<string>, str.substr(pos0, pos1 - pos0), &key))return false;
        pos0 = pos1 + 1;
        while (isspace(str[pos0]))pos0++;
        if (str[pos0] == '{')
          pos1 = str.find('}', pos0) + 1;
        else
          pos1 = str.find(',', pos0);
        size_t off = t.get_offset_by_name(key);
        if (pos1 == string::npos) {
          pos1 = str.find('}', pos0);
          if (!parse(t.get_type_id_by_name(key), str.substr(pos0, pos1 - pos0), ((char *) &t) + off))return false;
          break;
        }
        if (!parse(t.get_type_id_by_name(key), str.substr(pos0, pos1 - pos0), ((char *) &t) + off))return false;
        pos0 = pos1 + 1;
      }
      return true;
    };
  } else {
    static_assert(experimental::is_detected_v<reflect::has_get_field_info_t, T>, "can not parse");
  }
}

int main() {
  //Object obj{1024, 2048.0};
  O2 obj{.w = 1024, .obj = {
      .i = 2048, .d = 4096
  }, .str = "8192"};
  const Object obj2{20, 4.0};
  Derived d{obj2};
  parser[reflect::type_id<int>] = get_parse_func<int>();
  parser[reflect::type_id<double>] = get_parse_func<double>();
  parser[reflect::type_id<string>] = get_parse_func<string>();
  parser[reflect::type_id<Object>] = get_parse_func<Object>();
  parser[reflect::type_id<O2>] = get_parse_func<O2>();
  auto o = reflect::reflect_default_serialize(obj);
  O2 o2;
  parse(reflect::type_id<O2>, o, &o2);
  cout << reflect::reflect_default_serialize(o2);
}