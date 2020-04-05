/**
  * @file   TypeCast.h 
  * @author sora
  * @date   2020/4/5
  */

#ifndef LIBYURI_SRC_V2_TYPECAST_H_
#define LIBYURI_SRC_V2_TYPECAST_H_

namespace yuri {
  template<typename To, typename From>
  inline To forceCast(From from) {
    union {
      From f;
      To t;
    } u{from};
    return u.t;
  }

  template<typename M>
  inline size_t memberToOffset(M m) {
    return forceCast<size_t>(m);
  }

  template<typename T, typename M>
  inline M T::*offsetToMember(size_t offset) {
    return forceCast<M T::*>(offset);
  }

  template<typename T, typename M>
  M memberTypeExtract(M T::* member);

  template<typename Mp>
  using memberTypeOfMemberPointer = decltype(memberTypeExtract(std::declval<Mp>()));
}

#endif //LIBYURI_SRC_V2_TYPECAST_H_
