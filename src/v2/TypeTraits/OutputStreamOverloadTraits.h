/**
  * @file   OutputStreamOverloadTraits.h 
  * @author sora
  * @date   2020/4/5
  */

#ifndef LIBYURI_SRC_V2_TYPETRAITS_OUTPUTSTREAMOVERLOADTRAITS_H_
#define LIBYURI_SRC_V2_TYPETRAITS_OUTPUTSTREAMOVERLOADTRAITS_H_

namespace yuri {
  template<typename T>
  using outputstream_t = decltype(std::declval<std::ostream>() << std::declval<T>());

  template<typename T>
  constexpr bool is_outputstream_overload = std::experimental::is_detected_v<outputstream_t, T>;
}

#endif //LIBYURI_SRC_V2_TYPETRAITS_OUTPUTSTREAMOVERLOADTRAITS_H_
