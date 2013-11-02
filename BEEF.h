#ifndef __FOO_H__
#define __FOO_H__

#define XS_STATE(type, x) \
    INT2PTR(type, SvROK(x) ? SvIV(SvRV(x)) : SvIV(x))

#define XS_STRUCT2OBJ(sv, class, obj) \
    if (obj == NULL) { \
        sv_setsv(sv, &PL_sv_undef); \
    } else { \
        sv_setref_pv(sv, class, (void *) obj); \
    }

#include <time.h>
#include <stdio.h>
#include <stdint.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/sem.h>
#include <sys/shm.h>

#define FORMAT(fmt,arg...) fmt " [%s():%s:%d @ %u]\n",##arg,__func__,__FILE__,__LINE__,(unsigned int) time(NULL)
#define E(fmt,arg...) fprintf(stderr,FORMAT(fmt,##arg))
#define D(fmt,arg...) printf(FORMAT(fmt,##arg))
#define SAYX(rc,fmt,arg...) do {    \
        die(fmt,##arg);             \
} while(0)

#define SAYPX(fmt,arg...) SAYX(EXIT_FAILURE,fmt " { %s }",##arg,errno ? strerror(errno) : "undefined error");
#define TREE_BLOB_SIZE 1048576  // 1mb
#define DATA_BLOB_SIZE 10485760 // 10mb
#define ITEM(pool,off) ((struct item *) (&((pool)->tree_blob[1 + (off)])))
#define DATA(pool,item) ((char *) (&((pool)->data_blob[ ((item)->data.off)])))
#define POOL_SIZE(_pool) (sizeof(struct pool) - DATA_BLOB_SIZE + (_pool)->data_pos)

#define IN_PATH  1
#define IN_VALUE 2
static struct sembuf sem_lock = { 0, -1, 0 };
static struct sembuf sem_unlock = { 0, 1, 0 };

struct item {
    uint32_t branch[26];
    struct data {
        uint32_t off;
        uint32_t len;
    } data;
    uint8_t used;
} __attribute__((aligned));

struct pool {
    struct item root;
    uint32_t tree_pos;
    uint32_t data_pos;
    uint8_t tree_blob[TREE_BLOB_SIZE]; 
    uint8_t data_blob[DATA_BLOB_SIZE];
} __attribute__((packed));

struct shared_pool {
    int shm_id;
    int sem_id;
    int key;
    struct pool *pool;
    struct pool *copy;
};

struct result {
    uint8_t *blob;
    uint32_t size;
};

inline uint8_t symbol(char s) {
    if (s < 'a' || s > 'z')
        SAYX(EXIT_FAILURE,"key can contain only characters between 'a' .. 'z', '%c' is invalid",s);
    return s - 'a';
}

static inline void shared_pool_lock(struct shared_pool *sp) {
    if (semop(sp->sem_id,&sem_lock, 1) == -1)
        SAYPX("semop");
}

static inline int shared_pool_unlock(struct shared_pool *sp, int err) {
    if (semop(sp->sem_id,&sem_unlock, 1) == -1)
        SAYPX("semop");

    return err;

}

static int t_add(struct shared_pool *sp,char *key, const uint8_t *p, size_t len) {
    if (len <= 0)
        return -ENOMSG;
    shared_pool_lock(sp);

    uint8_t current;
    struct item *item = NULL;
    struct pool *pool = sp->pool;
    struct item *head = &pool->root;
    uint32_t off = 0;
    while ((current = *key++)) {
        current = symbol(current);
        off = head->branch[current];
        if (off == 0) {
            if (pool->tree_pos + sizeof(*item) >= TREE_BLOB_SIZE - 1)
                return shared_pool_unlock(sp,-ENOMEM);
            off = 1 + pool->tree_pos;
            pool->tree_pos += sizeof(*item);
        }

        item = ITEM(pool,off);
        if (!item->used) {
            item->used = IN_PATH;
            head->branch[current] = off;
        }
        head = item;
    }
    if (!item)
        SAYX(EXIT_FAILURE,"unable to store");
    
    if (item->data.off == 0 || item->data.len < len) {
        if (pool->data_pos + len >= DATA_BLOB_SIZE)
            return shared_pool_unlock(sp,-ENOMEM);
        item->data.off = pool->data_pos;
        pool->data_pos += len;
    }

    item->data.len = len;
    item->used = IN_VALUE;
    memcpy(DATA(pool,item),p,len);
    return shared_pool_unlock(sp,0);
}

inline static int t_find_locked(struct pool *pool, char *key, struct result *r) {
    struct item *head = &pool->root;
    struct item *item = NULL;
    uint8_t current;
    uint32_t off = 0;
    while ((current = *key++)) {
        off = head->branch[symbol(current)];
        if (off == 0)
            return -ENOKEY;

        item = ITEM(pool,off);
        head = item;
    }
    r->blob = DATA(pool,item);
    r->size = item->data.len;
    return 0;
}

static struct shared_pool *shared_pool_init(int key) {
    if (key % 2 == 0)
        SAYX(EXIT_FAILURE,"key must be odd number, key + 1 is used for the semaphore's key");

    struct shared_pool *sp;
    Newx(sp,1,struct shared_pool);
    sp->pool = NULL;
    sp->copy = NULL;
    struct pool *p;
    int flags = 0666 | IPC_CREAT;
    if ((sp->shm_id = shmget(key, sizeof(*p), flags)) < 0)
        SAYPX("shmget: 0x%x",key);

    if ((sp->sem_id = semget(key + 1, 1, flags)) < 0)
        SAYPX("shmget: 0x%x",key + 1);

    if ((sp->pool = shmat(sp->shm_id, NULL, 0)) < 0 )
        SAYPX("shmat");

    sp->key = key;
    // D("shmid: %d(%x) semid: %d(%x)",sp->shm_id,key,sp->sem_id,key + 1);
    shared_pool_unlock(sp,0);
    return sp;
}

static struct shared_pool *shared_pool_reset(struct shared_pool *sp) {
    memset(sp->pool,0,sizeof(*sp->pool));
}

static void shared_pool_destroy(struct shared_pool *sp) {
    if (semctl(sp->sem_id, 1, IPC_RMID ) != 0) 
        SAYPX("IPC_RMID failed");

    if (shmdt(sp->pool) != 0)
        SAYPX("detach failed");

    if (shmctl(sp->shm_id, IPC_RMID, NULL) != 0) 
        SAYPX("IPC_RMID failed");

    if (sp->copy)
        Safefree(sp->copy);
    Safefree(sp);
}
void shared_pool_copy_locally(struct shared_pool *sp) {
    shared_pool_lock(sp);
    if (sp->copy)
        Safefree(sp->copy);

    Newx(sp->copy, 1, struct pool);
    memcpy(sp->copy,sp->pool, POOL_SIZE(sp->pool));
    shared_pool_unlock(sp,0);
}
typedef struct shared_pool BEEFContext;

#endif
