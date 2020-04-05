/**
  * @file   Reflectable.h 
  * @author sora
  * @date   2020/4/5
  */

#ifndef LIBYURI_SRC_V2_REFLECTABLE_H_
#define LIBYURI_SRC_V2_REFLECTABLE_H_

#include <vector>
#include <unordered_map>
#include <sstream>
#include "ReflectInfo.h"
#include "FieldInfo.h"
#include "TypeCast.h"
#include "UniqueVariable.h"
#include "StringSerializer.h"
namespace yuri {

  template<typename T>
  inline void outputRegister() {
    SerializeFunctionMap<StringOutput>::getInstance().registerSerializeFunction<T>();
  }

  template<typename T>
  class Reflectable {
    // Type Info Storage
   private:
    static ReflectInfo<T> &reflectInfo() {
      return ReflectInfo<T>::getInstance();
    }
    T &toSubclass() { return *static_cast<T *>(this); }
    const T &toSubclass() const { return *static_cast<T *>(this); }
    // Call by subclass
   protected:
    using baseType = Reflectable<T>;
    using selfType = T;
    template<typename Mp>
    static bool addField(std::string_view name, Mp member) noexcept {
      outputRegister<T>();
      using memberType = memberTypeOfMemberPointer<Mp>;
      auto offset = memberToOffset(member);
      auto id = typeId<memberType>;
      reflectInfo().fieldInfoList.emplace_back(FieldInfo{name, id, offset});
      reflectInfo().nameToTypeId[name] = id;
      reflectInfo().offsetToTypeId[offset] = id;
      reflectInfo().nameToOffset[name] = offset;
      outputRegister<memberType>();
      return true;
    }
   public:
    Reflectable() = default;
    const auto &getFieldInfoList() const { return reflectInfo().fieldInfoList; }
    template<typename Member>
    Member &getFieldByName(std::string_view name) {
      if (auto it = reflectInfo().nameToOffset.find(name); it == reflectInfo().nameToOffset.end()) {
        throw std::runtime_error("access unknown field");
      } else {
        return getFieldByOffset<Member>(it->second);
      }
    }
    template<typename Member>
    Member &getFieldByOffset(size_t offset) {
      return toSubclass().*(offsetToMember<T, Member>(offset));
    }
    bool hasField(std::string_view name) const {
      return reflectInfo().nameToTypeId.find(name) != reflectInfo().nameToTypeId.end();
    }
    std::string toString() const {
      return StringOutput().serialize(*this);
    }
  };

}

#define ReflectField(name, ...) name __VA_ARGS__ ; \
 private: inline static bool _unique_var = (baseType::addField(#name, &selfType::name)); public:

#endif //LIBYURI_SRC_V2_REFLECTABLE_H_
