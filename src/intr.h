#define PHYS_ADDR 0x800000000
#define doorbell_path "/dev/uio0"

extern void intr_send();
extern void intr_wait();
extern void intr_init();
