#define PHYS_ADDR_READ 0x800000000
#define PHYS_ADDR_WRITE 0x900000000
#define doorbell_path "/dev/uio0"

extern void intr_send(const unsigned short dest);
extern void intr_wait();
extern void intr_init();
