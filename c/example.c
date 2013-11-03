#include "../beef.h"
int main(void) {
    struct shared_pool *sp = shared_pool_init(0xbeef);
    if (t_add(sp,"aaa",(uint8_t *) "bbb",4) != 0)
        SAYX("failed to store");
    struct result r;
    shared_pool_lock(sp);
    if (t_find_locked(sp->pool,"aaa",&r) == 0) {
        D("found %s",r.blob);
    }
    shared_pool_unlock(sp);
    shared_pool_destroy(sp);
    return 0;
}
