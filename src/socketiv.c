#include <stdint.h>
#include <stdlib.h>

typedef enum block_size_mode // byte
{
    BSM_1 = 262144
} BLK_SIZ_MODE;
typedef enum interrupt_mode // count
{
    IM_1 = 10
} INT_MODE;
typedef enum polling_mode // ns
{
    PM_1 = 1000,
    PM_2 = 100000
} POLL_MODE;
typedef enum throughput_level // Gbps
{
    TL_1 = 1
} T_LVL;
typedef enum burst_timeout_mode // ns
{
    BTM_1 = 1000000
} BURST_TMO_MODE;

typedef struct inter_vm_socket
{
    BLK_SIZ_MODE block_size_mode;
    INT_MODE interrupt_mode;
    POLL_MODE polling_mode;
    T_LVL throughput_level;
    BURST_TMO_MODE burst_timeout_mode;

    IVSM *ivsm_ptr;
} IVSOCK;
typedef struct inter_vm_shmem
{
    uint64_t host_size;
    void *host_base;
    void *host_rptr;
    void *host_wptr;
    void *host_interrupt_uio;
    uint64_t remote_size;
    void *remote_base;
    void *remote_rptr;
    void *remote_wptr;
    int remote_interrupt_uio;
} IVSM;

int socketiv_inited = 0;
uint32_t vm_subnet;
int socketiv_init();

int *socket_list = calloc(3, sizeof(int));
int socket_list_size = 3;
int highest_socket = 2;
int socketiv_register_socket();
int socketiv_check_socket();
int socketiv_unregister_socket();

int socketiv_check_vm_subnet();
int socketiv_establish();

int socketiv_read();
int socketiv_write();

int socketiv_interrupt();
int socketiv_wait();

int socketiv_monitor();
int socketiv_poll();
int socketiv_set_polling_mode();

int socketiv_sleep();