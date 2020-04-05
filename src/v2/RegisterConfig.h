/**
  * @file   RegisterConfig.h 
  * @author sora
  * @date   2020/4/5
  */

#ifndef LIBYURI_SRC_V2_REGISTERCONFIG_H_
#define LIBYURI_SRC_V2_REGISTERCONFIG_H_
#include "JsonSerializer.h"
#include "StringSerializer.h"
namespace yuri {
  template<typename T>
  inline void outputRegister() {
    SerializeFunctionMap<StringOutput>::getInstance().registerSerializeFunction<T>();
    SerializeFunctionMap<JsonOutput>::getInstance().registerSerializeFunction<T>();
  }
}
#endif //LIBYURI_SRC_V2_REGISTERCONFIG_H_
