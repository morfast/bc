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

int bc_rabin_init(void);
int bc_rabin_roll(char *pdata, 
					uint32_t size, 
					uint32_t *blksize,
					uint64_t *roll,
                    bool *natural_boundary,
                    bool *special);

int md5hash(char *data, uint32_t len, bc_chunk_id_t *hash);

#endif /*_BC_RABIN_H_*/

