#ifndef MOXITREL_COGO_COGO_H_
#define MOXITREL_COGO_COGO_H_
#ifdef __cplusplus
extern "C" {
#endif

#include "co_fs.h"

#define co_begin    CO_BEGIN
#define co_end      CO_END
#define co_yield    CO_YIELD
#define co_return   CO_RETURN
#define co_this     CO_THIS
#define co_state    CO_STATE
#define co_declare  CO_DECLARE
#define co_define   CO_DEFINE
#define co_make     CO_MAKE
#define co_await    CO_AWAIT
#define co_start    CO_START

#define co_chan_make    CO_CHAN_MAKE
#define co_chan_write   CO_CHAN_WRITE
#define co_chan_read    CO_CHAN_READ

#ifdef __cplusplus
}
#endif
#endif // MOXITREL_COGO_COGO_H_
