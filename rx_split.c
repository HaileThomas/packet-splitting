#include <stdint.h>
#include <stdio.h>
#include <inttypes.h>
#include <rte_eal.h>
#include <rte_ethdev.h>
#include <rte_mbuf.h>
#include <rte_ether.h>

#define PORT_ID            0
#define RX_QUEUE_ID        0
#define TX_QUEUE_ID        0
#define RX_QUEUE_COUNT     1
#define TX_QUEUE_COUNT     0
#define RX_RING_SIZE       16
#define TX_RING_SIZE       16
#define NUM_MBUFS          8191
#define MBUF_CACHE_SIZE    250
#define BURST_SIZE         32
#define SPLIT_HDR_SIZE     64

static void setup(int argc, char *argv[]) {
    if (rte_eal_init(argc, argv) < 0) {
        rte_exit(EXIT_FAILURE, "Error with EAL initialization\n");
    }

    if (rte_eth_dev_count_avail() == 0) {
        rte_exit(EXIT_FAILURE, "No Ethernet ports available\n");
    }
    
    int socket_id = rte_socket_id();
    struct rte_mempool *host_pool = rte_pktmbuf_pool_create("HOST_POOL", 
        NUM_MBUFS, MBUF_CACHE_SIZE, 0, RTE_PKTMBUF_HEADROOM + SPLIT_HDR_SIZE, socket_id);

    if (host_pool == NULL) {
        rte_exit(EXIT_FAILURE, "Cannot create host mbuf pool\n");
    }

    struct rte_mempool *dummy_pool = rte_pktmbuf_pool_create("DUMMY_POOL", 
        NUM_MBUFS, MBUF_CACHE_SIZE, 0, 1500, socket_id);

    if (dummy_pool == NULL) {
        rte_exit(EXIT_FAILURE, "Cannot create dummy mbuf pool\n");
    }

    struct rte_eth_conf port_conf = {
        .rxmode = {
            .offloads = RTE_ETH_RX_OFFLOAD_BUFFER_SPLIT | RTE_ETH_RX_OFFLOAD_SCATTER,
        },
    };

    if (rte_eth_dev_configure(PORT_ID, RX_QUEUE_COUNT, TX_QUEUE_COUNT, &port_conf) < 0) {
        rte_exit(EXIT_FAILURE, "Cannot configure device\n");
    }

    union rte_eth_rxseg rx_segs[2] = {0};
    rx_segs[0].split.length = SPLIT_HDR_SIZE;
    rx_segs[0].split.mp = host_pool;
    rx_segs[1].split.length = 0;
    rx_segs[1].split.mp = dummy_pool;

    struct rte_eth_rxconf rxq_conf = {
        .rx_nseg = 2,
        .rx_seg = rx_segs,
    };

    if (rte_eth_rx_queue_setup(PORT_ID, RX_QUEUE_ID, RX_RING_SIZE, socket_id, &rxq_conf, NULL) < 0) {
        rte_exit(EXIT_FAILURE, "rte_eth_rx_queue_setup failed\n");
    }

    rte_eth_promiscuous_enable(PORT_ID);

    if (rte_eth_dev_start(PORT_ID) < 0) {
        rte_exit(EXIT_FAILURE, "rte_eth_dev_start failed\n");
    }
}

static void print_segments(struct rte_mbuf *m) {
    struct rte_mbuf *seg = m;
    int seg_idx = 0;

    while (seg != NULL) {
        printf("    seg[%d]: data_len=%u\n",
               seg_idx, seg->data_len);
        seg = seg->next;
        seg_idx++;
    }
}

int main(int argc, char *argv[]) {
    setup(argc, argv);

    printf("Port %u started with Buffer Split enabled.\n", PORT_ID);

    struct rte_mbuf *bufs[BURST_SIZE];
    while (1) {
        const uint16_t nb_rx = rte_eth_rx_burst(PORT_ID, RX_QUEUE_ID, bufs, BURST_SIZE);
        if (unlikely(nb_rx == 0)) continue;

        for (int i = 0; i < nb_rx; i++) {
            struct rte_mbuf *m = bufs[i];

            // if (m->nb_segs > 1) {
            //     printf("  [SUCCESS] Packet split into %u segments (pkt_len=%u)\n",
            //         m->nb_segs, m->pkt_len);

            //     print_segments(m);

            // } else {
            //     printf("  [FAILED] Not split. nb_segs=%u, len=%u\n",
            //         m->nb_segs, m->data_len);
            // }

            rte_pktmbuf_free(m);
        }
    }
    
    return 0;
}