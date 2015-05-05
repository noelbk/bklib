//
 0001:0000004d       _print_cert                0040104d f   cryptbk_t.obj
 0001:00000277       _enc_test                  00401277 f   cryptbk_t.obj
      //

int
main() {
     
     // encrypt a function in an executable
     cryptbk_seskey(crypt, pass, -1);
     fread(raw, raw, 1, len, f); 
     cryptbk_enc(crypt, raw, len, enc, len, 
		 CRYPTBK_MODE_ENC | CRYPTBK_MODE_SESKEY);
     fwrite(enc, 1, len, f); 
     
     cryptexe_dec(func, "func");
}
