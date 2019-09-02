#ifndef LIBYURI_YURI_H_
#define LIBYURI_YURI_H_

#include <unordered_map>
#include <vector>
#include <string_view>
#include <functional>
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
  inline size_t _member_pointer_to_offset(_Mp _p) {
    return force_cast<size_t>(_p);
  }
  // 将偏移量转换为成员指针
  template<typename _T, typename _M>
  inline _M _T::* _offset_to_member_pointer(size_t _off) {
    return force_cast<_M _T::*>(_off);
  }
  // 类型ID，各种类型之间的ID不同，用于判断T是否属于某一种类型（思路来自std::any）
  using _TypeID = void *;
  template<typename _T>
  class _Identifier {
   private:
    static void _id() {}
   public:
    inline static const auto ID = reinterpret_cast<void *>(&_Identifier<_T>::_id);
  };
  template<typename _T>
  inline bool _is_type(_TypeID id) {
    return id == _Identifier<_T>::ID;
  }
  // 字段信息，包括字段名、该字段的类型ID，该字段的偏移量
  struct FieldInfo {
    const char *name;
    _TypeID type_id;
    size_t offset;
    template<typename T>
    bool is_type() const { return _is_type<T>(type_id); }
  };

  template<typename T>
  static std::string reflect_default_serialize(const T &t);

  template<typename T>
  using serialize_func = std::function<std::string(const T &, size_t)>;

  template<typename T>
  using use_base_serializer_func = std::optional<std::string>(*)(_TypeID id, const T &t, size_t off);

  template<typename T>
  inline static const _TypeID _type_id = _Identifier<T>::ID;

  inline static std::unordered_map<_TypeID, std::string(*)(void *)> global_serializer;

  template<typename D>
  using has_begin_t = decltype(std::declval<D &>().begin());
  template<typename D>
  using has_end_t = decltype(std::declval<D &>().end());
  template<typename D>
  using has_first_t = decltype(std::declval<D &>().first);
  template<typename D>
  using has_second_t = decltype(std::declval<D &>().second);
  template<typename T>
  class Serializer {
   public:
    std::unordered_map<reflect::_TypeID, serialize_func<T>> handler;
   public:
    std::string serialize(reflect::_TypeID type_id, const T &obj, size_t offset) {
      if (auto it = handler.find(type_id); it != handler.end()) {
        return it->second(obj, offset);
      } else {
        return global_serializer[type_id](((char *) &obj) + offset);
      }
    }
    template<typename D>
    void register_basic_func() {
      if constexpr (std::is_same_v<D, std::string>) {
        handler[_type_id<D>] = [](const T &obj, size_t offset) -> std::string {
          return obj.template get_field_by_offset<std::string>(offset);
        };
      } else if constexpr (std::is_arithmetic_v<D>) {
        handler[_type_id<D>] = [](const T &obj, size_t offset) -> std::string {
          return std::to_string(obj.template get_field_by_offset<D>(offset));
        };
      } else {
        if constexpr (!(std::experimental::is_detected_v<has_begin_t, D>
            && std::experimental::is_detected_v<has_end_t, D>)) {
          static_assert(1 != 2, "can not convert");
        } else {
          handler[_type_id<D>] = [](const T &obj, size_t offset) -> std::string {
            const auto &vec = obj.template get_field_by_offset<D>(offset);
            std::stringstream ss;
            ss << '[';
            for (auto it = vec.begin(), end = vec.end();;) {
              ss << reflect_default_serialize(*it);
              it++;
              if (it == end)break;
              ss << ',';
            }
            ss << ']';
            return ss.str();
          };
        }
      }
    }
  };
  template<typename T>
  static std::string reflect_default_serialize(const T &t) {
    if constexpr (std::is_arithmetic_v<T>) {
      return std::to_string(t);
    } else if constexpr (std::is_same_v<T, std::string>) {
      return '"' + t + '"';
    } else {
      std::stringstream ss;
      ss << "{";
      for (auto it = t.get_field_info_vec().begin(), end = t.get_field_info_vec().end();;) {
        ss << '"' << it->name << '"' << ':';
        bool is_string = it->template is_type<std::string>();
        if (is_string) ss << '"';
        ss << t._serializer.serialize(it->type_id, t, it->offset);
        if (is_string)ss << '"';
        it++;
        if (it == end)break;
        ss << ",";
      }
      ss << "}";
      return ss.str();
    }
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
   return this->*reflect::_offset_to_member_pointer<_reflect_type, T>(offset);\
 }\
 /*throw runtime_error("unknown offset") if offset not found; throw runtime_error("bad access") if offset and T is incompatible*/\
 template<typename T>\
 decltype(auto) get_field_by_offset_safe(size_t offset){\
  if (auto it = _offset_to_type_id.find(offset); it == _offset_to_type_id.end()){\
    throw std::runtime_error("unknown offset");\
  } else {\
    if (!reflect::_is_type<T>(it->second)){\
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
   return this->*reflect::_offset_to_member_pointer<_reflect_type, T>(offset);\
 }\
 /*throw runtime_error("unknown offset") if offset not found; throw runtime_error("bad access") if offset and T is incompatible*/\
 template<typename T>\
 decltype(auto) get_field_by_offset_safe(size_t offset) const {\
  if (auto it = _offset_to_type_id.find(offset); it == _offset_to_type_id.end()){\
    throw std::runtime_error("unknown offset");\
  } else {\
    if (!reflect::_is_type<T>(it->second)){\
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
   return reflect::_is_type<T>(it->second);\
  }\
 }\
 static const vector<reflect::FieldInfo>& get_field_info_vec() {return _field_info_vec;}\
 inline static const auto _unique_var = ((reflect::global_serializer[reflect::_type_id<type>] = [](void* p){\
    return reflect::reflect_default_serialize(*static_cast<type*>(p));\
   }),0);

// 非继承的using_reflect，声明了字段名到偏移量、字段名到类型id、偏移量到类型id三种map，以及一个保存所有字段信息的vector
#define using_reflect(type) \
 protected: \
 using _reflect_type = type;\
 friend std::string reflect::reflect_default_serialize<type>(const type &);\
 inline static std::unordered_map<std::string_view, size_t> _name_to_offset;\
 inline static std::unordered_map<std::string_view, reflect::_TypeID> _name_to_type_id;\
 inline static std::unordered_map<size_t, reflect::_TypeID> _offset_to_type_id;\
 inline static std::vector<reflect::FieldInfo> _field_info_vec;\
 inline static reflect::Serializer<type> _serializer;\
 using_reflect_common(type);

// 声明某个字段为反射字段，可以进行初始化，但需要多一个逗号
// 然后将该字段注册到以上的三种map和一个vector中
#define reflect_field(type, name, ...)\
 public: type name __VA_ARGS__;\
 private:\
 inline static const auto _##name##_offset = reflect::_member_pointer_to_offset(&_reflect_type::name);\
 inline static const auto _##name##_type_id = reflect::_Identifier<type>::ID;\
 inline static const auto _unique_var = (_name_to_offset[#name] = _##name##_offset, 0);\
 inline static const auto _unique_var = (_name_to_type_id[#name] = _##name##_type_id, 0);\
 inline static const auto _unique_var = (_offset_to_type_id[_##name##_offset] = _##name##_type_id, 0);\
 inline static const auto _unique_var = (_field_info_vec.emplace_back(reflect::FieldInfo{#name, _##name##_type_id, _##name##_offset}), 0);\
 inline static const auto _unique_var = (_serializer.register_basic_func<type>(), 0);

#define reflect_field_self_define(type, name, ...)\
 public: type name __VA_ARGS__;\
 private:\
 inline static const auto _##name##_offset = reflect::_member_pointer_to_offset(&_reflect_type::name);\
 inline static const auto _##name##_type_id = reflect::_Identifier<type>::ID;\
 inline static const auto _unique_var = (_name_to_offset[#name] = _##name##_offset, 0);\
 inline static const auto _unique_var = (_name_to_type_id[#name] = _##name##_type_id, 0);\
 inline static const auto _unique_var = (_offset_to_type_id[_##name##_offset] = _##name##_type_id, 0);\
 inline static const auto _unique_var = (_field_info_vec.emplace_back(reflect::FieldInfo{#name, _##name##_type_id, _##name##_offset}), 0);

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

#define def_get_base_serializer() \
 private:\
template<typename T, typename ...Ts>\
static auto get_serializer() -> auto {\
  return std::make_tuple(\
      std::ref(T::_serializer), std::ref(Ts::_serializer)...\
  );\
}

template<typename V, typename T, typename U>
void extract_tuple(T &t, const U &u) {
  for (const auto &m : u) {
    t[m.first] = [f = m.second](const V &v, size_t off) { return f(v, off); };
  }
}

#define def_create_serializer_handler() \
 private:\
template<typename T, typename ...Bs>\
static std::unordered_map<reflect::_TypeID, reflect::serialize_func<T>> create_serializer_handler() {\
  std::unordered_map<reflect::_TypeID, reflect::serialize_func<T>> result;\
  std::apply([&result](auto&& ...args) mutable {\
    (extract_tuple<T>(result, args.handler), ...);\
  },get_serializer<Bs...>());\
  return result;\
}\

#define using_reflect_inherit(type, ...)\
 private: \
 using _reflect_type = type;\
 def_get_base_serializer();\
 def_create_serializer_handler();\
 friend std::string reflect::reflect_default_serialize<type>(const type &);\
 def_get_base_map_func();\
 inline static std::unordered_map<std::string_view, size_t> _name_to_offset = create_inherited_name_to_offset(__VA_ARGS__);\
 inline static std::unordered_map<std::string_view, reflect::_TypeID> _name_to_type_id = create_inherited_map(_name_to_type_id, __VA_ARGS__);\
 inline static std::unordered_map<size_t, reflect::_TypeID> _offset_to_type_id = create_inherited_offset_to_type_id( __VA_ARGS__);\
 inline static std::vector<reflect::FieldInfo> _field_info_vec = create_inherited_field_info_vec(__VA_ARGS__);\
 inline static reflect::Serializer<type> _serializer{.handler = create_serializer_handler<type, __VA_ARGS__>()};\
 using_reflect_common(type)

#define reflect_comma ,

#endif
