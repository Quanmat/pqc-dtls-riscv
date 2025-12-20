#include <stdio.h>
#include <string.h>
#include <sys/types.h>

/* LiteX / SoC specific includes */
#include <irq.h>                
#include <libbase/uart.h>       
#include <libbase/console.h>    
#include <generated/csr.h>      
#include <generated/soc.h>      
#include <libliteeth/udp.h>     

/* WolfSSL cryptography and SSL/TLS includes */
#include <wolfssl/wolfcrypt/user_settings.h>
#include <wolfssl/options.h>
#include <wolfssl/ssl.h>
#include <wolfssl/wolfcrypt/random.h>
#include <wolfssl/wolfcrypt/sha3.h>
#include <wolfssl/wolfcrypt/wc_mlkem.h>
#include <wolfssl/wolfcrypt/types.h>
#include <wolfssl/wolfio.h>
typedef wc_Sha3 wc_Shake;       // Typedef required for PQC algorithms

/* Custom files includes */
#include "certs/client_der.h"   // Client certificate
#include "certs/client_key.h"   // Client private key
#include "certs/CA_der.h"       // CA Certificate
#include "mem_profile.h"        // Memory profiling

/*  
 * Custom RNG (Random Number Generator)
 */
int CustomRngGenerateBlock(byte *output, word32 sz){
    for (word32 i = 0; i < sz; i++)
        output[i] = (byte)(i * 37 + 123);
    return 0;
}
/* 
 * Reads the 64-bit hardware cycle counter on RISC-V.
 */
uint64_t read_cycle64(void){
    uint32_t hi, lo, hi2;
    /* Loop prevents an inconsistent 64-bit value if the low 32 bits overflow
    (wrap around) while reading the high and low parts.*/
    do{
        asm volatile("rdcycleh %0" : "=r"(hi));
        asm volatile("rdcycle  %0" : "=r"(lo));
        asm volatile("rdcycleh %0" : "=r"(hi2));
    } 
    while (hi != hi2);
    return ((uint64_t)hi << 32) | lo;
}

/* 
 * Converts raw CPU cycles to milliseconds based on system clock frequency.
 */
static uint32_t cycles_to_ms(uint64_t c){
    uint64_t temp = (CONFIG_CLOCK_FREQUENCY / 1000);
    return (uint32_t)((c / temp));
}

/* 
 * for WolfSSL USER_TICKS, returns time in seconds.
 */
unsigned int LowResTimer(void)
{
    return cycles_to_ms(read_cycle64()) / 1000;
}

/*                           */
/* GLOBAL METRICS & COUNTERS */
/*                           */

/* Timing markers for performance analysis */
uint64_t dilith_start_clks;
uint64_t dilith_end_clks;
uint64_t client_start_clocks;
uint64_t client_end_clocks;

/* Global Metrics Storage */
uint64_t t_session_start = 0;
uint64_t t_handshake_start = 0;
uint64_t t_handshake_end = 0;
uint64_t t_data_start = 0;       // Timer for sustained data start
uint64_t t_data_end = 0;         // Timer for sustained data end

uint32_t total_bytes_tx = 0;
uint32_t total_bytes_rx = 0;
uint32_t handshake_tx_bytes = 0;
uint32_t handshake_rx_bytes = 0;
uint32_t data_cycles = 0;
uint8_t in_handshake = 0;        // Flag to track if we are in handshake phase
uint8_t handshake_ms = 0;

/* Throughput test configuration */
#define THROUGHPUT_TEST_SIZE (50 * 1024) // Total data to send in test
#define CHUNK_SIZE 1200                  // Max payload size per DTLS record

/*                       */
/* NETWORK CONFIGURATION */
/*                       */

/* Static IP configures for the simulation environment and Host IP corresponds to the tap0 interface.*/
uint32_t my_ip = IPTOINT(192, 168, 1, 50);
uint32_t host_ip = IPTOINT(192, 168, 1, 100);
uint8_t my_mac[6] = {0x02, 0x00, 0x00, 0x00, 0x00, 0x01};

/* UDP Ports */
static const uint16_t SOC_SRC_PORT = 15000;  // Port on the Client (RISC-V) side
static const uint16_t HOST_DST_PORT = 4444;  // Port on the Server side

/*                */
/* RX RING BUFFER */
/*                */

/*  Ring Buffer for buffering incoming UDP packets from the ISR and
    it decouples the high-speed Interrupt Handler from the slower WolfSSL read loop. */
#define MAX_PACKET_SIZE 1500
#define RX_QUEUE_DEPTH 16

typedef struct
{
    uint8_t data[MAX_PACKET_SIZE];
    uint32_t len;
    uint32_t read_offset;   // Tracks how much WolfSSL has read from this packet
    volatile uint8_t ready; // 1 if packet contains data, 0 if empty
} PacketSlot;

static PacketSlot rx_queue[RX_QUEUE_DEPTH];
static volatile uint8_t write_idx = 0;
static volatile uint8_t read_idx = 0;

/* Clears the buffer safely by disabling interrupts during the reset. */
void flush_rx_queue(void){
    unsigned int old_ie = irq_getie();
    irq_setie(0); //Disable IRQ while modifying queue pointers

    for (int i = 0; i < RX_QUEUE_DEPTH; i++)    {
        rx_queue[i].ready = 0;
        rx_queue[i].read_offset = 0;
    }
    write_idx = 0;
    read_idx = 0;
    irq_setie(old_ie); // Restore IRQ
}

/*                   */
/* INTERRUPT HANDLER */
/*                   */

/*Runs immediately when the Ethernet hardware raises an interrupt.*/
void eth_irq_handler(void)
{
    udp_service();
}

/*                            */
/* CALLBACKS & IO ABSTRACTION */
/*                            */

/* Debugs Certificate Verification Issues.*/
int my_verify_cb(int preverify, WOLFSSL_X509_STORE_CTX *store)
{
    if (preverify == 0){
        int err = wolfSSL_X509_STORE_CTX_get_error(store);
        printf("[VERIFY] Verification Failed! Error: %d, Description: %s\n",
               err, wolfSSL_ERR_error_string(err, NULL));
               
    }else{
        printf("[VERIFY] Certificate valid.\n");
    }
    return preverify;
}

/* Called by udp_service() inside the ISR context,
   it filters packets and pushes them into the Ring Buffer. */
static void my_udp_rx(uint32_t src_ip, uint16_t src_port, uint16_t dst_port, void *data, uint32_t length)
{
    if (src_ip != host_ip || src_port != HOST_DST_PORT)
        return;

    if (length > MAX_PACKET_SIZE)
        return;

    if (rx_queue[write_idx].ready){
        return;
    }

    memcpy(rx_queue[write_idx].data, data, length); // Copy data to software buffer
    rx_queue[write_idx].len = length;
    rx_queue[write_idx].read_offset = 0;
    rx_queue[write_idx].ready = 1;
    
#ifdef DEBUG
    printf("[IO-IRQ] Queued packet at slot %d (%ld bytes)\n", write_idx, length);
#endif

    write_idx = (write_idx + 1) % RX_QUEUE_DEPTH; // Advance write pointer
}

/* Sends data over UDP. */
static int EmbedSend(WOLFSSL *ssl, char *buf, int sz, void *ctx)
{
    /* 1. Flush Queue: Sending a new state, ignore old retransmissions. */
    if (sz > 0)
        flush_rx_queue();

    /* 2. Disable interrupts here bcoz,'udp_send' manipulates the Ethernet
       TX buffer descriptors. If an interrupt fires here 
       (triggering eth_irq_handler -> udp_service),
       the ISR might try to send an ARP reply, corrupting the shared TX state. */
    unsigned int old_ie = irq_getie();
    irq_setie(0);

    uint8_t *tx_buf = (uint8_t *)udp_get_tx_buffer();

    int ret = sz;
    if (tx_buf == NULL)    {
        ret = WOLFSSL_CBIO_ERR_WANT_WRITE;  // Hardware buffer busy
    }else{
        memcpy(tx_buf, buf, sz);  // Copy data to hardware buffer and fire
        udp_send(SOC_SRC_PORT, HOST_DST_PORT, sz);
        
        if (in_handshake) // Track metrics
            handshake_tx_bytes += sz;
    }
    /* 3. RESTORE INTERRUPTS */
    irq_setie(old_ie);

    return ret;
}

/* Reads data from our Ring Buffer (not directly from HW). */
static int EmbedReceive(WOLFSSL *ssl, char *buf, int sz, void *ctx)
{
    PacketSlot *slot = &rx_queue[read_idx];
    
    if (!slot->ready){
        return WOLFSSL_CBIO_ERR_WANT_READ;
    }

    uint32_t remaining = slot->len - slot->read_offset; // Calculating how much data is left in this packet
    int copy_len = (sz < remaining) ? sz : remaining;

    memcpy(buf, slot->data + slot->read_offset, copy_len); // Copy data to WolfSSL buffer
    slot->read_offset += copy_len;

    if (slot->read_offset >= slot->len){
        slot->ready = 0;
        read_idx = (read_idx + 1) % RX_QUEUE_DEPTH;
    }

    if (in_handshake) // Track metrics
        handshake_rx_bytes += copy_len;
        
    return copy_len;
}

/*           */
/* REPORTING */
/*           */

/* Calculates and displays throughput and efficiency metrics. */
void print_performance_report(void)
{
    printf("Throughput Metrics:\n");

    if (data_cycles > 0){

        uint32_t data_ms = cycles_to_ms(data_cycles);

        if (data_ms == 0)  // Safety check
            data_ms = 1;
            
        uint32_t throughput_kbs = (THROUGHPUT_TEST_SIZE / 1024) * 1000 / data_ms;  // Throughput calculation
        printf("    a. Data Transmission Throughput (Sustained): %lu KB/s\n", (unsigned long)throughput_kbs);
        printf("         - Measured over %lu bytes in %lu ms\n", (unsigned long)THROUGHPUT_TEST_SIZE, (unsigned long)data_ms);
    }else{
        printf("    a. Data Transmission Throughput (Sustained): N/A (Test skipped/t_data timers invalid)\n");
    }
    // Handshake Efficiency (Total Handshake Bytes / Handshake Time)
    if (handshake_ms > 0){
        uint32_t efficiency_bytes_per_ms = (handshake_tx_bytes + handshake_rx_bytes) / handshake_ms;
        printf("    b. Efficiency (Handshake Phase): %lu Bytes/ms\n", (unsigned long)efficiency_bytes_per_ms);
    }else{
        printf("    b. Efficiency (Handshake Phase): N/A (Handshake time is zero)\n");
    }
}

/*            */
/* MAIN LOGIC */
/*            */

/* The core state machine: Init -> Load Certs -> Handshake -> Data Test -> Report. */
void run_dtls_client(void)
{
    // 1. Initialize WolfSSL with memory tracking
    wolfSSL_SetAllocators(TrackMalloc, TrackFree, TrackRealloc);
    wolfSSL_Init();
    #ifdef DEBUG
    wolfSSL_Debugging_ON();
    #endif
    
    // 2. Create Context for DTLS 1.3
    WOLFSSL_CTX *ctx = wolfSSL_CTX_new(wolfDTLSv1_3_client_method());
    if (!ctx)
        return;

    if (wolfSSL_CTX_set_cipher_list(ctx, "TLS13-CHACHA20-POLY1305-SHA256") != WOLFSSL_SUCCESS) {
        printf("Failed to set cipher list!\n");
    }

    // 3. Register Custom IO Callbacks
    wolfSSL_SetIORecv(ctx, EmbedReceive);
    wolfSSL_SetIOSend(ctx, EmbedSend);

    /* 
     * CERTIFICATE LOADING
     */
    printf("[CHECKPOINT 1] Loading Certificates & Keys...\n");

    // Load CA Certificate
    if (wolfSSL_CTX_load_verify_buffer(ctx, CA_der, CA_der_len, WOLFSSL_FILETYPE_ASN1) != WOLFSSL_SUCCESS){
        printf("CRITICAL: Failed to load CA Certificate buffer!\n");
    }else{
        printf(" -> CA Cert loaded.\n");
    }

    // Load Client Certificate
    if (wolfSSL_CTX_use_certificate_buffer(ctx, client_der, client_der_len, WOLFSSL_FILETYPE_ASN1) != WOLFSSL_SUCCESS){
        printf("CRITICAL: Failed to load Client Certificate buffer!\n");
    }else{
        printf(" -> Client Cert loaded.\n");
    }

    // Load Client Private Key
    if (wolfSSL_CTX_use_PrivateKey_buffer(ctx, client_key, client_key_len, WOLFSSL_FILETYPE_ASN1) != WOLFSSL_SUCCESS){
        printf("CRITICAL: Failed to load Client Key buffer!\n");
    }else{
        printf(" -> Client Key loaded.\n");
    }

    /* 
     * VERIFICATION CONFIGURATION
     */
    printf("[CHECKPOINT 2] Enabling Certificate Verification...\n");

    // Check the server's cert against our CA, Check Server MUST send a cert.
    wolfSSL_CTX_set_verify(ctx, WOLFSSL_VERIFY_PEER | WOLFSSL_VERIFY_FAIL_IF_NO_PEER_CERT, my_verify_cb);

    // Create SSL Object
    WOLFSSL *ssl = wolfSSL_new(ctx);
    // Set Post-Quantum Key Exchange (ML-KEM / Kyber)
    wolfSSL_UseKeyShare(ssl, WOLFSSL_KYBER_LEVEL1);

    printf("Starting Handshake...\n");

    flush_rx_queue(); // Clear queue before we begin handshake to avoid stale packets

    uint64_t hs_start_clocks = read_cycle64();
    
    in_handshake = 1;

    // --- HANDSHAKE LOOP ---
    while (1){
        int ret = wolfSSL_connect(ssl);

        if (ret == WOLFSSL_SUCCESS){
            printf("=======================================\n");
            break; // Handshake Done
        }

        int err = wolfSSL_get_error(ssl, ret);

        if (err == WOLFSSL_ERROR_WANT_READ || err == WOLFSSL_ERROR_WANT_WRITE){
            continue;
        }

        char errstr[80];
        wc_ErrorString(err, errstr);
        printf("Err: %d %s\n", err, errstr);
        break;
    }
    in_handshake = 0;

    // Handshake Metrics
    printf("Time taken (Peer cert verification): %lu ms (%llu cycles)\n", cycles_to_ms(dilith_end_clks - dilith_start_clks), dilith_end_clks - dilith_start_clks);
    printf("RAM (Peak heap usage): %lu bytes\n", (unsigned long)g_heap_peak);
    printf("RAM (Active session heap usage): %lu bytes\n", (unsigned long)g_heap_current);
    unsigned long freed_mem = (unsigned long)(g_heap_peak - g_heap_current);
    printf("Memory freed after handshake: %lu bytes\n", freed_mem);

    if (wolfSSL_is_init_finished(ssl)){
        printf("HANDSHAKE COMPLETED!\n\n");
        uint64_t hs_end_clocks = read_cycle64();
        printf("Time taken (handshake): %lu ms (%llu cycles)\n", cycles_to_ms(hs_end_clocks - hs_start_clocks), hs_end_clocks - hs_start_clocks);
        handshake_ms = cycles_to_ms(hs_end_clocks - hs_start_clocks);
        
        char *msg = "RISC-V Simulation complete\n\n"; // Send initial completion message
        wolfSSL_write(ssl, msg, strlen(msg));
        printf("MSG sent: %s", msg);
    }

    client_end_clocks = read_cycle64();
    printf("Time taken (whole client): %lu ms (%llu cycles)\n", cycles_to_ms(client_end_clocks - client_start_clocks), client_end_clocks - client_start_clocks);
    printf("Starting Throughput Test...\n");

    /* 
     * THROUGHPUT TEST
     */
    char test_buffer[CHUNK_SIZE];
    memset(test_buffer, 'A', CHUNK_SIZE);
    int bytes_to_send = THROUGHPUT_TEST_SIZE;

    t_data_start = read_cycle64(); // Start Timer for Data Phase

    while (bytes_to_send > 0){
        int current_sz = (bytes_to_send > CHUNK_SIZE) ? CHUNK_SIZE : bytes_to_send;

        int ret = wolfSSL_write(ssl, test_buffer, current_sz);
        if (ret <= 0){
            int err = wolfSSL_get_error(ssl, ret);
            if (err == WOLFSSL_ERROR_WANT_WRITE){
                continue;
            }
            printf("[THROUGHPUT FAIL] Write error: %d\n", err);
            break;
        }
        bytes_to_send -= ret;
    }

    t_data_end = read_cycle64(); // Stop Timer
    data_cycles = t_data_end - t_data_start;

    // Final message after the throughput test
    char *msg = "RISC-V Simulation complete\n";
    wolfSSL_write(ssl, msg, strlen(msg));

    // PRINT THE REPORT
    print_performance_report();

    // Cleanup
    wolfSSL_free(ssl);
    wolfSSL_CTX_free(ctx);
    wolfSSL_Cleanup();
}

/*             */
/* ENTRY POINT */
/*             */

int main(void)
{
    uart_init(); // 1. Basic Hardware Init
    printf("\n=== RISC-V IRQ Attached Boot ===\n");

    eth_init();

    udp_start(my_mac, my_ip); // 2. Setup UDP stack
    udp_set_callback(my_udp_rx);

    printf("Resolving ARP (Polling)...\n"); // 3. ARP Resolution (Polling Mode)
    if (udp_arp_resolve(host_ip) < 0){
        printf("ERROR: ARP Failed.\n");
        goto END;
    }
    
    // Process ARP replies
    for (int i = 0; i < 1000; ++i){
        udp_service();
    }

    // 4. ENABLE INTERRUPTS USING IRQ_ATTACH
#ifdef ETHMAC_INTERRUPT
    /* A. Clear all pending events in the SRAM WRITER (RX) */
    ethmac_sram_writer_ev_pending_write(ethmac_sram_writer_ev_pending_read());

    /* B. Enable the SRAM WRITER Interrupt at the Hardware Block level */
    ethmac_sram_writer_ev_enable_write(1);

    /* C. Configure the CPU Interrupt Controller (VexRiscv) */
    irq_setmask(irq_getmask() | (1 << ETHMAC_INTERRUPT));
    irq_attach(ETHMAC_INTERRUPT, eth_irq_handler);
    irq_setie(1); // Enable Global Interrupts

    printf("Interrupts Fully Enabled (RX Writer).\n");
#else
    printf("Error: ETHMAC_INTERRUPT not defined.\n");
#endif

    // 5. Run Application
    client_start_clocks = read_cycle64();
    run_dtls_client();

END:
    while (1)
        ;
    return 0;
}
