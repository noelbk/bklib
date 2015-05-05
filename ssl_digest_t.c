// -*- compile-command: "cc -g -Wall -o ssl_digest_t ssl_digest_t.c -lcrypto";  -*-

#include <stdio.h>
#include <openssl/evp.h>

int
main(int argc, char *argv[])
{
    EVP_MD_CTX mdctx;
    const EVP_MD *md;
    char mess1[] = "Test Message\n";
    char mess2[] = "Hello World\n";
    unsigned char md_value[EVP_MAX_MD_SIZE];
    int md_len, i;

    OpenSSL_add_all_digests();

    if(!argv[1]) {
	printf("Usage: mdtest digestname\n");
	exit(1);
    }

    md = EVP_get_digestbyname(argv[1]);

    if(!md) {
	printf("Unknown message digest %s\n", argv[1]);
	exit(1);
    }

    EVP_DigestInit(&mdctx, md);
    EVP_DigestUpdate(&mdctx, mess1, strlen(mess1));
    EVP_DigestUpdate(&mdctx, mess2, strlen(mess2));
    EVP_DigestFinal(&mdctx, md_value, &md_len);

    printf("digest=%s len=%d: ", argv[1], md_len);
    for(i = 0; i < md_len; i++) printf("%02x", md_value[i]);
    printf("\n");
    
    return 0;
}
