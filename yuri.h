#ifndef LIBYURI_YURI_H_
#define LIBYURI_YURI_H_

#include <unordered_map>
#include <vector>
#include <string_view>
#include <functional>
#include <sstream>
#include <any>
#include <memory>
#include <experimental/type_traits>

#define YURI_ERROR std::cerr << __FILE__ << ':' << __LINE__ << '[' << __PRETTY_FUNCTION__ << ']'
#define YURI_LOG std::cerr << __FILE__ << ':' << __LINE__ << '[' << __PRETTY_FUNCTION__ << ']'
namespace reflect {
  /*
   * 一些基础设施
   */
  // 强制转换
  template<typename To, typename From>
  inline To force_cast(From from) {
    union {
      From f;
      To t;
    } u{from};
    return u.t;
  }
  // 将成员指针转换为偏移量
  template<typename _Mp>
  inline size_t member_pointer_to_offset(_Mp _p) {
    return force_cast<size_t>(_p);
  }
  // 将偏移量转换为成员指针
  template<typename _T, typename _M>
  inline _M _T::* offset_to_member_pointer(size_t _off) {
    return force_cast<_M _T::*>(_off);
  }
  // 类型ID，各种类型之间的ID不同，用于判断T是否属于某一种类型（思路来自std::any）
  using TypeID = const void *;
  template<typename _T>
  class Identifier {
   private:
    static void _id() {}
   public:
    inline static const auto ID = reinterpret_cast<const void *>(&Identifier<_T>::_id);
  };
  // 判断某个type_id是否T类型
  template<typename _T>
  inline bool is_type(TypeID id) {
    return id == Identifier<_T>::ID;
  }
  // 字段信息，包括字段名、该字段的类型ID，该字段的偏移量
  struct FieldInfo {
    const char *name;
    TypeID type_id;
    size_t offset;
    template<typename T>
    bool is_type() const { return reflect::is_type<T>(type_id); }
  };

  // 简化获取type_id的代码
  template<typename T>
  inline static const TypeID type_id = Identifier<T>::ID;

  // 判断是否可迭代
  template<typename D>
  using has_begin_t = decltype(std::declval<D &>().begin());
  template<typename D>
  using has_end_t = decltype(std::declval<D &>().end());
  // 判断是否pair
  template<typename D>
  using has_first_t = decltype(std::declval<D &>().first);
  template<typename D>
  using has_second_t = decltype(std::declval<D &>().second);
  // 判断是否reflect类型
  template<typename D>
  using has_get_field_info_t = decltype(std::declval<D &>().get_field_info_vec());
  template<typename D>
  using has_unique_t = decltype(std::declval<D &>().unique());
  template<typename D>
  using has_use_count_t = decltype(std::declval<D &>().use_count());
  template<typename D>
  constexpr bool is_shared_ptr_v =
      std::experimental::is_detected_v<has_unique_t, D> && std::experimental::is_detected_v<has_use_count_t, D>;




}

// 通过__COUNTER__保证每次调用该宏都产生一个唯一变量名
#define __counter_meta(val) __counter_##val
#define _counter_meta(val) __counter_meta(val)
#define _unique_var _counter_meta(__COUNTER__)

// 非继承、继承的using_reflect公共部分
// 由偏移量获取字段、由字段名获取字段、判断字段是否某一类型、获取全部字段信息这四类方法的声明
#define using_reflect_common(type) \
public: \
  bool has_field(std::string_view field_name){return _name_to_offset.find(field_name) != _name_to_offset.end();}\
 template<typename ___T>\
 decltype(auto) get_field_by_offset(size_t offset){\
   return this->*reflect::offset_to_member_pointer<_reflect_type, ___T>(offset);\
 }\
 /*throw runtime_error("unknown offset") if offset not found; throw runtime_error("bad access") if offset and T is incompatible*/\
 template<typename ___T>\
 decltype(auto) get_field_by_offset_safe(size_t offset){\
  if (auto it = _offset_to_type_id.find(offset); it == _offset_to_type_id.end()){\
    throw std::runtime_error("unknown offset");\
  } else {\
    if (!reflect::is_type<___T>(it->second)){\
      throw std::runtime_error("bad access by offset");\
    } else {\
      return this->get_field_by_offset<___T>(offset);\
    }\
  }\
 }\
 /*throw runtime_error("unknown field") if field not found*/\
 template<typename ___T>\
 decltype(auto) get_field_by_name(std::string_view field_name){\
   if(auto it = _name_to_offset.find(field_name); it == _name_to_offset.end()){\
    throw std::runtime_error("unknown field");\
   }else{\
    return this->get_field_by_offset<___T>(it->second);\
   }\
 }\
 template<typename ___T>\
 decltype(auto) get_field_by_offset(size_t offset) const {\
   return this->*reflect::offset_to_member_pointer<_reflect_type, ___T>(offset);\
 }\
 /*throw runtime_error("unknown offset") if offset not found; throw runtime_error("bad access") if offset and T is incompatible*/\
 template<typename ___T>\
 decltype(auto) get_field_by_offset_safe(size_t offset) const {\
  if (auto it = _offset_to_type_id.find(offset); it == _offset_to_type_id.end()){\
    throw std::runtime_error("unknown offset");\
  } else {\
    if (!reflect::is_type<___T>(it->second)){\
      throw std::runtime_error("bad access by offset");\
    } else {\
      return this->get_field_by_offset<___T>(offset);\
    }\
  }\
 }\
 /*throw runtime_error("unknown field") if field not found*/\
 template<typename ___T>\
 decltype(auto) get_field_by_name(std::string_view field_name) const {\
   if(auto it = _name_to_offset.find(field_name); it == _name_to_offset.end()){\
    throw std::runtime_error("unknown field");\
   }else{\
    return this->get_field_by_offset<___T>(it->second);\
   }\
 }\
 /*throw runtime_error("unknown_field") if field not found*/\
 template<typename ___T>\
 static bool field_is_type(std::string_view field_name){\
  if(auto it = _name_to_type_id.find(field_name); it == _name_to_type_id.end()){\
    throw std::runtime_error("unknown_field");\
  }else{\
   return reflect::is_type<___T>(it->second);\
  }\
 }\
 static reflect::TypeID get_type_id_by_name(std::string_view field_name){\
  if(auto it = _name_to_type_id.find(field_name); it == _name_to_type_id.end()){\
    throw std::runtime_error("unknown_field");\
  }else{\
   return it->second;\
  }\
 }\
 static size_t get_offset_by_name(std::string_view field_name){\
  if(auto it = _name_to_offset.find(field_name); it == _name_to_offset.end()){\
    throw std::runtime_error("unknown_field");\
  }else{\
   return it->second;\
  }\
 }\
 std::string get_field_string(std::string_view field_name){\
  if (auto it = _name_to_type_id.find(field_name); it == _name_to_type_id.end()){\
    throw std::runtime_error("unknown_field");\
  } else {\
    return Serializer::serialize_by_type_id(it->second, ((char*)this) + _name_to_offset[field_name]);\
  }\
 }\
 bool set_field_from_string(std::string_view field_name, const std::string& value){\
  if (auto it = _name_to_type_id.find(field_name); it == _name_to_type_id.end()){\
    throw std::runtime_error("unknown_field");\
  } else {\
    size_t off = 0;\
    return Deserializer::parse(it->second, value , off, ((char*)this) + _name_to_offset[field_name]);\
  }\
 }\
 static const std::vector<reflect::FieldInfo>& get_field_info_vec() {return _field_info_vec;}\
 private:inline static const auto _unique_var = (reflect::Serializer::register_handler<type>(),0);\
 inline static const auto _unique_var = (reflect::Deserializer::register_handler<type>(), 0)

// 非继承的using_reflect，声明了字段名到偏移量、字段名到类型id、偏移量到类型id三种map，以及一个保存所有字段信息的vector
#define using_reflect(type) \
 protected: \
 using _reflect_type = type;\
 inline static std::unordered_map<std::string_view, size_t> _name_to_offset;\
 inline static std::unordered_map<std::string_view, reflect::TypeID> _name_to_type_id;\
 inline static std::unordered_map<size_t, reflect::TypeID> _offset_to_type_id;\
 inline static std::vector<reflect::FieldInfo> _field_info_vec;\
 using_reflect_common(type);

// 声明某个字段为反射字段，可以进行初始化，但需要多一个逗号
// 然后将该字段注册到以上的三种map和一个vector中
// 用__attribute__((used))避免被编译器优化（gcc/clang）
#define reflect_field(type, name, ...)\
 public: type name __VA_ARGS__;\
 private:\
 inline static const auto _##name##_offset = reflect::member_pointer_to_offset(&_reflect_type::name);\
 inline static const auto _##name##_type_id = reflect::Identifier<type>::ID;\
 inline static const auto _unique_var __attribute__((used)) = (_name_to_offset[#name] = _##name##_offset, 0);\
 inline static const auto _unique_var __attribute__((used)) = (_name_to_type_id[#name] = _##name##_type_id, 0);\
 inline static const auto _unique_var __attribute__((used)) = (_offset_to_type_id[_##name##_offset] = _##name##_type_id, 0);\
 inline static const auto _unique_var __attribute__((used)) = (_field_info_vec.emplace_back(reflect::FieldInfo{#name, _##name##_type_id, _##name##_offset}), 0);\
 inline static const auto _unique_var __attribute__((used)) = (reflect::Serializer::register_handler<type>(), 0);\
 inline static const auto _unique_var __attribute__((used)) = (reflect::Deserializer::register_handler<type>(), 0);

// 从所有基类获取名称为map_name的map（名称范围为上述的三种map和一种vector），将它们的引用放进vector里并返回
// 因为要使用基类的成员，所以需要被声明在类里面
#define get_base_map(map_name) \
 private:\
template<typename T, typename ...Ts>\
static auto get_base_map##map_name() -> std::vector<std::pair<size_t,std::reference_wrapper<decltype(T::map_name)>>> {\
  return std::vector<std::pair<size_t,std::reference_wrapper<decltype(T::map_name)>>>{std::make_pair<size_t, std::reference_wrapper<decltype(T::map_name)>>(sizeof(T),std::ref(T::map_name)),std::make_pair<size_t, std::reference_wrapper<decltype(Ts::map_name)>>(sizeof(Ts),std::ref(Ts::map_name))...};\
}

// 声明四种get_base_map
#define def_get_base_map_func()\
get_base_map(_name_to_offset);\
get_base_map(_name_to_type_id);\
get_base_map(_offset_to_type_id);\
get_base_map(_field_info_vec);

// 根据map_name调用get_base_map
#define call_get_base_map(map_name) get_base_map##map_name

// 返回一个保存了所有基类的map_name数据的map或者vec
#define create_inherited_map(map_name, ...) [](){\
  typename decltype(call_get_base_map(map_name)<__VA_ARGS__>())::value_type::second_type::type map_copy;\
  for (auto&& m : call_get_base_map(map_name)<__VA_ARGS__>()){\
    for (auto&& p : m.second.get()){\
      map_copy.insert(map_copy.end(), p);\
    }\
  }\
  return map_copy;\
}()

// 需要计算偏移量
#define create_inherited_name_to_offset(...) [](){\
  typename decltype(call_get_base_map(_name_to_offset)<__VA_ARGS__>())::value_type::second_type::type map_copy;\
  size_t off = 0;\
  for(auto &&m : call_get_base_map(_name_to_offset)<__VA_ARGS__>()){\
    for (auto&& p: m.second.get()){\
      map_copy.insert(map_copy.end(), std::make_pair(p.first, p.second+off));\
    }\
    off += m.first;\
  }\
  return map_copy;\
}()

#define create_inherited_offset_to_type_id(...) [](){\
  typename decltype(call_get_base_map(_offset_to_type_id)<__VA_ARGS__>())::value_type::second_type::type map_copy;\
  size_t off = 0;\
  for(auto &&m : call_get_base_map(_offset_to_type_id)<__VA_ARGS__>()){\
    for (auto&& p: m.second.get()){\
      map_copy.insert(map_copy.end(), std::make_pair(p.first + off, p.second));\
    }\
    off += m.first;\
  }\
  return map_copy;\
}()

#define create_inherited_field_info_vec(...) [](){\
  typename decltype(call_get_base_map(_field_info_vec)<__VA_ARGS__>())::value_type::second_type::type map_copy;\
  size_t off = 0;\
  for(auto &&m : call_get_base_map(_field_info_vec)<__VA_ARGS__>()){\
    for (auto&& p: m.second.get()){\
      map_copy.insert(map_copy.end(), reflect::FieldInfo{p.name, p.type_id, p.offset+off});\
    }\
    off += m.first;\
  }\
  return map_copy;\
}()

#define using_reflect_inherit(type, ...)\
 private: \
 using _reflect_type = type;\
 def_get_base_map_func();\
 inline static std::unordered_map<std::string_view, size_t> _name_to_offset = create_inherited_name_to_offset(__VA_ARGS__);\
 inline static std::unordered_map<std::string_view, reflect::TypeID> _name_to_type_id = create_inherited_map(_name_to_type_id, __VA_ARGS__);\
 inline static std::unordered_map<size_t, reflect::TypeID> _offset_to_type_id = create_inherited_offset_to_type_id( __VA_ARGS__);\
 inline static std::vector<reflect::FieldInfo> _field_info_vec = create_inherited_field_info_vec(__VA_ARGS__);\
 using_reflect_common(type)

#define with_comma(...) __VA_ARGS__



namespace reflect {
  /*
   * 序列化
   */

  namespace json {

    // 序列化函数类型，传入一个void*，返回一个string
    using serialize_func = std::string(*)(const void *);

    template<typename T>
    inline static std::string serialize(const void *);

    class Serializer {
     public:
      inline static std::unordered_map<reflect::TypeID, serialize_func> handler;
     public:
      inline static std::string serialize_by_type_id(TypeID id, const void *ptr) {
        if (auto it = handler.find(id); it != handler.end()) {
          return it->second(ptr);
        }
        throw std::runtime_error("can not serialize");
      }
      template<typename _T>
      static void register_handler() {
        // 注册所有类型，包括无限定符、const、volatile、cv
        using T = std::remove_cv_t<_T>;
        using CT = std::add_const_t<T>;
        using VT = std::add_volatile_t<T>;
        using CVT = std::add_cv_t<T>;
        handler[type_id<T>] = handler[type_id<CT>] = handler[type_id<VT>] = handler[type_id<CVT>] = serialize<T>;
      }
    };

    template<typename _T>
    inline static std::string serialize(const void *ptr) {
      using T = std::remove_cv_t<_T>;
      const T &val = *static_cast<const T *>(ptr);
      // 字符串类型
      if constexpr (std::is_same_v<T, std::string>) {
        return '"' + val + '"';
      }
        // bool类型
      else if constexpr (std::is_same_v<T, bool>) {
        return val ? "true" : "false";
      }
        // 算术类型
      else if constexpr (std::is_arithmetic_v<T>) {
        return std::to_string(val);
      }
        // 指针类型，解一次引用再序列化
      else if constexpr (std::is_pointer_v<T>) {
        using element_type = std::remove_pointer_t<T>;
        if (val == nullptr) return std::string("null");
        return serialize<element_type>(val);
      }
        // 智能指针类型
      else if constexpr (is_shared_ptr_v<T>) {
        using element_type = typename T::element_type;
        if (val == nullptr) return std::string("null");
        return serialize<element_type>(val.get());
      }
        // 枚举类型
      else if constexpr (std::is_enum_v<T>) {
        return std::to_string(val);
      }
        // 检测有无get_field_info_vec()，如果有，代表是object类型
      else if constexpr(std::experimental::is_detected_v<has_get_field_info_t, T>) {
        std::stringstream ss;
        ss << "{";
        // 遍历所有字段并序列化
        for (auto it = val.get_field_info_vec().begin(), end = val.get_field_info_vec().end();;) {
          ss << '"' << it->name << '"' << ':'
             << Serializer::serialize_by_type_id(it->type_id, ((char *) (&val)) + it->offset);
          it++;
          if (it == end)break;
          ss << ",";
        }
        ss << "}";
        return ss.str();
      } else {
        // 检测有无begin()、end()，如果有，则是可遍历类型
        if constexpr (std::experimental::is_detected_v<has_begin_t, T>
            && std::experimental::is_detected_v<has_end_t, T>) {
          using value_type = typename T::value_type;
          std::stringstream ss;
          ss << '[';
          // 遍历所有数据并序列化
          for (auto it = val.begin(), end = val.end();;) {
            ss << serialize<value_type>(&(*it));
            it++;
            if (it == end)break;
            ss << ',';
          }
          ss << ']';
          return ss.str();
        }
          // 检测有无first、second成员，如果有，则是pair类型
        else if constexpr (std::experimental::is_detected_v<has_first_t, T>
            && std::experimental::is_detected_v<has_second_t, T>) {
          using first_type = typename T::first_type;
          using second_type = typename T::second_type;
          std::stringstream ss;
          ss << '{' << '"' << "first" << '"' << ':' << serialize<first_type>(&val.first)
             << ',' << '"' << "second" << '"' << ':' << serialize<second_type>(&val.second)
             << '}';
          return ss.str();

        }
          // 不是任何一种可序列化的类型，报错
        else {
          static_assert(std::experimental::is_detected_v<has_get_field_info_t, T>, "can not serialize");
        }
      }
    }
  }
  using Serializer = json::Serializer;
  template<typename T>
  std::string dumps(const T &t) {
    return json::serialize<T>(&t);
  }
}

#include <iostream>

#define PARSE_ERROR(what) YURI_ERROR << "parse " << what << " failed. str = " << (std::string(str).insert(off,"\e[31m").insert(str.size(),"\e[0m")) << ", off = " << off  << "\n"
#define PARSE_SPACE() if (!parse_space(str, off)){PARSE_ERROR("space");return false;}
namespace reflect {
  using Object = std::unordered_map<std::string, std::any>;
  namespace json {
    /*
  * 反序列化
  */
    // parse函数类型，传入要解析的字符串、偏移量、要赋值的地址
    using parse_func = bool (*)(const std::string &, size_t &off, void *);

    // 对于null，调用parse_unknown_field会解析成该类型
    struct null {};
    // 前向声明
    template<typename T>
    inline static bool deserialize(const std::string &, size_t &off, void *ptr);
    inline bool parse_space(const std::string &str, size_t &off);

    inline bool isspace(char c) {
      switch (c) {
        case '\t':
        case '\n':
        case '\f':
        case '\v':
        case '\r':
        case ' ':return true;
        default:return false;
      }
    }

    // 反序列化类
    class Deserializer {
     public:
      // 管理type_id到parse_func的映射
      // 初始化时注册用于解析unknown_field的函数
      inline static std::unordered_map<TypeID, parse_func> handler{
          {type_id<bool>, deserialize<bool>},
          {type_id<std::string>, deserialize<std::string>},
          {type_id<double>, deserialize<double>}
      };
     public:
      inline static bool parse(reflect::TypeID id, const std::string &str, size_t &off, void *ptr) {
        PARSE_SPACE();
        if (auto it = handler.find(id); it != handler.end()) {
          return it->second(str, off, ptr);
        } else {
          YURI_ERROR << "no such handler\n";
          return false;
        }
      }
      // 解析一个未知的字段
      // 如果out是nullptr，则丢弃解析结果
      static bool parse_unknown_field(const std::string &str, size_t &off, std::any *out) __attribute__((noinline));
      // 注册一个T类型的parse_func到Deserializer的handler中
      template<typename T>
      inline static void register_handler() { handler[type_id<T>] = deserialize<T>; }
    };

    template<typename T>
    inline static T reflect_default_deserialize(const std::string &str) {
      size_t off = 0;
      T t;
      if (!Deserializer::parse(type_id<T>, str, off, &t)) {
        PARSE_ERROR("can not parse");
      }
      return t;
    }

    inline static std::any reflect_deserialize_unknown(const std::string &str) {
      size_t off = 0;
      std::any val;
      if (!Deserializer::parse_unknown_field(str, off, &val)) {
        PARSE_ERROR("unknown field");
      }
      return val;
    }

    // 具体实现


    // 解析空格
    inline bool parse_space(const std::string &str, size_t &off) {
      while (isspace(str[off]) && off < str.size())off++;
      return off < str.size();
    }
    // 解析一个字符，如果str[off]是该字符，则解析成功，off自增1并返回true，否则返回false
    inline bool parse_ch(char ch, const std::string &str, size_t &off) {
      PARSE_SPACE();
      if (str[off] != ch)return false;
      off++;
      return true;
    }
// 获取类型T的反序列化函数
    template<typename T>
    inline static bool deserialize(const std::string &str, size_t &off, void *ptr) {
      // bool类型
      if constexpr (std::is_same_v<T, bool>) {
        PARSE_SPACE();
        // 判断字符串是true还是false
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
        return true;
      }
        // 整数类型，先转为long long，再强转为T
      else if constexpr (std::is_integral_v<T>) {
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
      }
        // 浮点类型，先转为double，再强转为T
      else if constexpr(std::is_floating_point_v<T>) {
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
      }
        // 字符串类型
      else if constexpr(std::is_same_v<T, std::string>) {
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
        // 引号
        off += 1;
        // 字符串内容
        while ((str[off] != '"' || str[off - 1] == '\\') && off < str.size())off++;
        if (off >= str.size() || str[off] != '"') {
          PARSE_ERROR("string");
          off = save;
          return false;
        }
        t = str.substr(save + 1, off - (save + 1));
        // 引号
        off += 1;
        return true;
      }
        // 可迭代类型
      else if constexpr (std::experimental::is_detected_v<reflect::has_begin_t, T>
          && std::experimental::is_detected_v<reflect::has_end_t, T>) {
        using value_type = typename T::value_type;
        // 左中括号
        if (!parse_ch('[', str, off)) {
          PARSE_ERROR("[");
          return false;
        }
        // 循环解析每一个元素
        auto &vec = *static_cast<T *>(ptr);
        vec.clear();
        // 该数组中没有元素
        if (parse_ch(']', str, off)) {
          return true;
        }
        while (true) {
          value_type v;
          if (!deserialize<value_type>(str, off, &v)) {
            PARSE_ERROR("array_element");
            return false;
          }
          vec.insert(vec.end(), std::move(v));
          // 没有逗号，是最后一个元素
          if (!parse_ch(',', str, off)) {
            break;
          }
        }
        // 右中括号
        if (!parse_ch(']', str, off)) {
          PARSE_ERROR("]");
          return false;
        }
        return true;
      }
        // pair类型，针对map<K,V>::value_type是pair<const K,V>做了处理，用const_cast强制转换成可变类型
      else if constexpr (std::experimental::is_detected_v<reflect::has_first_t, T>
          && std::experimental::is_detected_v<reflect::has_second_t, T>) {
        // 注册first_type解析
        using first_type = typename T::first_type;
        using remove_cv_first_type = std::remove_cv_t<first_type>;
        // 注册second_type解析
        using second_type = typename T::second_type;
        if (!parse_ch('{', str, off)) {
          PARSE_ERROR("'{'");
          return false;
        }
        auto &p = *static_cast<T *>(ptr);
        std::string key;
        if (!deserialize<std::string>(str, off, &key)) {
          PARSE_ERROR(key);
          return false;
        }
        if (key != "first") {
          PARSE_ERROR("first");
          return false;
        }
        if (!parse_ch(':', str, off)) {
          PARSE_ERROR(":");
          return false;
        }
        if (!deserialize<remove_cv_first_type>(str, off, const_cast<remove_cv_first_type *>(&p.first))) {
          PARSE_ERROR(key);
          return false;
        }
        if (!parse_ch(',', str, off)) {
          PARSE_ERROR("','");
          return false;
        }
        if (!deserialize<std::string>(str, off, &key)) {
          PARSE_ERROR(key);
          return false;
        }
        if (key != "second") {
          PARSE_ERROR("second");
          return false;
        }
        if (!parse_ch(':', str, off)) {
          PARSE_ERROR(":");
          return false;
        }
        if (!deserialize<second_type>(str, off, &p.second)) {
          PARSE_ERROR(key);
          return false;
        }
        if (!parse_ch('}', str, off)) {
          PARSE_ERROR("'}'");
          return false;
        }
        return true;
      }
        // reflect类型
      else if constexpr (std::experimental::is_detected_v<reflect::has_get_field_info_t, T>) {
        // 需要先注册std::string以解析key
        if (!parse_ch('{', str, off)) {
          PARSE_ERROR("{");
          return false;
        }
        // 该对象没有字段
        if (parse_ch('}', str, off)) {
          return true;
        }
        T &t = *static_cast<T *>(ptr);
        // 循环解析字段
        while (true) {
          // 解析key
          std::string key;
          if (!deserialize<std::string>(str, off, &key)) {
            PARSE_ERROR(key);
            return false;
          }
          if (!parse_ch(':', str, off)) {
            PARSE_ERROR(":");
            return false;
          }
          // 不存在该key时，调用parse_unknown_field以解析未知该value并丢弃
          if (!t.has_field(key)) {
            if (!Deserializer::parse_unknown_field(str, off, nullptr)) {
              PARSE_ERROR("unknown field " + key);
              return false;
            }
          } else {
            // 存在该key时，获取该key对应的type_id、偏移量，然后递归调用parse解析
            size_t offset = t.get_offset_by_name(key);
            reflect::TypeID id = t.get_type_id_by_name(key);
            if (!Deserializer::parse(id, str, off, ((char *) ptr) + offset)) {
              PARSE_ERROR(key);
              return false;
            }
          }
          // 没有逗号，是最后一个字段
          if (!parse_ch(',', str, off)) {
            break;
          }
        }
        // 右大括号
        if (!parse_ch('}', str, off)) {
          PARSE_ERROR("}");
          return false;
        }
        return true;
      }
        // 智能指针类型
      else if constexpr(is_shared_ptr_v<T>) {
        using element_type = typename T::element_type;
        PARSE_SPACE();
        // 解析空指针
        if (str.substr(off, 4) == "null") {
          *static_cast<T *>(ptr) = nullptr;
          off += 4;
          return true;
        }
        *static_cast<T *>(ptr) = std::make_shared<element_type>();
        if (!deserialize<element_type>(str, off, static_cast<T *>(ptr)->get())) {
          PARSE_ERROR("pointer");
          return false;
        }
        return true;
      } else {
        static_assert(std::experimental::is_detected_v<reflect::has_get_field_info_t, T>, "can not parse");
      }
    }
    bool Deserializer::parse_unknown_field(const std::string &str, size_t &off, std::any *out) {
      PARSE_SPACE();
      switch (str[off]) {
        // 数组
        case '[': {
          // 左中括号
          parse_ch('[', str, off);
          std::vector<std::any> va;
          // 循环解析数组内容
          for (;;) {
            std::any val;
            if (!parse_unknown_field(str, off, &val)) {
              PARSE_ERROR("unknown_field");
              return false;
            }
            va.emplace_back(std::move(val));
            if (!parse_ch(',', str, off)) {
              break;
            }
          }
          // 右中括号
          if (!parse_ch(']', str, off)) {
            PARSE_ERROR("]");
            return false;
          }
          if (out != nullptr)*out = std::any(std::move(va));
          return true;
        }
          // 对象
        case '{': {
          // 左大括号
          parse_ch('{', str, off);
          Object obj;
          // 循环解析字段
          while (true) {
            std::string key;
            // 解析key，是一个string
            if (!deserialize<std::string>(str, off, &key)) {
              PARSE_ERROR("key");
              return false;
            }
            if (!parse_ch(':', str, off)) {
              PARSE_ERROR(":");
              return false;
            }
            // 解析value，递归调用parse_unknown_field
            std::any val;
            if (!parse_unknown_field(str, off, &val)) {
              PARSE_ERROR("unknown_field");
              return false;
            }
            obj[std::move(key)] = std::move(val);
            if (!parse_ch(',', str, off)) {
              break;
            }
          }
          // 右大括号
          if (!parse_ch('}', str, off)) {
            PARSE_ERROR("}");
            return false;
          }
          if (out != nullptr)*out = std::any(std::move(obj));
          return true;
        }
          // 字符串
        case '"': {
          std::string key;
          if (!deserialize<std::string>(str, off, &key)) {
            PARSE_ERROR("string");
            return false;
          }
          if (out != nullptr) *out = std::any(std::move(key));
          return true;
        }
          // bool
        case 't':
        case 'f': {
          bool b;
          if (!deserialize<bool>(str, off, &b)) {
            PARSE_ERROR("bool");
            return false;
          }
          if (out != nullptr) *out = std::any(b);
          return true;
        }
          // null
        case 'n': {
          if (str.substr(off, 4) != "null") {
            PARSE_ERROR("null");
            return false;
          }
          off += 4;
          if (out != nullptr) *out = std::any(null{});
          return true;
        }
          // number，一律解析成double
        default: {
          double d;
          if (!deserialize<double>(str, off, &d)) {
            PARSE_ERROR("number");
            return false;
          }
          if (out != nullptr) *out = std::any(d);
          return true;
        }
      }
      YURI_ERROR << "should not reach here!\n";
      return false;
    }
  }
  using Deserializer = json::Deserializer;
  template<typename T>
  T parse(const std::string &str) {
    size_t off = 0;
    T t;
    if (!json::deserialize<T>(str, off, &t)) {
      PARSE_ERROR("object");
    }
    return t;
  }
  std::any parse(const std::string &str) {
    size_t off = 0;
    std::any a;
    if (!Deserializer::parse_unknown_field(str, off, &a)) {
      PARSE_ERROR("object");
    }
    return a;
  }
}
#undef PARSE_ERROR
#undef PARSE_SPACE
#endif
