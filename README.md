# libyuri

libyuri是一个简单的、基于宏的C++反射库。它只需要包含一个头文件即可使用。提供字段遍历、根据字段名获取字段、序列化、反序列化(json)功能。

需要C++17以使用`inline static`类成员变量,以及头文件`<experimental/type_traits>`以使用`is_detected_v`。

支持类型：基础类型、std::string、std::pair、STL可遍历容器（需要`begin()`,`end()`,`value_type`）、以及使用上述类型组合出的类型，以及智能指针（会序列化/反序列化指针指向的内容）。裸指针可以被序列化，但不能被反序列化，因为其生命周期无法管理。

提供反序列化未知结构体的功能，object表示为`unordered_map<string,any>`，number表示为double，null表示为`reflect::null`，string表示为`std::string`。

## 使用

```C++
#include <iostream>
#include "yuri.h"

using namespace std;

struct Object {
  // 声明使用reflect，则该类型为reflect类型
 using_reflect(Object);
 // 初始化，需要写在第二个逗号后面
 reflect_field(int, i, = 2);
 reflect_field(double, d);
};

struct O2 {
 using_reflect(O2);
 reflect_field(int, w);
 // 使用reflect类型
 reflect_field(Object, obj);
 // 字符串类型
 reflect_field(string, str);
};

struct Derived : Object, O2 {
  // 使用继承，基类需要是reflect类型，继承的声明顺序与类定义时的继承顺序一样
 using_reflect_inherit(Derived, Object, O2);
 reflect_field(std::string, msg, { "hello,world" });
 // 支持容器
 reflect_field(std::vector<Object>, vec, {{ 3, 4.5 }, { 5, 7.5 }, { 7, 10.5 }});
 reflect_field(with_comma(unordered_map<string, Object>), o, {
   { "123", Object{.i = 17, .d = 23.24}},
   { "456", Object{.i = 24, .d = 34.56}}
 });
 // 支持智能指针
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
  // 序列化
  auto o = reflect::reflect_default_serialize(d);
  // 反序列化
  auto o2 = reflect::reflect_default_deserialize<Derived>(o);
  // 前后结果相同
  cout << reflect::reflect_default_serialize(d) << '\n';
  cout << reflect::reflect_default_serialize(o2) << '\n';
}
```

树：

```c++
#include <iostream>
#include <memory>
#include "yuri.h"

using namespace std;

template<typename T>
struct Node {
  // 泛型声明
 using_reflect(Node<T>);
 // 智能指针声明
 reflect_field(shared_ptr<Node>, left);
 reflect_field(shared_ptr<Node>, right);
 reflect_field(T, data);
 public:
  Node() : left(nullptr), right(nullptr), data(0) {}
  Node(int i) : left(nullptr), right(nullptr), data(i) {}
};

template<typename T>
struct Tree {
 using_reflect(Tree<T>);
 reflect_field(shared_ptr<Node<T>>, root);
};

int main() {
  Tree<int> t;
  t.root = make_shared<Node<int>>(17);
  t.root->left = make_shared<Node<int>>(26);
  t.root->right = make_shared<Node<int>>(31);
  t.root->left->left = make_shared<Node<int>>(42);
  t.root->left->right = make_shared<Node<int>>(47);
  t.root->right->left = make_shared<Node<int>>(49);
  // t.root->right->right = nullptr
  // nullptr会被序列化为null，null会被反序列化为nullptr
  auto str = reflect::reflect_default_serialize(t);
  cout << str << endl;
  auto t2 = reflect::reflect_default_deserialize<Tree<int>>(str);
  cout << reflect::reflect_default_serialize(t2) << endl;
}
```