#include <iostream>
#include <vector>
#include <unordered_map>
#include <sstream>
#include <queue>
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

int main() {
  //Object obj{1024, 2048.0};
  O2 obj{.w = 1024, .obj = {
      .i = 2048, .d = 4096
  }, .str = "8192"};
  cout << reflect::reflect_default_serialize(obj) << "\n";
  const Object obj2{20, 4.0};
  Derived d{obj2};
  cout << reflect::reflect_default_serialize(d) << "\n";

}