/**
  * @file   TypeId.h 
  * @author sora
  * @date   2020/4/5
  */

#ifndef LIBYURI_SRC_V2_TYPEID_H_
#define LIBYURI_SRC_V2_TYPEID_H_

namespace yuri {

  using TypeId = const void*;

  template<typename T>
  struct TypeIdGenerator {
    static void id() {}
    inline static const auto Id = reinterpret_cast<TypeId>(&TypeIdGenerator<T>::id);
  };

  template <typename T>
  inline TypeId typeId = TypeIdGenerator<T>::Id;

  template <typename T>
  inline bool isTypeIdOf(TypeId id) {
    return id == typeId<T>;
  }

}

#endif //LIBYURI_SRC_V2_TYPEID_H_
