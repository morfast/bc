#include "bc_util.h"
#include "bc_rabin.h"
#include "db.h"

#define BUF_SIZE 1000

int test_encode()
{
    int session_id;
    char *in_buf;
    int in_buf_len;
    char *outbuf;
    int out_buf_len;

    in_buf = (char *)malloc(BUF_SIZE);
    in_buf_len = BUF_SIZE;
    session_id = 1;

    bc_encode(session_id, in_buf, in_buf_len, &outbuf, &out_buf_len);

    return 0;
}
int main()
{
    bc_rabin_init();
    test_encode();
    return 0;
}
