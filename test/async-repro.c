#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include "liburing.h"

#define BUF_SIZE	(256 * 1024)
#define N               60000

int main(int argc, char *argv[])
{
  struct io_uring_sqe *sqe;
  struct io_uring_cqe *cqe;
  struct io_uring ring;
  int i, fd, ret;
  char *buf, *fname;
  off_t off;

  fname = ".cpu-spin-rw";
  fd = open(fname, O_CREAT | O_WRONLY | O_SYNC, 0644);
  buf = malloc(BUF_SIZE);
  struct iovec iov = {
    .iov_base = buf,
    .iov_len = sizeof(buf) - 1,
  };
  off = 0;


  ret = io_uring_queue_init(4096, &ring, 0);
  if (ret) {
    fprintf(stderr, "ring create failed: %d\n", ret);
    return 1;
  }

  for (i = 0; i < N; i++) {
    while (true) {
      sqe = io_uring_get_sqe(&ring);
      if (!sqe) {
        ret = io_uring_submit(&ring);
        if (ret < 0) {
          fprintf(stderr, "failed to submit\n");
          return -1;
        }
        continue;
      }
      break;
    }
    io_uring_prep_writev(sqe, fd, &iov, 1, off);
    sqe->flags = IOSQE_ASYNC;
    off += BUF_SIZE;
  }
  ret = io_uring_submit(&ring);
  if (ret < 0) {
    fprintf(stderr, "failed to submit\n");
    return -1;
  }

  for (i = 0; i < N; i++) {
    ret = io_uring_wait_cqe(&ring, &cqe);
    if (ret) {
      fprintf(stderr, "wait_cqe=%d\n", ret);
      return -1;
    }
    io_uring_cqe_seen(&ring, cqe);
  }

  io_uring_queue_exit(&ring);
  close(fd);
  unlink(fname);
  return 0;
}
