
#include "bc_util.h"
#include "bc_rabin.h"

/*temp here,may be we should get this config from cfg file*/

//uint16_t w = DEF_WIN_SIZE;
static int perim = DEF_PERIM;
static int zero_cnt = DEF_ZERO_CNT;
static uint16_t max_blksize = MAX_BLK_SIZE;
static uint16_t min_blksize = MIN_BLK_SIZE;

static uint64_t finger_mask = 0xFFFFFFFFFFFFFFE0;

/******************End of temp*********************/

static uint64_t *polynomial_buf = NULL;

int bc_rabin_init(void)
{
	uint64_t index;
	uint64_t polynomial = 1;

	polynomial_buf = (uint64_t *)malloc(sizeof(uint64_t) * (max_blksize + 1));
	if(NULL == polynomial_buf)
	{
		ERROR("malloc fail!");
		return -1;
	}
	for(index = 0; index <= min_blksize; index++)
	{
		polynomial_buf[min_blksize - index] = polynomial;
		polynomial *= perim;
	}

	finger_mask = (~((1ULL << zero_cnt) -1ULL));
	return 0;
}

int bc_rabin_roll(char *pdata, 
					uint32_t size, 
					uint32_t *blksize,
					uint64_t *roll)
{
    register unsigned short i;
    register uint64_t fingers;
    register uint64_t cur_roll = 0;
    char *p = pdata;
    uint64_t *pwin;
    uint64_t win[MAX_BLK_SIZE] = {0};

    /* size too small, do not split into blocks */
    if(size < min_blksize) {
        return -1;
    }
	
    /* front segment smaller than min_blksize */
	pwin = win;
	for(i = 0; i < min_blksize; i++) {
		pwin[i] = p[i] * polynomial_buf[i];
		cur_roll += pwin[i];
	}

    /* left segment */
	p = pdata + min_blksize;
	for(i = 0; i < (size - min_blksize);i++) {
		fingers = MOD(cur_roll);
		if( ((fingers & finger_mask) == fingers) && 
            (fingers != 0) ) {
			*roll = fingers;
            *blksize = min_blksize +i;
			return 0;
		}
        if (min_blksize + i > max_blksize) {
            *blksize = min_blksize +i;
			*roll = fingers;
			return 0;
        }

		cur_roll = (cur_roll - pwin[0])*perim + p[i];
	}
	return -1;
}

int bc_split_block(char *pdata, uint32_t size)
{
    int i;
    uint64_t fp;
    uint32_t blksize;


    i = 0;
    while (size > 0) {
        bc_rabin_roll(pdata, size, &blksize, &fp);
        /* put the block to result */

        pdata += blksize;
        size -= blksize;
    }
}
        


