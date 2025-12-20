#ifndef USER_SETTINGS_H
#define USER_SETTINGS_H

#include <sys/types.h>

//#define DEBUG

/*
 * ARCHITECTURE & OS SETTINGS
 */
#define SINGLE_THREADED
#define NO_FILESYSTEM
#define NO_WRITEV
#define NO_MAIN_DRIVER
#define WOLFSSL_USER_IO
#define WOLFSSL_NO_SOCK
#define WOLFSSL_NO_CLOCK
#define NO_TIME_H
#define USER_TICKS
#define NO_ASN_TIME

/*
 * 2. MATH CONFIGURATION
 */
#define USE_FAST_MATH

/* 
 * 3. MEMORY OPTIMIZATIONS
 */
#define WOLFSSL_SMALL_STACK
#define WOLFSSL_SMALL_CERT_VERIFY
#define NO_ERROR_STRINGS
#define WOLFSSL_SMALL_SESSION_CACHE

/* 
 * 4. TLS 1.3 CONFIGURATION
 */
#define WOLFSSL_TLS13
#define WOLFSSL_DTLS
#define WOLFSSL_DTLS13
#define HAVE_TLS_EXTENSIONS
#define HAVE_SUPPORTED_CURVES
#define HAVE_SESSION_TICKET
#define HAVE_HKDF
#define WC_RSA_PSS
#define WOLFSSL_SEND_HRR_COOKIE
#define WOLFSSL_DTLS_CH_FRAG
#define NO_OLD_TLS
#define WOLFSSL_NO_TLS12
#define WOLFSSL_USE_ALIGN
#define WOLFSSL_DTLS_MTU

/* 
 * 5. ALGORITHMS (AES + PQC)
 */

/* Standard algorithms ("AES-GCM-SHA256") */
#define HAVE_AESGCM
#define WOLFSSL_SHA256
#define WOLFSSL_SHA384
#define HAVE_ECC
#define HAVE_CURVE25519
#define HAVE_KYBER
#define HAVE_CHACHA  
#define HAVE_POLY1305

/* Post-quantum algorithms (Kyber/ML-KEM/ML-DSA) */
#define WOLFSSL_HAVE_KYBER
#define WOLFSSL_WC_MLKEM
#define WOLFSSL_HAVE_MLKEM
#define WOLFSSL_SHA3
#define HAVE_FFDHE_2048
#define WOLFSSL_SHAKE256
#define WOLFSSL_SHAKE128
#define WOLFSSL_MLKEM_KYBER
#define HAVE_DILITHIUM
#define WOLFSSL_WC_DILITHIUM
#define WOLFSSL_DILITHIUM_LEVEL2

/* 
 * 6. DEBUGGING & RNG
 */
#ifdef DEBUG
#define DEBUG_WOLFSSL
#define WOLFSSL_LOG_PRINTF
#endif

/* Optimizations*/

/* 
 * 1. Disable public key algorithms
 */
#define NO_RSA
#define NO_ECC
#define NO_DH 
#define NO_DSA  
#define NO_ED25519  
#define NO_CURVE25519 
#define NO_SRP

/* 
 * 2. Disable legacy/unused/symmetric ciphers
 */
#define NO_DES3
#define NO_RC4 
#define NO_RABBIT   
#define NO_HC128    
#define NO_IDEA     
#define NO_SEED     
#define NO_AES_CBC  
 #define NO_CAMELLIA

/* 
 * 3. Disable unused hashing algorithms
 */
#define NO_MD5         
#define NO_SHA         
#define NO_MD4         
#define NO_RIPEMD      
#define NO_BLAKE2      
#define WOLFSSL_NO_SM3 
#define NO_CMAC        

/* 
 * 4. Disable other protocol versions
 */
#define WOLFSSL_NO_TLS12 
#define NO_OLD_TLS       
#define WOLFSSL_NO_SSL3  

/* 
 * 5. Disable certificate/key features
 */
#define NO_PKCS7                        
#define NO_PKCS12                       
#define NO_PWDBASED                     
#define NO_OCSP                         
#define NO_CRL                          
#define WOLFSSL_IGNORE_NAME_CONSTRAINTS 

/* 
 * 6. System & misc optimizations
 */
#define NO_FILESYSTEM    
#define NO_WRITEV        
#define NO_MAIN_DRIVER   
#define NO_ERROR_STRINGS 
#define SINGLE_THREADED  
#define NO_SESSION_CACHE 

/* 
 * 7. Encoding optimizations
 */
#define NO_CODING     
#define WOLFSSL_NO_PEM

/* 
 * RNG Hook
 */
extern int CustomRngGenerateBlock(unsigned char *, unsigned int);
#define CUSTOM_RAND_GENERATE_SEED  CustomRngGenerateBlock
#define CUSTOM_RAND_GENERATE_BLOCK CustomRngGenerateBlock


#endif /* USER_SETTINGS_H */