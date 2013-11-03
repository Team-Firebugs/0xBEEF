#ifndef __BEEF_H__
#define __BEEF_H__

#define XS_STATE(type, x) \
    INT2PTR(type, SvROK(x) ? SvIV(SvRV(x)) : SvIV(x))

#define XS_STRUCT2OBJ(sv, class, obj) \
    if (obj == NULL) { \
        sv_setsv(sv, &PL_sv_undef); \
    } else { \
        sv_setref_pv(sv, class, (void *) obj); \
    }
#define BEEF_ALLOC(s,cast) Newx(s,1,cast)
#define BEEF_FREE(s) Safefree(s)
#define BEEF_DIE(fmt,arg...) die(fmt,##arg)

#include <beef.h>
typedef struct shared_pool BEEFContext;

#endif
