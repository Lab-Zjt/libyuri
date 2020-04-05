/**
  * @file   Serializer.h 
  * @author sora
  * @date   2020/4/5
  */

#ifndef LIBYURI_SRC_V2_SERIALIZER_H_
#define LIBYURI_SRC_V2_SERIALIZER_H_
#include "TypesDef.h"
namespace yuri {

  template<typename CRTP, typename Result>
  class OutputBase {
   public:
    template<typename T>
    void output(const T &object) {
      (*static_cast<CRTP *>(this)).output(object);
    }
    Result toResult() { (*static_cast<CRTP *>(this)).toResult(); }
  };

  template<typename Output, typename Result = std::string>
  class Serializer {
   protected:
    Output output;
   public:
    template<typename ...Args>
    explicit Serializer(Args &&...args): output(std::forward<Args>(args)...) {}
   public:
    template<typename T>
    Result serialize(const T &object) {
      this->output.output(object);
      return this->output.toResult();
    }

  };
}

#endif //LIBYURI_SRC_V2_SERIALIZER_H_
