#ifndef LIBYURI__SERIALIZER_H_
#define LIBYURI__SERIALIZER_H_

#include "reflect.h"

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

#endif
