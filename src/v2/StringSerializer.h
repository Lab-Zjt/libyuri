/**
  * @file   StringSerializer.h 
  * @author sora
  * @date   2020/4/5
  */

#ifndef LIBYURI_SRC_V2_STRINGSERIALIZER_H_
#define LIBYURI_SRC_V2_STRINGSERIALIZER_H_

#include "Serializer.h"
#include "TypeTraits/RangeTrait.h"
#include "TypeTraits/OutputStreamOverloadTraits.h"
#include "SerializeFunctionMap.h"
#include <list>
#include <memory>

namespace yuri {

  template<typename T>
  class Reflectable;

  class StringOutput : public OutputBase<StringOutput> {
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
        ss << '&';
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
      ss << '<';
      output(pair.first);
      ss << ':';
      output(pair.second);
      ss << '>';
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
        ss << info.name << '=';
        SerializeFunctionMap<StringOutput>::getInstance().getSerializeFunction(info.id)(this,
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

    template<typename T>
    std::string serialize(const T &object) {
      output(object);
      return ss.str();
    }
  };

}

#endif //LIBYURI_SRC_V2_STRINGSERIALIZER_H_
