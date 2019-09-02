#ifndef LIBYURI_YURI_H_
#define LIBYURI_YURI_H_

#include <unordered_map>
#include <vector>
#include <string_view>
#include <functional>
#include <sstream>
#include <experimental/type_traits>
namespace reflect {
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

  // 序列化函数类型，传入一个void*，返回一个string
  using serialize_func = std::string(*)(const void *);

  // 简化获取type_id的代码
  template<typename T>
  inline static const TypeID type_id = Identifier<T>::ID;

  // 几个
  template<typename D>
  using has_begin_t = decltype(std::declval<D &>().begin());
  template<typename D>
  using has_end_t = decltype(std::declval<D &>().end());
  template<typename D>
  using has_first_t = decltype(std::declval<D &>().first);
  template<typename D>
  using has_second_t = decltype(std::declval<D &>().second);
  template<typename D>
  using has_get_field_info_t = decltype(std::declval<D &>().get_field_info_vec());
  class Serializer {
   public:
    inline static std::unordered_map<reflect::TypeID, serialize_func> handler;
   public:
    static std::string serialize(TypeID id, const void *ptr) {
      return handler[id](ptr);
    }
    template<typename _T>
    static std::string serialize(const void *ptr) {
      using T = std::remove_cv_t<_T>;
      return handler[type_id<T>](ptr);
    }
    template<typename _T>
    static void register_basic_func() {
      // 注册所有类型，包括无限定符、const、volatile、cv
      using T = std::remove_cv_t<_T>;
      using CT = std::add_const_t<T>;
      using VT = std::add_volatile_t<T>;
      using CVT = std::add_cv_t<T>;
      // 字符串
      if constexpr (std::is_same_v<T, std::string>) {
        handler[type_id<T>] = handler[type_id<CT>] = handler[type_id<VT>] = handler[type_id<CVT>] =
            [](const void *ptr) -> std::string {
              return '"' + *static_cast<const std::string *>(ptr) + '"';
            };
      } else if constexpr (std::is_same_v<T, bool>) {
        handler[type_id<T>] = handler[type_id<CT>] = handler[type_id<VT>] = handler[type_id<CVT>] =
            [](const void *ptr) -> std::string {
              return *static_cast<const bool *>(ptr) ? "true" : "false";
            };
      }
        // 算术类型
      else if constexpr (std::is_arithmetic_v<T>) {
        handler[type_id<T>] = handler[type_id<CT>] = handler[type_id<VT>] = handler[type_id<CVT>] =
            [](const void *ptr) -> std::string {
              return std::to_string(*static_cast<const T *>(ptr));
            };
      }
        // 指针类型，解一次引用再序列化
      else if constexpr (std::is_pointer_v<T>) {
        // 先注册指针指向的类型
        register_basic_func<std::remove_pointer_t<T>>();
        handler[type_id<T>] = handler[type_id<CT>] = handler[type_id<VT>] = handler[type_id<CVT>] =
            [](const void *ptr) -> std::string {
              auto p = *static_cast<const T *const *>(ptr);
              if (p == nullptr) return std::string("null");
              return handler[type_id<std::remove_pointer_t<T>>](p);
            };
      }
        // 枚举类型
      else if constexpr (std::is_enum_v<T>) {
        handler[type_id<T>] = handler[type_id<CT>] = handler[type_id<VT>] = handler[type_id<CVT>] =
            [](const void *ptr) -> std::string {
              return std::to_string(*static_cast<const T *>(ptr));
            };
      }
        // 检测有无get_field_info_vec()，如果有，代表是reflect类型
      else if constexpr(std::experimental::is_detected_v<has_get_field_info_t, T>) {
        handler[type_id<T>] = handler[type_id<CT>] = handler[type_id<VT>] = handler[type_id<CVT>] =
            [](const void *ptr) -> std::string {
              std::stringstream ss;
              const auto &t = *static_cast<const T *>(ptr);
              ss << "{";
              // 遍历所有字段并序列化
              for (auto it = t.get_field_info_vec().begin(), end = t.get_field_info_vec().end();;) {
                ss << '"' << it->name << '"' << ':'
                   << serialize(it->type_id, ((char *) (&t)) + it->offset);
                it++;
                if (it == end)break;
                ss << ",";
              }
              ss << "}";
              return ss.str();
            };
      } else {
        // 检测有无begin()、end()，如果有，则是可遍历类型
        if constexpr (std::experimental::is_detected_v<has_begin_t, T>
            && std::experimental::is_detected_v<has_end_t, T>) {
          register_basic_func<typename T::value_type>();
          handler[type_id<T>] = handler[type_id<CT>] = handler[type_id<VT>] = handler[type_id<CVT>] =
              [](const void *ptr) -> std::string {
                const auto &container = *static_cast<const T *>(ptr);
                std::stringstream ss;
                ss << '[';
                // 遍历所有数据并序列化
                for (auto it = container.begin(), end = container.end();;) {
                  ss << handler[type_id<typename T::value_type>](&(*it));
                  it++;
                  if (it == end)break;
                  ss << ',';
                }
                ss << ']';
                return ss.str();
              };
        }
          // 检测有无first、second成员，如果有，则是pair类型
        else if constexpr (std::experimental::is_detected_v<has_first_t, T>
            && std::experimental::is_detected_v<has_second_t, T>) {
          register_basic_func<typename T::first_type>();
          register_basic_func<typename T::second_type>();
          handler[type_id<T>] = handler[type_id<CT>] = handler[type_id<VT>] = handler[type_id<CVT>] =
              [](const void *ptr) -> std::string {
                const auto &pair = *static_cast<const T *>(ptr);
                std::stringstream ss;
                ss << '{' << '"' << "first" << '"' << ':' << serialize<typename T::first_type>(&pair.first)
                   << ',' << '"' << "second" << '"' << ':' << serialize<typename T::second_type>(&pair.second)
                   << '}';
                return ss.str();
              };
        } else {
          static_assert(std::experimental::is_detected_v<has_get_field_info_t, T>, "can not serialize");
        }
      }
    }
  };

  template<typename T>
  static std::string reflect_default_serialize(const T &t) {
    return Serializer::serialize<T>(&t);
  }
}

// 通过__COUNTER__保证每次调用该宏都产生一个唯一变量名
#define __counter_meta(val) __counter_##val
#define _counter_meta(val) __counter_meta(val)
#define _unique_var _counter_meta(__COUNTER__)

// 非继承、继承的using_reflect公共部分
// 由偏移量获取字段、由字段名获取字段、判断字段是否某一类型、获取全部字段信息这四类方法的声明
#define using_reflect_common(type) \
public: \
 template<typename T>\
 decltype(auto) get_field_by_offset(size_t offset){\
   return this->*reflect::offset_to_member_pointer<_reflect_type, T>(offset);\
 }\
 /*throw runtime_error("unknown offset") if offset not found; throw runtime_error("bad access") if offset and T is incompatible*/\
 template<typename T>\
 decltype(auto) get_field_by_offset_safe(size_t offset){\
  if (auto it = _offset_to_type_id.find(offset); it == _offset_to_type_id.end()){\
    throw std::runtime_error("unknown offset");\
  } else {\
    if (!reflect::is_type<T>(it->second)){\
      throw std::runtime_error("bad access by offset");\
    } else {\
      return this->get_field_by_offset<T>(offset);\
    }\
  }\
 }\
 /*throw runtime_error("unknown field") if field not found*/\
 template<typename T>\
 decltype(auto) get_field_by_name(std::string_view field_name){\
   if(auto it = _name_to_offset.find(field_name); it == _name_to_offset.end()){\
    throw std::runtime_error("unknown field");\
   }else{\
    return this->get_field_by_offset<T>(it->second);\
   }\
 }\
 template<typename T>\
 decltype(auto) get_field_by_offset(size_t offset) const {\
   return this->*reflect::offset_to_member_pointer<_reflect_type, T>(offset);\
 }\
 /*throw runtime_error("unknown offset") if offset not found; throw runtime_error("bad access") if offset and T is incompatible*/\
 template<typename T>\
 decltype(auto) get_field_by_offset_safe(size_t offset) const {\
  if (auto it = _offset_to_type_id.find(offset); it == _offset_to_type_id.end()){\
    throw std::runtime_error("unknown offset");\
  } else {\
    if (!reflect::is_type<T>(it->second)){\
      throw std::runtime_error("bad access by offset");\
    } else {\
      return this->get_field_by_offset<T>(offset);\
    }\
  }\
 }\
 /*throw runtime_error("unknown field") if field not found*/\
 template<typename T>\
 decltype(auto) get_field_by_name(std::string_view field_name) const {\
   if(auto it = _name_to_offset.find(field_name); it == _name_to_offset.end()){\
    throw std::runtime_error("unknown field");\
   }else{\
    return this->get_field_by_offset<T>(it->second);\
   }\
 }\
 /*throw runtime_error("unknown_field") if field not found*/\
 template<typename T>\
 static bool field_is_type(std::string_view field_name){\
  if(auto it = _name_to_type_id.find(field_name); it == _name_to_type_id.end()){\
    throw std::runtime_error("unknown_field");\
  }else{\
   return reflect::is_type<T>(it->second);\
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
 static const vector<reflect::FieldInfo>& get_field_info_vec() {return _field_info_vec;}\
 private:inline static const auto _unique_var = (reflect::Serializer::register_basic_func<type>(),0);

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
#define reflect_field(type, name, ...)\
 public: type name __VA_ARGS__;\
 private:\
 inline static const auto _##name##_offset = reflect::member_pointer_to_offset(&_reflect_type::name);\
 inline static const auto _##name##_type_id = reflect::Identifier<type>::ID;\
 inline static const auto _unique_var = (_name_to_offset[#name] = _##name##_offset, 0);\
 inline static const auto _unique_var = (_name_to_type_id[#name] = _##name##_type_id, 0);\
 inline static const auto _unique_var = (_offset_to_type_id[_##name##_offset] = _##name##_type_id, 0);\
 inline static const auto _unique_var = (_field_info_vec.emplace_back(reflect::FieldInfo{#name, _##name##_type_id, _##name##_offset}), 0);\
 inline static const auto _unique_var = (reflect::Serializer::register_basic_func<type>(), 0);

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

#endif
