#include <iostream>
#include <unordered_set>
#include "src/v2/Reflectable.h"

// 引入命名空间
using namespace yuri;
using namespace std;

// CRTP声明，继承Reflectable<T>
struct A : public Reflectable<A> {
  // 使用ReflectField(name, initialize<optional>)声明一个序列化字段
 public:
  // 基本类型
  int ReflectField(a, = 1);
  // 字符串
  string ReflectField(str);
  // 容器
  std::vector<int> ReflectField(vec);
  std::unordered_map<int, string> ReflectField(id2str, {{ 7, "7" }, { 6, "6" }, { 3, "3" }});
  // 智能指针、嵌套
  std::shared_ptr<std::unordered_set<double>> ReflectField(uset, = nullptr);
  // 自包含
  std::unique_ptr<A> ReflectField(next, = nullptr);
};

// 只能组合，不能继承A
struct B : public Reflectable<B> {
  int ReflectField(b, = 5);
  A ReflectField(a);
};

int main() {
  A a;
  // 遍历字段
  for (auto info : a.getFieldInfoList()) {
    cout << info.name << ": " << info.offset << " isTypeof<int>?: " << boolalpha << info.isTypeOf<int>() << endl;
  }
  // 输出
  auto output = [](const A &a) { cout << a.toString() << endl; };
  output(a);
  // 通过字段名获取字段并修改
  a.getFieldByName<std::string>("str") = "hello, reflection";
  output(a);
  a.vec = std::vector<int>{1, 2, 3, 4, 5};
  output(a);
  a.uset = std::make_shared<std::unordered_set<double>>();
  a.uset->emplace(3.14);
  a.uset->emplace(1.414);
  a.uset->emplace(a.uset->size() + *a.uset->begin());
  output(a);
  a.next = std::make_unique<A>();
  a.next->str = "a.next";
  output(a);
  B b;
  cout << b.toString() << endl;
}