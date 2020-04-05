/**
  * @file   ReflectInfo.h 
  * @author sora
  * @date   2020/4/5
  */

#ifndef LIBYURI_SRC_V2_REFLECTINFO_H_
#define LIBYURI_SRC_V2_REFLECTINFO_H_

#include "FieldInfo.h"
namespace yuri {
  template<typename T>
  struct ReflectInfo {
    std::vector<FieldInfo> fieldInfoList{};
    std::unordered_map<std::string_view, TypeId> nameToTypeId{};
    std::unordered_map<size_t, TypeId> offsetToTypeId{};
    std::unordered_map<std::string_view, size_t> nameToOffset{};
    static ReflectInfo<T> &getInstance() {
      static ReflectInfo<T> ins{};
      return ins;
    }
  };
}

#endif //LIBYURI_SRC_V2_REFLECTINFO_H_
