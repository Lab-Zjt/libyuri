/**
  * @file   SerializeFunctionMap.h 
  * @author sora
  * @date   2020/4/5
  */

#ifndef LIBYURI_SRC_V2_SERIALIZEFUNCTIONMAP_H_
#define LIBYURI_SRC_V2_SERIALIZEFUNCTIONMAP_H_
#include "TypeId.h"
namespace yuri {
  template<typename Output>
  class SerializeFunctionMap {
    using SerializeFunction = void (*)(OutputBase<Output> *output, const void *object);
   private:
    std::unordered_map<TypeId, SerializeFunction> functions;
    template<typename T>
    static void output(OutputBase<Output> *output, const void *object) {
      output->output(*static_cast<const T *>(object));
    }
   public:
    template<typename T>
    void registerSerializeFunction() {
      functions[typeId<T>] = &SerializeFunctionMap<Output>::output<T>;
    }
    SerializeFunction getSerializeFunction(TypeId id) {
      return functions[id];
    }
    static SerializeFunctionMap<Output> &getInstance() {
      static SerializeFunctionMap<Output> ins;
      return ins;
    }
  };
}

#endif //LIBYURI_SRC_V2_SERIALIZEFUNCTIONMAP_H_
