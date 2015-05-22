#ifndef _BC_DB_H_
#define _BC_DB_H_

#include <stdint.h>
#include <stdbool.h>

#define DBKEYLEN 16
typedef struct db_key_s {
        unsigned char id[DBKEYLEN];
} db_key_t;

typedef db_key_t bc_chunk_id_t;

typedef uint32_t db_srec_id_t;

// 数据块 chunk 
typedef struct bc_chunk_s {
    unsigned char *start; // 在buf中的起始地址
    int len; 
    bool special;
    bool boundary;
    bc_chunk_id_t id;
    db_srec_id_t* srec_id;
} bc_chunk_t;

typedef struct bc_chunkgrp_s {
    unsigned char *buf;
    int buf_len;
    int chunk_cnt;
    bc_chunk_t *data_chunk;
    uint32_t data_chunk_len;
} bc_chunkgrp_t;

typedef struct bc_para_s {
    uint32_t session_id;
    bc_chunkgrp_t chunkgrp;
    uint32_t hit_cnt;
    uint32_t new_cnt;
    uint32_t raw_cnt;
} bc_para_t;

int bc_db_en_data_process(bc_para_t *bc_para);
int bc_db_adjust_srec_id(db_srec_id_t* srec_id, int vain_len);


/* 成功返回OK， 失败返回错误值，调用者根据错误值进行处理 BC_EWAITSYN BC_EREQSYN  BC_EHEAD等 */
int bc_db_de_data_process(uint32_t session_id, char *in_buf, int in_buf_len, char** out_buf, int* out_buf_len);

#endif
