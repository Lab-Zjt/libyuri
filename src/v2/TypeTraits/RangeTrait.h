/**
  * @file   RangeTrait.h 
  * @author sora
  * @date   2020/4/5
  */

#ifndef LIBYURI_SRC_V2_TYPETRAITS_RANGETRAIT_H_
#define LIBYURI_SRC_V2_TYPETRAITS_RANGETRAIT_H_
#include <experimental/type_traits>
namespace yuri {
  template<typename T>
  using begin_t = decltype(std::declval<T>().begin());

  template<typename T>
  using end_t = decltype(std::declval<T>().end());

  template<typename T>
  constexpr bool
      is_range_v = std::experimental::is_detected_v<begin_t, T> && std::experimental::is_detected_v<end_t, T>;
}

#endif //LIBYURI_SRC_V2_TYPETRAITS_RANGETRAIT_H_
