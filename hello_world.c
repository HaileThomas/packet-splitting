#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <inttypes.h>
#include <rte_eal.h>
#include <rte_ethdev.h>
#include <rte_mbuf.h>

#define PORT_ID            0
#define RX_QUEUE_ID        0
#define TX_QUEUE_ID        0
#define RX_QUEUE_COUNT     1
#define TX_QUEUE_COUNT     1
#define RX_RING_SIZE       128
#define TX_RING_SIZE       128
#define NUM_MBUFS          8192
#define MBUF_CACHE_SIZE    250
#define MBUF_DATA_SIZE     2048

static void setup(int argc, char *argv[]) {
    if (rte_eal_init(argc, argv) < 0) {
        rte_exit(EXIT_FAILURE, "Error with EAL initialization\n");
    }

    if (!rte_eth_dev_is_valid_port(PORT_ID)) {
        rte_exit(EXIT_FAILURE, "Port %u not available\n", PORT_ID);
    }

    int socket_id = rte_eth_dev_socket_id(PORT_ID);
    struct rte_mempool *mbuf_pool = rte_pktmbuf_pool_create("MBUF_POOL", 
        NUM_MBUFS, MBUF_CACHE_SIZE, 0, MBUF_DATA_SIZE, socket_id);

    if (mbuf_pool == NULL) {
        rte_exit(EXIT_FAILURE, "Cannot create mbuf pool\n");
    }

    struct rte_eth_conf port_conf = {0};
    if (rte_eth_dev_configure(PORT_ID, RX_QUEUE_COUNT, TX_QUEUE_COUNT, &port_conf) < 0) {
        rte_exit(EXIT_FAILURE, "rte_eth_dev_configure failed\n");
    }

    if (rte_eth_rx_queue_setup(PORT_ID, RX_QUEUE_ID, RX_RING_SIZE, socket_id, NULL, mbuf_pool) < 0) {
        rte_exit(EXIT_FAILURE, "Rx queue setup failed\n");
    }

    if (rte_eth_tx_queue_setup(PORT_ID, TX_QUEUE_ID, TX_RING_SIZE, socket_id, NULL) < 0) {
        rte_exit(EXIT_FAILURE, "Tx queue setup failed\n");
    }

    if (rte_eth_dev_start(PORT_ID) < 0) {
        rte_exit(EXIT_FAILURE, "rte_eth_dev_start failed\n");
    }
}

int main(int argc, char *argv[]) {
    setup(argc, argv);

    printf("\n--- Starting DM Access Test ---\n");

    uint8_t write_buffer[64] = "Hello World!";
    uint8_t read_buffer[64] = {0};

    if (rte_eth_memcpy_to_dm(PORT_ID, 0, write_buffer, 20, 0) == 0) {
        printf("Successfully wrote to DM!\n");

        if (rte_eth_memcpy_from_dm(PORT_ID, 0, read_buffer, 20, 0) == 0) {
            printf("Read back from DM: %s\n", read_buffer);
        }
    }

    rte_eth_dev_stop(PORT_ID);
    rte_eal_cleanup();
    return 0;
}