#include <stdio.h>
#include <openssl/rsa.h>
#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/buffer.h>
#include <openssl/rand.h>
#include <openssl/blowfish.h>

void 
gen_cb(int code, int n, void *arg) {
    int c;
    switch(code) {
    case 0: c = '.'; break;
    case 1: c = '1'; break;
    case 2: c = '2'; break;
    case 3: c = '3'; break;
    default: c = '?'; break;
    }
    printf("%c", c);
    fflush(stdout);
}

void
print_bin(FILE *f, char *p, int n) {
    for(;n>0; n--, p++) {
	fprintf(f, "%02x", *p);
    }
}

int
main() {
    BIO *bio;
    RSA *rsa=0, *rsa_pub, *rsa_priv=0;
    char buf[4096], enc[4096], dec[4096];
    BUF_MEM *ptr;
    int len, enc_len, dec_len;
    int i;
    BF_KEY key;
    char ivec_orig[8] = {1, 2, 3, 4, 5, 6, 7, 8}, ivec[8];

    
    do {
        //OpenSSL_add_all_algorithms();
        //OpenSSL_add_all_ciphers();
        //OpenSSL_add_all_digests();

	printf("generate_key:\n");
	rsa = RSA_generate_key(1024, 3, gen_cb, 0);
	printf("\n");

	printf("pem public key:\n");
	bio = BIO_new(BIO_s_mem());
	i = PEM_write_bio_RSAPublicKey(bio, rsa);
	BIO_flush(bio);
	i = BIO_get_mem_ptr(bio, &ptr);
	printf("pem public key len=%d\n", ptr->length);
	fwrite(ptr->data, ptr->length, 1, stdout);
	len = ptr->length;
	memcpy(buf, ptr->data, len);
	BIO_free(bio);
	printf("\n");

	bio = BIO_new_mem_buf(buf, len);
	rsa_pub = PEM_read_bio_RSAPublicKey(bio, 0, 0, 0);
	BIO_free(bio);

	printf("pem private key:\n");
	bio = BIO_new(BIO_s_mem());
	i = PEM_write_bio_RSAPrivateKey(bio, rsa, 0, 0, 0, 0, 0);
	BIO_flush(bio);
	i = BIO_get_mem_ptr(bio, &ptr);
	printf("pem private key i=%d len=%d\n", i, ptr->length);
	fwrite(ptr->data, ptr->length, 1, stdout);
	len = ptr->length;
	memcpy(buf, ptr->data, len);
	BIO_free(bio);
	printf("\n");

	bio = BIO_new_mem_buf(buf, len);
	rsa_priv = PEM_read_bio_RSAPrivateKey(bio, 0, 0, 0);
	BIO_free(bio);

	/* encrypt */

	printf("buffer:\n");
	len = sprintf(buf, "1234567890123456");
	len = 128/8;
	RAND_bytes(buf, len);
	printf("buf_len=%d\n", len);
	//printf("%s", dec); 
	for(i=0; i<len; i++) { 
	    printf("%02x", (unsigned int)buf[i]&0xff); 
	}
	printf("\n");
	

	printf("public_encrypt:\n");
	memset(enc, 0, sizeof(enc));
	enc_len = RSA_public_encrypt(len, buf, enc, rsa_pub, RSA_PKCS1_OAEP_PADDING);
	if( enc_len < 0 ) {
	    printf("err=%ld\n", ERR_get_error());
	    break;
	
	}
	printf("enc_len=%d\n", enc_len);
	for(i=0; i<enc_len; i++) { 
	    printf("%02x", (unsigned int)enc[i]&0xff); 
	}
	printf("\n");


	printf("public_decrypt:\n");
	memset(dec, 0, sizeof(dec));
	dec_len = RSA_private_decrypt(enc_len, enc, dec, rsa_priv, RSA_PKCS1_OAEP_PADDING);
	if( dec_len < 0 ) {
	    printf("err=%ld\n", ERR_get_error());
	    break;
	
	}
	printf("dec_len=%d\n", dec_len);
	for(i=0; i<dec_len; i++) { 
	    printf("%02x", (unsigned int)dec[i]&0xff); 
	}
	printf("\n");

	// blowfish
	BF_set_key(&key, dec_len, dec);

	i = 0;
	memcpy(ivec, ivec_orig, 8);
	memset(buf, 0, sizeof(buf));
	BF_cfb64_encrypt(enc, buf, enc_len, &key, ivec, &i, BF_ENCRYPT);

	printf("BF_cfb64_encrypt:\n");
	for(i=0; i<enc_len; i++) { 
	    printf("%02x", (unsigned int)buf[i]&0xff); 
	}
	printf("\n");

	i = 0;
	memcpy(ivec, ivec_orig, 8);
	memset(enc, 0, sizeof(buf));
	BF_cfb64_encrypt(buf, enc, enc_len, &key, ivec, &i, BF_DECRYPT);

	printf("BF_cfb64_decrypt:\n");
	for(i=0; i<enc_len; i++) { 
	    printf("%02x", (unsigned int)enc[i]&0xff); 
	}
	printf("\n");

    } while(0);

    if( rsa ) {
	RSA_free(rsa);
    }


    return 0;
}

int
p2p_msg_pack_crypt(p2p_crypt *crypt, p2p_msg *msg, char *buf, int max) {
    int len;
    char raw[P2P_MSG_PACK_MAX], enc[P2P_MSG_PACK_MAX];
    p2p_msg msg_enc;
    
    len = p2p_msg_pack(msg, raw, sizeof(raw));

    len = p2p_crypt_enc(crypt, raw, len, enc, sizeof(enc));
    // BF_cfb64_encrypt(raw, enc, len, &key, ivec, &i, BF_ENCRYPT);

    p2p_msg_init(&msg_enc, P2P_MSG_ENC); {
	p2p_msg_enc *m = msg_enc.p2p_msg_u.enc;
	m->enc.enc_val = enc;
	m->enc.enc_len = len;
    }
    len = p2p_msg_pack(msg_enc, buf, max);
    return len;
}


/* 
Local Variables:
compile-command: "cc -g -Wall -o ssl_rsa_t ssl_rsa_t.c -lcrypto"
End:
*/

