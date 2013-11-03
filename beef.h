#include <time.h>
#include <stdio.h>
#include <stdint.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/sem.h>
#include <sys/shm.h>

#ifndef BEEF_DIE
#define BEEF_DIE(fmt,arg...) \
do {                         \
    E(fmt,##arg);            \
    exit(EXIT_FAILURE);      \
} while(0)
#endif

#define FORMAT(fmt,arg...) fmt " [%s():%s:%d @ %u]\n",##arg,__func__,__FILE__,__LINE__,(unsigned int) time(NULL)
#define E(fmt,arg...) fprintf(stderr,FORMAT(fmt,##arg))
#define D(fmt,arg...) printf(FORMAT(fmt,##arg))
#define SAYX(fmt,arg...) do {         \
        BEEF_DIE(fmt,##arg);          \
} while(0)

#define SAYPX(fmt,arg...) SAYX(fmt " { %s }",##arg,errno ? strerror(errno) : "undefined error");
#define TREE_BLOB_SIZE 1048576  // 1mb
#define DATA_BLOB_SIZE 10485760 // 10mb
#define ITEM(pool,off) ((struct item *) (&((pool)->tree_blob[1 + (off)])))
#define DATA(pool,item) ((uint8_t *) (&((pool)->data_blob[ ((item)->data.off)])))
#define POOL_SIZE(_pool) (sizeof(struct pool) - DATA_BLOB_SIZE + (_pool)->data_pos)

#define IN_PATH  1
#define IN_VALUE 2

#ifndef BEEF_ALLOC
#define BEEF_ALLOC(s,cast)               \
do {                                     \
    s = (cast *) x_malloc(sizeof(cast)); \
} while(0)

void *x_malloc(size_t s) {
    void *x = malloc(s);
    if (!x)
        SAYPX("malloc for %zu failed",s);
    return x;
}

#endif

#ifndef BEEF_FREE
#define BEEF_FREE(s) x_free(s)
void x_free(void *x) {
    if (x != NULL)
        free(x);
}
#endif

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

inline int8_t symbol(char s) {
    if (s < 'a' || s > 'z')
        return -1;
    return s - 'a';
}

static inline void shared_pool_lock(struct shared_pool *sp) {
    if (semop(sp->sem_id,&sem_lock, 1) == -1)
        SAYPX("semop");
}

static inline void shared_pool_unlock(struct shared_pool *sp) {
    if (semop(sp->sem_id,&sem_unlock, 1) == -1)
        SAYPX("semop");
}

static inline int shared_pool_unlock_and_return_err(struct shared_pool *sp, int err) {
    shared_pool_unlock(sp);
    return err;
}

static int t_add(struct shared_pool *sp,char *key, const uint8_t *p, size_t len) {
    if (len <= 0)
        return -ENOMSG;
    shared_pool_lock(sp);

    int8_t current;
    struct item *item = NULL;
    struct pool *pool = sp->pool;
    struct item *head = &pool->root;
    uint32_t off = 0;
    while ((current = *key++)) {
        current = symbol(current);
        if (current < 0)
            return shared_pool_unlock_and_return_err(sp,-EBADRQC);

        off = head->branch[current];
        if (off == 0) {
            if (pool->tree_pos + sizeof(*item) >= TREE_BLOB_SIZE - 1)
                return shared_pool_unlock_and_return_err(sp,-ENOMEM);
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
        return shared_pool_unlock_and_return_err(sp,-EFAULT);
    
    if (item->data.off == 0 || item->data.len < len) {
        if (pool->data_pos + len >= DATA_BLOB_SIZE)
            return shared_pool_unlock_and_return_err(sp,-ENOMEM);
        item->data.off = pool->data_pos;
        pool->data_pos += len;
    }

    item->data.len = len;
    item->used = IN_VALUE;
    memcpy(DATA(pool,item),p,len);
    return shared_pool_unlock_and_return_err(sp,0);
}

inline static int t_find_locked(struct pool *pool, char *key, struct result *r) {
    struct item *head = &pool->root;
    struct item *item = NULL;
    int8_t current;
    uint32_t off = 0;
    while ((current = *key++)) {
        current = symbol(current);
        if (current < 0)
            return -ENOKEY;

        off = head->branch[current];
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
        SAYX("key must be odd number, key + 1 is used for the semaphore's key");

    struct shared_pool *sp;
    BEEF_ALLOC(sp,struct shared_pool);
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
    shared_pool_unlock(sp);
    return sp;
}

static void shared_pool_reset(struct shared_pool *sp) {
    memset(sp->pool,0,sizeof(*sp->pool));
}

static void shared_pool_destroy(struct shared_pool *sp) {

    shared_pool_lock(sp);
    if (shmdt(sp->pool) != 0)
        SAYPX("detach failed");
    
    struct shmid_ds ds;
    
    if (shmctl(sp->shm_id, IPC_STAT, &ds) != 0)
        SAYPX("IPC_STAT failed on sem_id %d",sp->sem_id);
    if (ds.shm_nattch == 0) {
        if (semctl(sp->sem_id, 0, IPC_RMID ) != 0) 
            SAYPX("IPC_RMID failed on sem_id %d",sp->sem_id);
        if (shmctl(sp->shm_id, IPC_RMID, NULL) != 0) 
            SAYPX("IPC_RMID failed on shm_id %d",sp->shm_id);
    } else {
       shared_pool_unlock(sp); 
    }

    if (sp->copy)
        BEEF_FREE(sp->copy);
    BEEF_FREE(sp);
}

void shared_pool_copy_locally(struct shared_pool *sp) {
    shared_pool_lock(sp);
    if (sp->copy)
        BEEF_FREE(sp->copy);
    BEEF_ALLOC(sp->copy,struct pool);
    memcpy(sp->copy,sp->pool, POOL_SIZE(sp->pool));
    shared_pool_unlock(sp);
}

