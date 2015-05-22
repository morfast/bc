
#include "bc_util.h"
#include "bc_rabin.h"
#include "db.h"
#include "intf_sys.h"
#include <stdio.h>

/*temp here,may be we should get this config from cfg file*/

//uint16_t w = DEF_WIN_SIZE;
static int perim = DEF_PERIM;
static int zero_cnt = DEF_ZERO_CNT;
static uint16_t max_blksize = MAX_BLK_SIZE;
static uint16_t min_blksize = MIN_BLK_SIZE;

static uint64_t chara_finger_mask = 0xFFFFFFFFFFFFFF00;
static uint64_t finger_mask = 0xFFFFFFFFFFFFFFE0;

/******************End of temp*********************/

static uint64_t *polynomial_buf = NULL;
static char remain_buff[MIN_BLK_SIZE];
static uint32_t remain_buff_len = 0;

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
            fprintf(stderr, "natural_boundary found: %x\n", fingers);
			*roll = fingers;
            *blksize = min_blksize +i;
            *natural_boundary = 1;
            if ((fingers & chara_finger_mask) == fingers) {
                fprintf(stderr, "special found\n");
                *special = 1;
            } else {
                *special = 0;
            }
			return 0;
		}
        if (min_blksize + i > max_blksize) {
            /* block larger than max_blksize,
               NOT natural_boundary */
            fprintf(stderr, "m boundary found: %x\n", fingers);
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
        chunk->srec_id = NULL;
        md5hash(pdata, blksize, &(chunk->id));

        pdata += blksize;
        size -= blksize;
    }
    chunk_grp->chunk_cnt = ichunk;
}

int get_prev_remain(uint32_t session_id, char **prev_raw, int *prev_raw_len)
{
    *prev_raw = remain_buff;
    *prev_raw_len = remain_buff_len;
}

char* combine_buf(char *a, int alen, char *b, int blen)
{
    char *c;

    c = (char *)malloc(alen + blen);
    memcpy(c,a,alen);
    memcpy(c+alen, b, blen);

    return c;
}

char* combine_3buf(char *a, int alen, char *b, int blen, char *c, int clen)
{
    char *d;

    d = (char *)malloc(alen + blen + clen);
    memcpy(d,a,alen);
    memcpy(d+alen, b, blen);
    memcpy(d+alen+blen, c, clen);

    return d;
}


int bc_encode(uint32_t session_id, char *ori_in_buf, int ori_in_buf_len,
              char **out_buf, int *out_buf_len)
{
    bc_para_t bc_para;
    bc_chunkgrp_t *chunkgrp;
    bc_chunk_t  *chunk;
    char *prev_raw;
    char *in_buf;
    int prev_raw_len;
    int in_buf_len;
    int i, nchunk;
    int olen;
    uint32_t remain_len;

    /* combine the remain buffer of the last transfer */
    get_prev_remain(session_id, &prev_raw, &prev_raw_len);
    in_buf = (char *)combine_buf(prev_raw, prev_raw_len, ori_in_buf, ori_in_buf_len);
    in_buf_len = ori_in_buf_len + prev_raw_len;

    /* split the combined input buffer into blocks */
    bc_split_into_block(session_id, in_buf, in_buf_len, &bc_para, &remain_len);
    
    /* cache the blocks to the database */
    //bc_db_en_data_process(&bc_para);


    /* grab the result */
    chunkgrp = &(bc_para.chunkgrp);
    nchunk = chunkgrp->chunk_cnt;

    /* count the total length of the output buffer */
    olen = 0;
    /* the header for all chunks */
    olen += sizeof(bc_large_chunk_head_t);
    fprintf(stderr, "olen: %d\n", olen);
    /* first block: contain prev_raw, special treatment */
    chunk = (chunkgrp->data_chunk);
    if ((chunk->start != NULL) && 
        (chunk->srec_id == NULL)) {
        /* new chunk */
        olen += sizeof(bc_chunk_head_t);
        olen += (chunk->len - prev_raw_len);
    } else if ((chunk->start == NULL) &&
          (chunk->srec_id != NULL)) {
        /* old chunk */
        olen += sizeof(bc_old_chunk_t);
    }
    fprintf(stderr, "olen: %d\n", olen);

    /* other blocks */
    for(i = 1; i < nchunk; i++) {
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
    fprintf(stderr, "olen: %d\n", olen);
    /* raw block */
    if (remain_len != 0) {
        olen += remain_len;
        olen += sizeof(bc_chunk_head_t);
    }
    fprintf(stderr, "olen: %d\n", olen);

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
    PUTSHORT(lheader.len, olen-sizeof(bc_large_chunk_head_t));
    memcpy(pout, &lheader, sizeof(lheader));
    pout += (sizeof(lheader));
    fprintf(stderr, "pout: %d\n", (pout - *out_buf));

    /* first block: contain prev_raw, special treatment */
    chunk = (chunkgrp->data_chunk);
    if ((chunk->start != NULL) && 
        (chunk->srec_id == NULL)) {
        /* new chunk */
        fprintf(stderr, "new chunk\n");
        header.type = BC_DATA_CHUNK_RAW;
        PUTSHORT(header.len, (ushort)(chunk->len - prev_raw_len));
        memcpy(pout, &header, sizeof(header));
        pout += sizeof(header);
        memcpy(pout, chunk->start + prev_raw_len, chunk->len - prev_raw_len);
        pout += (chunk->len - prev_raw_len);
    } else if ((chunk->start == NULL) &&
          (chunk->srec_id != NULL)) {
        /* old chunk */
        /* adjust the offset */
        //bc_db_adjust_srec_id(chunk->srec_id, prev_raw_len);
        fprintf(stderr, "old chunk\n");
        ochunk.type = BC_DATA_CHUNK_OLD;
        PUTSHORT(header.len, (ushort)chunk->len);
        memcpy(&(ochunk.srec_id), chunk->srec_id, sizeof(db_srec_id_t));
        memcpy(pout, &ochunk, sizeof(ochunk));
        pout += sizeof(ochunk);

    }
    fprintf(stderr, "pout: %d\n", (pout - *out_buf));

    /* other blocks */
    for(i = 1; i < nchunk; i++) {
        chunk = (chunkgrp->data_chunk) + i;
        if ((chunk->start != NULL) && 
            (chunk->srec_id == NULL)) {
            fprintf(stderr, "new chunk\n");
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
            fprintf(stderr, "old chunk\n");
            ochunk.type = BC_DATA_CHUNK_OLD;
            PUTSHORT(header.len, (ushort)chunk->len);
            memcpy(&(ochunk.srec_id), chunk->srec_id, sizeof(db_srec_id_t));
            memcpy(pout, &ochunk, sizeof(ochunk));
            pout += sizeof(ochunk);
        }
    }
    fprintf(stderr, "pout: %d\n", (pout - *out_buf));
    /* raw block */
    if (remain_len != 0) {
        fprintf(stderr, "remain_len: %d\n", remain_len);
        header.type = BC_DATA_CHUNK_RAW;
        PUTSHORT(header.len, (ushort)chunk->len);
        memcpy(pout, &header, sizeof(header));
        pout += sizeof(header);
        memcpy(pout, in_buf + (in_buf_len - remain_len), remain_len);
        /* keep this remaining block for next encoding */
        memcpy(remain_buff, pout, remain_len);
        remain_buff_len = remain_len;
        pout += remain_len;
    }
    fprintf(stderr, "pout: %d\n", (pout - *out_buf));

    /* free the input buffer */
    free(ori_in_buf);
    free(in_buf);

    return 0;
    
}

int bc_decode(uint32_t session_id, char *in_buf, int in_buf_len,
              char **out_buf, int *out_buf_len)

{ 
    /* 成功返OK， 失败返回错误值，调用者根据错误值进行处理 BC_EWAITSYN BC_EREQSYN  BC_EHEAD等 */
    int ret;
    char *prev_raw;
    int prev_raw_len;
    char *pbuf = in_buf;
    char *in_buf_start = in_buf;
    char *new_in_buf;
    unsigned short len;
    /* add prev_raw_buff to the first chunk */
    get_prev_remain(session_id, &prev_raw, &prev_raw_len);
    bc_large_chunk_head_t lheader;
    bc_chunk_head_t *header;


    pbuf += sizeof(lheader);
    header = (bc_chunk_head_t *)pbuf;
    if (header->type == BC_DATA_CHUNK_RAW) {
        // add prev_raw_buff to the front of the value
        len = GETSHORT(header->len);
        len += prev_raw_len;
        PUTSHORT(&(header->len), len);
        pbuf += sizeof(bc_chunk_head_t);
        new_in_buf = combine_3buf(in_buf_start, sizeof(lheader) + sizeof(bc_chunk_head_t), 
                                  prev_raw, prev_raw_len, 
                                  pbuf, in_buf_len - (sizeof(lheader) + sizeof(bc_chunk_head_t)));
        //ret = bc_db_de_data_process(session_id, new_in_buf, in_buf_len, out_buf, out_buf_len);
    } else {
        //ret = bc_db_de_data_process(session_id, in_buf, in_buf_len, out_buf, out_buf_len);
    }
    /* remove prev_raw_buff from the result buffer */
    *out_buf += prev_raw_len;
    *out_buf_len -= prev_raw_len;

    free(new_in_buf);
    free(in_buf);

}

/* 如果数据长不够，就返回BC_ETOOSHORT。
   如果数据长够，就返回BC_OK, 同时通过require_len值返回实际解码需要的数据长度 */
int bc_decode_get_len(bc_large_chunk_head_t *head, int in_buf_len, uint32_t *require_len)
{
    if (in_buf_len < sizeof(bc_large_chunk_head_t)) {
        return BC_ETOOSHORT;
    } else {
        *require_len = GETLONG(head->len);
        return BC_OK;
    }
}

