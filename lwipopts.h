#ifndef _LWIPOPTS_H
#define _LWIPOPTS_H

// --- CONFIGURAÇÃO PARA FREERTOS ---
#ifndef NO_SYS
#define NO_SYS                      0 
#endif

#define LWIP_NETCONN                1
#define LWIP_SOCKET                 1

// --- FIX PARA O ERRO DE REDEFINIÇÃO DE TIMEVAL ---
#define LWIP_TIMEVAL_PRIVATE        0 

// Definições de Thread (Obrigatórias para NO_SYS 0)
#define TCPIP_THREAD_STACKSIZE      2048
#define TCPIP_THREAD_PRIO           5
#define DEFAULT_THREAD_STACKSIZE    1024
#define DEFAULT_THREAD_PRIO         1
#define LWIP_TCPIP_CORE_LOCKING     1

// Tamanhos de Mailbox (evita assert "size > 0" em sys_mbox_new)
#ifndef TCPIP_MBOX_SIZE
#define TCPIP_MBOX_SIZE             32
#endif
#ifndef DEFAULT_TCP_RECVMBOX_SIZE
#define DEFAULT_TCP_RECVMBOX_SIZE   6
#endif
#ifndef DEFAULT_UDP_RECVMBOX_SIZE
#define DEFAULT_UDP_RECVMBOX_SIZE   6
#endif
#ifndef DEFAULT_RAW_RECVMBOX_SIZE
#define DEFAULT_RAW_RECVMBOX_SIZE   6
#endif
#ifndef DEFAULT_ACCEPTMBOX_SIZE
#define DEFAULT_ACCEPTMBOX_SIZE     6
#endif

// --- MEMÓRIA E PERFORMANCE ---
#define MEM_ALIGNMENT               4
#define MEM_SIZE                    16000 
#define MEMP_NUM_TCP_SEG            32
#define MEMP_NUM_ARP_QUEUE          10
#define PBUF_POOL_SIZE              24

// --- PROTOCOLOS ---
#define LWIP_ARP                    1
#define LWIP_ETHERNET               1
#define LWIP_ICMP                   1
#define LWIP_RAW                    1
#define LWIP_DHCP                   1
#define LWIP_IPV4                   1
#define LWIP_TCP                    1
#define LWIP_UDP                    1
#define LWIP_DNS                    1
#define LWIP_TCP_KEEPALIVE          1
#define LWIP_NETIF_STATUS_CALLBACK  1
#define LWIP_NETIF_LINK_CALLBACK    1
#define LWIP_NETIF_HOSTNAME         1
#define LWIP_NETIF_TX_SINGLE_PBUF   1

// --- SEGURANÇA (TLS/MQTT) ---
// Como você está usando a porta 1883, o TLS não é necessário.
// Desativar estas opções resolve o erro de compilação do SDK.
#define LWIP_ALTCP                  0 
#define LWIP_ALTCP_TLS              0 
#define LWIP_ALTCP_TLS_MBEDTLS      0 

// FIX CRÍTICO: Desativa o cache que causa o erro 'mbedtls_ssl_session'
#define LWIP_ALTCP_TLS_SESSION_CACHE 0 

// Necessário para o mbedtls 
#define MEMP_NUM_SYS_TIMEOUT        (LWIP_NUM_SYS_TIMEOUT_INTERNAL + 1)

#ifndef NDEBUG
#define LWIP_DEBUG                  1
#define LWIP_STATS                  1
#define ALTCP_MBEDTLS_DEBUG         LWIP_DBG_OFF
#define MQTT_DEBUG                  LWIP_DBG_ON
#endif

#endif