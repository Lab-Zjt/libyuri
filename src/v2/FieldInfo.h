/**
  * @file   FieldInfo.h 
  * @author sora
  * @date   2020/4/5
  */

#ifndef LIBYURI_SRC_V2_FIELDINFO_H_
#define LIBYURI_SRC_V2_FIELDINFO_H_

#include "TypeId.h"

namespace yuri {
  struct FieldInfo {
    std::string_view name;
    TypeId id;
    size_t offset;
    template<typename T>
    bool isTypeOf() const { return isTypeIdOf<T>(id); }
    const void *getFieldPtr(const void *ptr) const { return reinterpret_cast<const char *>(ptr) + offset; }
  };
}

#endif //LIBYURI_SRC_V2_FIELDINFO_H_
