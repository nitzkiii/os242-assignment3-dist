#include "kernel/types.h"
#include "user/user.h"
#include "kernel/spinlock.h"
#include "kernel/sleeplock.h"
#include "kernel/fs.h"
#include "kernel/file.h"
#include "kernel/fcntl.h"

#include "kernel/crypto.h"

int main(void) {
  if(open("console", O_RDWR) < 0){
    mknod("console", CONSOLE, 0);
    open("console", O_RDWR);
  }
  dup(0);  // stdout
  dup(0);  // stderr

  printf("crypto_srv: starting\n");

  // TODO: implement the cryptographic server here

   if (getpid() != 2) {
        printf("crypto_srv: process not started by kernel (PID should be 2)\n");
        exit(1);
    }

  void *addr = 0;
  uint64 size = 0;
  while (1) {
   
    int ret = take_shared_memory_request(&addr, &size);

    if (ret == -1) {
      // No request available
      continue;
    }

    struct crypto_op *op = addr;

    // Validate the request
    if (op->state != CRYPTO_OP_STATE_INIT ||
      (op->type != CRYPTO_OP_TYPE_ENCRYPT && op->type != CRYPTO_OP_TYPE_DECRYPT) ||
      op->key_size <= 0 || op->data_size <= 0 )  //op->key_size + op->data_size + sizeof(struct crypto_op) > size
      {
        // Ensure memory operations complete before updating state
        asm volatile ("fence rw,rw" : : : "memory");  
        op->state = CRYPTO_OP_STATE_ERROR;
        //remove_shared_memory_request(addr, size); //check if neededdddddddd
        continue;
      }

    // Process the request
    uchar *key = op->payload;
    uchar *data = op->payload + op->key_size;

    for (uint64 i = 0; i < op->data_size; i++) {
      data[i] ^= key[i % op->key_size];
    }

    // Ensure memory operations complete before updating state
    asm volatile ("fence rw,rw" : : : "memory");

    // Mark the request as done
    op->state = CRYPTO_OP_STATE_DONE;

    // Remove shared memory mapping
    remove_shared_memory_request(addr, size);
  }

  exit(0);
}
