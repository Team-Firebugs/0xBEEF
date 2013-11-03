#include "ruby.h"
#define BEEF_DIE(fmt,arg...) rb_raise(rb_eArgError,fmt,##arg) 

#include "beef.h"
 
static void   wrap_BEEF_free(struct shared_pool *);
static VALUE  wrap_BEEF_init(VALUE, VALUE);


static void wrap_BEEF_free(struct shared_pool* sp) {
  shared_pool_destroy(sp);
}
 
static VALUE BEEF_init(VALUE class, VALUE _shmid) {
    int shmid = NUM2INT(_shmid);
    struct shared_pool *p = shared_pool_init(shmid);
    VALUE sp = Data_Wrap_Struct(class, 0, wrap_BEEF_free, p);
    return sp;
}

inline struct shared_pool *shared_pool(VALUE obj) {
    struct shared_pool *sp;
    Data_Get_Struct(obj,struct shared_pool, sp);
    return sp;
}
static VALUE BEEF_store(VALUE self, VALUE key, VALUE value) {
    struct shared_pool *sp = shared_pool(self);
    if (t_add(sp,StringValueCStr(key),RSTRING_PTR(value),RSTRING_LEN(value)) != 0)
        rb_raise(rb_eArgError,"failed to store");
    return value;
}

static VALUE BEEF_find(VALUE self, VALUE key) {
    VALUE ret = Qnil;
    struct shared_pool *sp = shared_pool(self);
    shared_pool_lock(sp);
    struct result r;
    if (t_find_locked(sp->pool,StringValueCStr(key),&r) == 0)
        ret = rb_str_new(r.blob,r.size);
    shared_pool_unlock(sp);
    return ret;
}

static VALUE BEEF_reset(VALUE self) {
    struct shared_pool *sp = shared_pool(self);
    shared_pool_reset(sp);
    return Qtrue;
}

static VALUE BEEF_export(VALUE self) {
    struct shared_pool *sp = shared_pool(self);
    shared_pool_lock(sp);
    VALUE ret = rb_str_new((char *) sp->pool,POOL_SIZE(sp->pool));
    shared_pool_unlock(sp);
    return ret;
}
static VALUE BEEF_overwrite(VALUE self, VALUE blob) {
    struct shared_pool *sp = shared_pool(self);
    size_t len = RSTRING_LEN(blob);
    if (len > sizeof(*sp->pool))
        rb_raise(rb_eArgError,"blob size mismatch: expected %zu got %zu",sizeof(*sp->pool),len);

    shared_pool_lock(sp);
    memcpy(sp->pool,RSTRING_PTR(blob),len);
    shared_pool_unlock(sp);
    return Qtrue;
}

void Init_beef() {
  VALUE c = rb_define_class("BEEF", rb_cObject);
  rb_define_singleton_method(c, "create", RUBY_METHOD_FUNC(BEEF_init), 1);
  rb_define_method(c, "find", BEEF_find, 1);
  rb_define_method(c, "store", BEEF_store, 2);
  rb_define_method(c, "export", BEEF_export, 0);
  rb_define_method(c, "overwrite", BEEF_overwrite, 1);
}
