
#include "bc_util.h"
#include "bc_rabin.h"
#include "db.h"
#include "intf_sys.h"

/*temp here,may be we should get this config from cfg file*/

//uint16_t w = DEF_WIN_SIZE;
static int perim = DEF_PERIM;
static int zero_cnt = DEF_ZERO_CNT;
static uint16_t max_blksize = MAX_BLK_SIZE;
static uint16_t min_blksize = MIN_BLK_SIZE;

static uint64_t finger_mask = 0xFFFFFFFFFFFFFFE0;
static uint64_t chara_finger_mask = 0xFFFFFFFFFFFFFF00;

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
					uint64_t *roll,
                    bool *natural_boundary,
                    bool *special)
{
    register unsigned short i;
    register uint64_t fingers;
    register uint64_t cur_roll = 0;
    char *p = pdata;
    uint64_t *pwin;
    uint64_t win[MAX_BLK_SIZE] = {0};

    /* size too small, do not split into blocks */
    if(size < min_blksize) {
        /* raw block */
        return size;
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
            /* natural_boundary */
			*roll = fingers;
            *blksize = min_blksize +i;
            *natural_boundary = 1;
            if ((fingers & chara_finger_mask) == fingers) {
                *special = 1;
            } else {
                *special = 0;
            }
			return 0;
		}
        if (min_blksize + i > max_blksize) {
            /* block larger than max_blksize,
               NOT natural_boundary */
            *blksize = min_blksize +i;
			*roll = fingers;
            *natural_boundary = 0;
            *special = 0;
			return 0;
        }

		cur_roll = (cur_roll - pwin[0])*perim + p[i];
	}
    /* raw block */
	return size;
}


int bc_split_into_block(uint32_t session_id, char *pdata, 
                        uint32_t size, bc_para_t *bc_para, 
                        uint32_t *remain_len)
{
    int i;
    int ichunk;
    uint64_t fp;
    uint32_t blksize;
    bc_chunkgrp_t *chunk_grp;
    bc_chunk_t  *chunk;
    bool nt_bond;
    bool special;
    int ret;

    chunk_grp = &(bc_para->chunkgrp);
    chunk_grp->buf = pdata;
    chunk_grp->buf_len = size;
    chunk_grp->chunk_cnt = 0;
    chunk_grp->data_chunk = (bc_chunk_t *)malloc(sizeof(bc_chunk_t) * size / MIN_BLK_SIZE);


    ichunk = 0;
    while (size > 0) {
        /* get a chunk */
        ret = bc_rabin_roll(pdata, size, &blksize, &fp, &nt_bond, &special);
        if (ret > 0) {
            /* raw block */
            *remain_len = ret;
            break;
        }
        /* put the chunk to result */
        chunk = (chunk_grp->data_chunk)+(ichunk++);
        chunk->start = pdata;
        chunk->len = blksize;
        chunk->boundary = nt_bond;
        chunk->special = special;
        md5hash(pdata, blksize, &(chunk->id));

        pdata += blksize;
        size -= blksize;
    }
    chunk_grp->chunk_cnt = ichunk;
}

int bc_encode(uint32_t session_id, char *in_buf, int in_buf_len,
              char **out_buf, int *out_buf_len, int *remain_len)
{
    bc_para_t bc_para;
    bc_chunkgrp_t *chunkgrp;
    bc_chunk_t  *chunk;
    int i, nchunk;
    int olen;


    bc_split_into_block(session_id, in_buf, in_buf_len, &bc_para, remain_len);
    //bc_db_en_data_process(&bc_para);
    /* grab the result */
    chunkgrp = &(bc_para.chunkgrp);
    nchunk = chunkgrp->chunk_cnt;

    /* count the total length of the output buffer */
    olen = 0;
    for(i = 0; i < nchunk; i++) {
        chunk = (chunkgrp->data_chunk) + i;
        if ((chunk->start != NULL) && 
            (chunk->srec_id == NULL)) {
            /* new chunk */
            // value
            olen += (chunk->len);
            // header
            olen += sizeof(bc_chunk_head_t);
        } else if ((chunk->start == NULL) &&
              (chunk->srec_id != NULL)) {
            /* old chunk */
            olen += (sizeof(bc_old_chunk_t));
        }
    }
    /* raw block */
    if (*remain_len != 0) {
        olen += *remain_len;
        olen += sizeof(bc_chunk_head_t);
    }
    /* the header for all chunks */
    olen += sizeof(bc_large_chunk_head_t);

    /* allocate output buffer */
    *out_buf_len = olen;
    *out_buf = (char *)malloc(olen);

    /* encode */
    char *pout = *out_buf;
    bc_large_chunk_head_t lheader;
    bc_chunk_head_t header;
    bc_old_chunk_t ochunk;
    /* large header */
    lheader.type = BC_DATA_LARGE_CHUNK;
    PUTSHORT(lheader.len, chunk->len);
    memcpy(pout, &lheader, sizeof(lheader));
    pout += (sizeof(lheader));
    for(i = 0; i < nchunk; i++) {
        chunk = (chunkgrp->data_chunk) + i;
        if ((chunk->start != NULL) && 
            (chunk->srec_id == NULL)) {
            /* new chunk */
            if (chunk->special == 0) {
                /* normal chunk */
                header.type = BC_DATA_CHUNK_NEW;
            } else {
                /* special chunk */
                header.type = BC_DATA_CHUNK_SPECIAL;
            }
            PUTSHORT(header.len, (ushort)chunk->len);
            memcpy(pout, &header, sizeof(header));
            pout += sizeof(header);
            /* value */
            memcpy(pout, chunk->start, chunk->len);
            pout += (chunk->len);
        } else if ((chunk->start == NULL) &&
              (chunk->srec_id != NULL)) {
            /* old chunk */
            ochunk.type = BC_DATA_CHUNK_OLD;
            PUTSHORT(header.len, (ushort)chunk->len);
            memcpy(&(ochunk.srec_id), chunk->srec_id, sizeof(db_srec_id_t));
            memcpy(pout, &ochunk, sizeof(ochunk));
            pout += sizeof(ochunk);
        }
    }
    /* raw block */
    if (*remain_len != 0) {
        header.type = BC_DATA_CHUNK_RAW;
        PUTSHORT(header.len, (ushort)chunk->len);
        memcpy(pout, &header, sizeof(header));
        pout += sizeof(header);
        memcpy(pout, in_buf + (in_buf_len - *remain_len), *remain_len);
    }


    return 0;
    
}

        


