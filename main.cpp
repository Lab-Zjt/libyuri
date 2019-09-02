#include <iostream>
#include <variant>
#include "yuri.h"
#include "parser.h"

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

using parse_func = std::function<bool(const std::string &, size_t &off, void *)>;

inline static unordered_map<reflect::TypeID, parse_func> parser;

string trim(const std::string &str) {
  auto pos1 = str.find_first_not_of("\r\n\t\v\f ");
  auto pos2 = str.find_last_not_of("\r\n\t\v\f ");
  return str.substr(pos1, pos2 - pos1 + 1);
}

inline static bool parse(reflect::TypeID id, const std::string &str, size_t &off, void *ptr) {
  PARSE_SPACE();
  if (auto it = parser.find(id); it != parser.end()) {
    return it->second(str, off, ptr);
  } else {
    ERROR << "no such handler\n";
    return false;
  }
}

template<typename T>
parse_func get_parse_func() {
  if constexpr (std::is_same_v<T, bool>) {
    return [](const std::string &str, size_t &off, void *ptr) -> bool {
      PARSE_SPACE();
      auto &t = *static_cast<T *>(ptr);
      if (str[off] == 't') {
        if (str.substr(off, 4) == "true") {
          t = true;
          off += 4;
        } else {
          PARSE_ERROR("bool");
          return false;
        }
      } else if (str[off] == 'f') {
        if (str.substr(off, 5) == "false") {
          t = false;
          off += 5;
        } else {
          PARSE_ERROR("bool");
          return false;
        }
      } else {
        PARSE_ERROR("bool");
        return false;
      }
    };
  } else if constexpr (std::is_integral_v<T>) {
    return [](const std::string &str, size_t &off, void *ptr) -> bool {
      PARSE_SPACE();
      auto &t = *static_cast<T *>(ptr);
      char *end;
      auto val = strtoll(str.c_str() + off, &end, 10);
      if (errno != 0 || end == str.c_str()) {
        PARSE_ERROR("integral");
        return false;
      }
      off = end - str.c_str();
      t = static_cast<T>(val);
      return true;
    };
  } else if constexpr(std::is_floating_point_v<T>) {
    return [](const std::string &str, size_t &off, void *ptr) -> bool {
      PARSE_SPACE();
      auto &t = *static_cast<T *>(ptr);
      char *end;
      auto val = strtof64(str.c_str() + off, &end);
      if (errno != 0 || end == str.c_str()) {
        PARSE_ERROR("floating_point");
        return false;
      }
      off = end - str.c_str();
      t = static_cast<T>(val);
      return true;
    };
  } else if constexpr(std::is_same_v<T, std::string>) {
    return [](const std::string &str, size_t &off, void *ptr) -> bool {
      auto &t = *static_cast<T *>(ptr);
      PARSE_SPACE();
      if (str.size() - off < 2) {
        PARSE_ERROR("string");
        return false;
      }
      if (str[off] != '"') {
        PARSE_ERROR("string");
        return false;
      }
      auto save = off;
      // "
      off += 1;
      // string content
      while (str[off] != '"' && str[off - 1] != '\\' && off < str.size())off++;
      if (off >= str.size() || str[off] != '"') {
        PARSE_ERROR("string");
        off = save;
        return false;
      }
      t = str.substr(save + 1, off - (save + 1));
      // "
      off += 1;
      return true;
    };
  } else if constexpr (experimental::is_detected_v<reflect::has_begin_t, T>
      && experimental::is_detected_v<reflect::has_end_t, T>) {
    return [](const std::string &str, size_t &off, void *ptr) -> bool {
      if (!parse_bracket_left(str, off)) {
        PARSE_ERROR("[");
        return false;
      }
      auto &vec = *static_cast<T *>(ptr);
      vec.clear();
      using value_type = typename T::value_type;
      value_type v;
      while (true) {
        if (!parse(reflect::type_id<value_type>, str, off, &v)) {
          PARSE_ERROR("array_element");
          return false;
        }
        vec.insert(vec.end(), v);
        // no comma means end of array
        if (!parse_comma(str, off)) {
          break;
        }
      }
      if (!parse_bracket_right(str, off)) {
        PARSE_ERROR("]");
        return false;
      }
      return true;
    };
  } else if constexpr (experimental::is_detected_v<reflect::has_first_t, T>
      && experimental::is_detected_v<reflect::has_second_t, T>) {
    return [](const std::string &str, size_t &off, void *ptr) -> bool {
      if (!parse_curly_left(str, off)) {
        PARSE_ERROR("'{'");
        return false;
      }
      using first_type = typename T::first_type;
      using remove_cv_first_type = std::remove_cv_t<first_type>;
      using second_type = typename T::second_type;
      auto &p = *static_cast<T *>(ptr);
      std::string key;
      if (!parse(reflect::type_id<string>, str, off, &key)) {
        PARSE_ERROR(key);
        return false;
      }
      if (key != "first") {
        PARSE_ERROR("first");
        return false;
      }
      if (!parse_colon(str, off)) {
        PARSE_ERROR(":");
        return false;
      }
      if (!parse(reflect::type_id<remove_cv_first_type>,
                 str,
                 off,
                 const_cast<remove_cv_first_type *>(&p.first))) {
        PARSE_ERROR(key);
        return false;
      }
      if (!parse_comma(str, off)) {
        PARSE_ERROR("','");
        return false;
      }
      if (!parse(reflect::type_id<string>, str, off, &key)) {
        PARSE_ERROR(key);
        return false;
      }
      if (key != "second") {
        PARSE_ERROR("second");
        return false;
      }
      if (!parse_colon(str, off)) {
        PARSE_ERROR(":");
        return false;
      }
      if (!parse(reflect::type_id<second_type>, str, off, &p.second)) {
        PARSE_ERROR(key);
        return false;
      }
      if (!parse_curly_right(str, off)) {
        PARSE_ERROR("'}'");
        return false;
      }
      return true;
    };

  } else if constexpr (experimental::is_detected_v<reflect::has_get_field_info_t, T>) {
    return [](const std::string &str, size_t &off, void *ptr) -> bool {
      if (!parse_curly_left(str, off)) {
        PARSE_ERROR("'{'");
        return false;
      }
      T &t = *static_cast<T *>(ptr);
      while (true) {
        string key;
        if (!parse(reflect::type_id<string>, str, off, &key)) {
          PARSE_ERROR(key);
          return false;
        }
        if (!parse_colon(str, off)) {
          PARSE_ERROR(":");
          return false;
        }
        size_t offset = t.get_offset_by_name(key);
        reflect::TypeID id = t.get_type_id_by_name(key);
        if (!parse(id, str, off, ((char *) ptr) + offset)) {
          PARSE_ERROR(key);
          return false;
        }
        // no comma means end of field
        if (!parse_comma(str, off)) {
          break;
        }
      }
      if (!parse_curly_right(str, off)) {
        PARSE_ERROR("'}'");
        return false;
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
  parser[reflect::type_id<Derived>] = get_parse_func<Derived>();
  parser[reflect::type_id<vector<Object>>] = get_parse_func<vector<Object>>();
  parser[reflect::type_id<unordered_map<string, Object>>] = get_parse_func<unordered_map<string, Object>>();
  parser[reflect::type_id<std::pair<const std::string, Object>>] =
      get_parse_func<std::pair<const string, Object>>();
  auto o = reflect::reflect_default_serialize(d);
  Derived o2;
  size_t off = 0;
  parse(reflect::type_id<Derived>, o, off, &o2);
  cout << reflect::reflect_default_serialize(d) << '\n';
  cout << reflect::reflect_default_serialize(o2) << '\n';
}