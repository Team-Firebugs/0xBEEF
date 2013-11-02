#include "EXTERN.h"
#include "perl.h"
#include "XSUB.h"

#include "ppport.h"

#include "BEEF.h"

MODULE = BEEF PACKAGE = BEEF		

PROTOTYPES: DISABLED

BEEFContext*
BEEF::new(key)
    int key
    PREINIT:
        BEEFContext* ctx;
    CODE:
        ctx = shared_pool_init(key);
        RETVAL=ctx;
    OUTPUT:
        RETVAL

void
DESTROY(BEEFContext* ctx)
    CODE:
        shared_pool_destroy(ctx);

SV*
find(BEEFContext* ctx, SV *key)
    CODE:
        shared_pool_lock(ctx);
        struct result r;
        if (t_find_locked(ctx->pool,SvPVX(key),&r) == 0) {
            RETVAL = newSVpv(r.blob,r.size); 
        } else {
            RETVAL = &PL_sv_undef;
        }
        shared_pool_unlock(ctx,0);
    OUTPUT:
        RETVAL

void
copy_locally(BEEFContext *ctx)
    CODE:
        shared_pool_copy_locally(ctx);

SV*
find_locally(BEEFContext* ctx, SV *key)
    CODE:
        if (!ctx->copy)
            die("you need to do $obj->copy_locally() before you can do find_locally()");

        struct result r;
        if (t_find_locked(ctx->copy,SvPVX(key),&r) == 0) {
            RETVAL = newSVpv(r.blob,r.size); 
        } else {
            RETVAL = &PL_sv_undef;
        }
    OUTPUT:
        RETVAL

int
store(BEEFContext *ctx, SV *key, SV* value)
    CODE:
        STRLEN len;
        char *ptr;
        ptr = SvPV(value, len);
        int rc = t_add(ctx,SvPVX(key),ptr,len);
        if (rc < 0)
            die("unable to store item");
        RETVAL = rc;
    OUTPUT:
        RETVAL 

void
reset(BEEFContext *ctx)
    CODE:
        shared_pool_reset(ctx);

void
overwrite(BEEFContext *ctx, SV *blob)
    CODE:
        shared_pool_lock(ctx);
        STRLEN len;
        char *ptr;
        ptr = SvPV(blob, len);
        if (len > sizeof(*ctx->pool))
            die("blob size mismatch: expected %zu got %zu",sizeof(*ctx->pool),len);

        memcpy(ctx->pool,ptr,len);
        shared_pool_unlock(ctx,0);

SV*
export(BEEFContext *ctx)
    CODE:
        shared_pool_lock(ctx);
        RETVAL = newSVpvn((char *) ctx->pool,POOL_SIZE(ctx->pool)); 
        shared_pool_unlock(ctx,0);
    OUTPUT:
        RETVAL


SV *
info(BEEFContext *ctx)
    CODE:
        RETVAL = newSVpvf("(export size: %d) radix can hold up to %d items with %zu size per item (%d bytes total), so far it holds: %d items; data blob can hold %d, so far it holds: %d",
                    POOL_SIZE(ctx->pool),
                    TREE_BLOB_SIZE/sizeof(struct item),
                    sizeof(struct item),
                    TREE_BLOB_SIZE,
                    ctx->pool->tree_pos / sizeof(struct item),
                    DATA_BLOB_SIZE,
                    DATA_BLOB_SIZE - ctx->pool->data_pos);
    OUTPUT:
        RETVAL
