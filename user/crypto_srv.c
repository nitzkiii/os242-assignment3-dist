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

   if (getpid() != 2) {
        printf("crypto_srv: process not started by kernel (PID should be 2)\n");
        exit(1);
    }

  void *addr = 0;
  uint64 size = 0;
  while (1) {
   
    int ret = take_shared_memory_request(&addr, &size);

    if (ret == -1) {
      // No request at the moment in the shared memory
      continue;
    }

    struct crypto_op *op = addr;

    // Check that the request received in the shared memory is valid
    if (op->state != CRYPTO_OP_STATE_INIT ||
      (op->type != CRYPTO_OP_TYPE_ENCRYPT && op->type != CRYPTO_OP_TYPE_DECRYPT) ||
      op->key_size <= 0 || op->data_size <= 0 )  
      {
        // Update the state to an error
        asm volatile ("fence rw,rw" : : : "memory");  
        op->state = CRYPTO_OP_STATE_ERROR;
        continue;
      }

    // Process the request
    uchar *key = op->payload;
    uchar *data = op->payload + op->key_size;

    //Encrypt the message with the XOR and save it to the data
    for (uint64 i = 0; i < op->data_size; i++) {
      data[i] ^= key[i % op->key_size];
    }

    // Update the state to Done
    asm volatile ("fence rw,rw" : : : "memory");
    op->state = CRYPTO_OP_STATE_DONE;

    // Remove mapping aafter proccessing the request
    remove_shared_memory_request(addr, size);
  }

  exit(0);
}
