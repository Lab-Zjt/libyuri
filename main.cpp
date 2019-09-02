#include <iostream>
#include <variant>
#include <memory>
#include "yuri.h"

using namespace std;

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

struct Derived : Object, O2 {
 using_reflect_inherit(Derived, Object, O2);
 reflect_field(std::string, msg, { "hello,world" });
 reflect_field(std::vector<Object>, vec, {{ 3, 4.5 }, { 5, 7.5 }, { 7, 10.5 }});
 reflect_field(with_comma(unordered_map<string, Object>), o, {
   { "123", Object{.i = 17, .d = 23.24}},
   { "456", Object{.i = 24, .d = 34.56}}
 });
 reflect_field(shared_ptr<Object>, so, = make_shared<Object>());
};

int main() {
  //Object obj{1024, 2048.0};
  O2 obj{.w = 1024, .obj = {
      .i = 2048, .d = 4096
  }, .str = "8192"};
  const Object obj2{20, 4.0};
  Derived d{obj2};
  d.so->i = 137;
  d.so->d = 268.37;
  auto o = reflect::reflect_default_serialize(d);
  auto o2 = reflect::reflect_default_deserialize<Derived>(o);
  cout << reflect::reflect_default_serialize(d) << '\n';
  cout << reflect::reflect_default_serialize(o2) << '\n';
}