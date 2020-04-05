/**
  * @file   UniqueVariable.h 
  * @author sora
  * @date   2020/4/5
  */

#ifndef LIBYURI_SRC_V2_UNIQUEVARIABLE_H_
#define LIBYURI_SRC_V2_UNIQUEVARIABLE_H_

#define __counter_meta(val) __counter_##val
#define _counter_meta(val) __counter_meta(val)
#define _unique_var _counter_meta(__COUNTER__)

#endif //LIBYURI_SRC_V2_UNIQUEVARIABLE_H_
