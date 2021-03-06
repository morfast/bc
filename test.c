#include "bc_util.h"
#include "bc_rabin.h"
#include "db.h"

#define BUF_SIZE 1000

void print_buf(char *buf, int len)
{
    int i;

    for (i = 0; i < len; i++) {
        if (i % 10 == 0) {
            fprintf(stderr, "\n");
        }
        fprintf(stderr, "%02x ", buf[i]);
    }
    fprintf(stderr, "\n");
}


int test_encode()
{
    int session_id;
    char *in_buf;
    int in_buf_len;
    char *outbuf;
    int out_buf_len;
    int i, ir;
    int j;

    in_buf_len = BUF_SIZE;
    session_id = 1;

    srand(time(NULL));
    for (ir = 0; ir < 2; ir++) {
        fprintf(stderr, "======================\n");
        in_buf = (char *)malloc(BUF_SIZE);
        for (i = 0; i < in_buf_len; i++) {
            in_buf[i] = i % 10;
        }
        // tiny modification
        if (ir > 0) {
            for (j = 0; j < 1; j++) {
                in_buf[rand() % BUF_SIZE] = 0;
            }
        }
        bc_encode(session_id, in_buf, in_buf_len, &outbuf, &out_buf_len);
    }

    //print_buf(in_buf, in_buf_len);
    //print_buf(outbuf, out_buf_len);

    //print_buf(in_buf, in_buf_len);
    //print_buf(outbuf, out_buf_len);

    return 0;
}
int main()
{
    bc_rabin_init();
    test_encode();
    return 0;
}
