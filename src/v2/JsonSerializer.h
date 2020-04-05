/**
  * @file   JsonSerializer.h 
  * @author sora
  * @date   2020/4/5
  */

#ifndef LIBYURI_SRC_V2_JSONSERIALIZER_H_
#define LIBYURI_SRC_V2_JSONSERIALIZER_H_
#include <memory>
#include "Serializer.h"
#include "TypeTraits/RangeTrait.h"
#include "SerializeFunctionMap.h"
#include "TypeTraits/OutputStreamOverloadTraits.h"

namespace yuri {

  template<typename T>
  class Reflectable;

  class JsonOutput : public OutputBase<JsonOutput, std::string> {
    std::stringstream ss;
   private:
    template<typename T>
    void stringOutput(const T &str) {
      ss << '"' << str << '"';
    }
    template<typename T>
    void listOutput(const T &list) {
      ss << '[';
      bool atLeastWriteOnce = false;
      for (auto &&elem : list) {
        atLeastWriteOnce = true;
        output(elem);
        ss << ',';
      }
      if (atLeastWriteOnce)ss.seekp(-1, std::ios::cur);
      ss << ']';
    }
   public:
    void output(std::string_view str) {
      stringOutput(str);
    }

    void output(const char *str) {
      stringOutput(str);
    }
    void output(const std::string &str) {
      stringOutput(str);
    }

    template<typename T>
    void output(T *const ptr) {
      if (ptr == nullptr) {
        ss << "null";
      } else {
        output(*ptr);
      }
    }

    template<typename T>
    void output(const std::shared_ptr<T> &ptr) {
      output(ptr.get());
    }

    template<typename T>
    void output(const std::unique_ptr<T> &ptr) {
      output(ptr.get());
    }

    template<typename K, typename V>
    void output(const std::pair<K, V> &pair) {
      ss << R"({"1":)";
      output(pair.first);
      ss << R"(,"2":)";
      output(pair.second);
      ss << '}';
    }

    template<typename T, typename Enable = std::enable_if_t<is_range_v<T>>>
    void output(const T &list) {
      listOutput(list);
    }

    template<typename T>
    void output(const Reflectable<T> &reflectable) {
      ss << '{';
      bool atLeastWriteOnce = false;
      for (auto &&info : reflectable.getFieldInfoList()) {
        if (info.offset >= sizeof(T))break;
        atLeastWriteOnce = true;
        stringOutput(info.name);
        ss << ':';
        SerializeFunctionMap<JsonOutput>::getInstance().getSerializeFunction(info.id)(this,
                                                                                      info.getFieldPtr(&reflectable));
        ss << ',';
      }
      if (atLeastWriteOnce) ss.seekp(-1, std::ios::cur);
      ss << '}';
    }

    template<typename T, typename Enable = std::enable_if_t<is_outputstream_overload<T>>, typename _Place = void>
    void output(const T &object) {
      ss << object;
    }

    std::string toResult() {
      return ss.str();
    }
  };

  using JsonSerializer = Serializer<JsonOutput>;
}

#endif //LIBYURI_SRC_V2_JSONSERIALIZER_H_
