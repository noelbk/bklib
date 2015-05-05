/*
i=1; 
while [ $i -lt 5 ]; do 
    ( 
	echo "char *test_cert_$i ="
	./cryptbk_selfsign_t "CN=test_cert_$i" | 
	    sed -e 's/^/"/' -e 's/$/\\n"/';
	echo ";"
	echo "int test_cert_${i}_len = sizeof(test_cert_$i);"
	echo
    ) 
    i=$(($i+1))
done
*/


extern char *test_cert_1;
extern int test_cert_1_len;

extern char *test_cert_2;
extern int test_cert_2_len;

extern char *test_cert_3;
extern int test_cert_3_len;

extern char *test_cert_4;
extern int test_cert_4_len;

