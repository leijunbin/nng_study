// nng内部实现，通用定义等。
// 所有内部模块都会包含此文件，避免总是出现头文件未引用
// 所导致的未定义错误。
// 库私有符号以nni_前缀开头，nng_开头可外部使用，应在
// nng.h头文件中找到。
#ifndef CORE_NNG_IMPL_H
#define CORE_NNG_IMPL_H

#include "nng/nng.h"

#include "core/defs.h"

#include "core/platform.h"

#include "list.h"
#include "core/panic.h"
#include "core/thread.h"

#endif // CORE_NNG_IMPL_H