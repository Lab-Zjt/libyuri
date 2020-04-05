/**
  * @file   Serializer.h 
  * @author sora
  * @date   2020/4/5
  */

#ifndef LIBYURI_SRC_V2_SERIALIZER_H_
#define LIBYURI_SRC_V2_SERIALIZER_H_
#include "TypesDef.h"
namespace yuri {

  template<typename CRTP>
  class OutputBase {
   public:
    template<typename T>
    void output(const T &object) {
      (*static_cast<CRTP *>(this)).output(object);
    }
  };

  template<typename CRTP, typename Output, typename Result = std::string>
  class Serializer {
   private:
    Output output;
   private:
    CRTP &toSubclass() { return *static_cast<CRTP *>(this); }
    const CRTP &toSubclass() const { return *static_cast<CRTP *>(this); }
   public:
    template<typename ...Args>
    explicit Serializer(Args &&...args): output(std::forward<Args>(args)...) {}
   public:
    template<typename T>
    Result serialize(const T &object) const { return toSubclass().serialize(object); }

  };
}

#endif //LIBYURI_SRC_V2_SERIALIZER_H_
