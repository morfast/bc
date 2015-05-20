#ifndef _BC_RABIN_H_
#define _BC_RABIN_H_

#include"db.h"


#define M (0x8000000000000000)

#if (M&(M-1ULL) == 0ULL)
#define MOD(x) ((x)&((M)-1ULL))
#else
#define MOD(x) ((x)%(M))
#endif


#define DEF_BLK_SIZE	256
#define MIN_BLK_SIZE 	256
#define MAX_BLK_SIZE  	512

#define DEF_PERIM		5
#define DEF_ZERO_CNT 	5

typedef enum {
    BC_DATA_LARGE_CHUNK = 0x0A,
    BC_DATA_CHUNK_RAW = 0x4A,
    BC_DATA_CHUNK_SPECIAL = 0x8B,
    BC_DATA_CHUNK_NEW = 0xBE,
    BC_DATA_CHUNK_OLD = 0xEF,
} bc_chunk_type_t;


/* message headers for all chunks */
typedef struct bc_large_chunk_head_s {
    unsigned char type;
    unsigned char len[4];
} bc_large_chunk_head_t;

/* message headers for one chunk */
typedef struct bc_chunk_head_s {
    unsigned char type;
    unsigned char len[2];
} bc_chunk_head_t;

/* message headers for old chunk, old chunks do not contain data */
typedef struct bc_old_chunk_s {
    unsigned char type;
    unsigned char len[2];
    db_srec_id_t srec_id;
} bc_old_chunk_t;

int bc_rabin_init(void);
int bc_rabin_roll(char *pdata, 
					uint32_t size, 
					uint32_t *blksize,
					uint64_t *roll,
                    bool *natural_boundary,
                    bool *special);

int md5hash(char *data, uint32_t len, bc_chunk_id_t *hash);

/* 如果数据长度不够，就返回BC_ETOOSHORT。
   如果数据长度够，就返回BC_OK, 同时通过require_len值返回实际解码需要的数据长度 */
int bc_decode_get_len(bc_large_chunk_head_t *head, int in_buf_len, uint32_t *require_len);

#endif /*_BC_RABIN_H_*/

