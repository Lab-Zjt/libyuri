#ifndef LIBYURI__DESERIALIZER_H_
#define LIBYURI__DESERIALIZER_H_
#include "reflect.h"

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

    // 反序列化类
    class Deserializer {
     public:
      // 管理type_id到parse_func的映射
      // 初始化时注册用于解析unknown_field的函数
      inline static std::unordered_map<TypeID, parse_func> handler{
          {type_id < bool > , deserialize<bool>},
          {type_id < std::string > , deserialize<std::string>},
          {type_id < double > , deserialize<double>}
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
        while ((str[off] != '"' || (str[off] == '"' && str[off - 1] == '\\')) && off < str.size())off++;
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
            if (!parse(type_id<std::string>, str, off, &key)) {
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
          if (!parse(type_id<std::string>, str, off, &key)) {
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
          if (!parse(type_id<bool>, str, off, &b)) {
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
          if (!parse(type_id<double>, str, off, &d)) {
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
