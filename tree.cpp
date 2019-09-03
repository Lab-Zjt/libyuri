#include <iostream>
#include <memory>
#include "yuri.h"

using namespace std;

template<typename T>
struct Node {
 using_reflect(Node<T>);
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
  auto str = reflect::reflect_default_serialize(t);
  cout << str << endl;
  auto t2 = reflect::reflect_default_deserialize<Tree<int>>(str);
  cout << reflect::reflect_default_serialize(t2) << endl;
  std::any parse;
  size_t off = 0;
  using Object = unordered_map<string, any>;
  reflect::Deserializer::parse_unknown_field(str, off, &parse);
  auto &obj = any_cast<Object &>(parse);
  auto &root = any_cast<Object &>(obj["root"]);
  auto data = any_cast<double>(root["data"]);
  cout << data << endl;
}