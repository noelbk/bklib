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


char *test_cert_1 =
"-----BEGIN CERTIFICATE-----\n"
"MIICpTCCAY2gAwIBAgIBATANBgkqhkiG9w0BAQQFADAWMRQwEgYDVQQDFAt0ZXN0\n"
"X2NlcnRfMTAeFw0wNDExMDQyMzQ3MDZaFw0wNTExMDQyMzQ3MDZaMBYxFDASBgNV\n"
"BAMUC3Rlc3RfY2VydF8xMIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEA\n"
"0g2xTodTOg3lDQiw8njkQ2fcfKBHWeeD7sRDffUmdEp3JtozIapr5QcpgGzJT9ku\n"
"Nh9F8sESGPnaJDzIy4K8pJg64pKAhcELUw2uqbdBrlrebaZ7W7xmQLOEu5NDI795\n"
"G3rsakKhUutLKyOTJ0rdQpMx4cMf4gO8iuHof05Dv0HXmYan3QK4heIHgAahz5Z4\n"
"vc/AcPUbffNcY5PmU0qBwQDedBGcNV2oME48tLwLXusq2NWPlubptio0NHu1GCIb\n"
"qTd466wlYDII+GqVIOAxAmv/8kaVgg3GWFx//sR+48ga8p81Hg9D5nJmTGIa1nGR\n"
"x0k/V8WpPp5sDApFdzCROwIDAQABMA0GCSqGSIb3DQEBBAUAA4IBAQACNyP2rNp4\n"
"oLAM3+1wzuaCtxn9+EQkBrfcSEjMZL2heojFBY9XENZkaDWGp2PiAhodeHwUL4dS\n"
"FHGm1X+AJ3Ueie+LOHkVdmWjX9fkYorbOinRhrCgkcCuEHJGyAmlTxbAGD1hkImA\n"
"TVZVJzncRjvvSkN6OWfF9x1AX+ZR13DlyTF255aYXh5Jg8J1LmL2Yp/TGKK37408\n"
"025ddjYVNPsVD7AyfJ8eMjjMinqgmJg11rfB0MlZo3jDBJdmOes81SKICUnPjO/5\n"
"mCBRurdDU4C6oLGHJvSetKsVBRIYHpZrgPP+TYnjtz87gvMzfFwnR5T/cj2e9WmC\n"
"9HuiTHOEIexm\n"
"-----END CERTIFICATE-----\n"
"-----BEGIN RSA PRIVATE KEY-----\n"
"MIIEpQIBAAKCAQEA0g2xTodTOg3lDQiw8njkQ2fcfKBHWeeD7sRDffUmdEp3Jtoz\n"
"Iapr5QcpgGzJT9kuNh9F8sESGPnaJDzIy4K8pJg64pKAhcELUw2uqbdBrlrebaZ7\n"
"W7xmQLOEu5NDI795G3rsakKhUutLKyOTJ0rdQpMx4cMf4gO8iuHof05Dv0HXmYan\n"
"3QK4heIHgAahz5Z4vc/AcPUbffNcY5PmU0qBwQDedBGcNV2oME48tLwLXusq2NWP\n"
"lubptio0NHu1GCIbqTd466wlYDII+GqVIOAxAmv/8kaVgg3GWFx//sR+48ga8p81\n"
"Hg9D5nJmTGIa1nGRx0k/V8WpPp5sDApFdzCROwIDAQABAoIBAHKsKvnWAueUolEF\n"
"R6R3vxXVY5dd4NYCABKfbsEazo16AaZiRLvCT/jOeie0PAqtY/8D/6nRIVIlRKO3\n"
"017W1ql8udjrZeKfStFijNH2S5Ml5Hw0APBJ4AC+gF3uvR6M707DQsGX8n5UGctJ\n"
"NpKi2jv4I6k+Xu+CXrHrNoQZc/66svl8ITpK/UuVidF9nu7jsWGMHsO/mnupM7yV\n"
"CDKla9Z5m5rgRqxoO4fj9JlXQ49NAIi1XiZj0MeI6hUOZ/lLILYQ7DIQIe9FtA8I\n"
"f6LUFsrJkWZeW5mzHhnxOASs0aqc8YUnX7s0jM0l0UxDgl701Q9pPl2vPoECIDjU\n"
"DE5CEOECgYEA/iRCyjsvq3Fe1bMImtPHAjAdoc3HKFexdUkXEPOh8iKlj3w0AfxF\n"
"YXXw52vbY+V8hgFc8hyJAmAwMvmt7yX3XyQhKmmDqm8twg+l/QVI+dpIMurkI5iN\n"
"zvehFdoH5FnR20RAhGutOfdyRIIZ1fnZXCtWfXUuAltEB+4vxNWAcHkCgYEA05bm\n"
"r2gCMVa4l1cp9ReGek7rWFLo0HOqPqRkYrhxfajg+2RBBT8DTC3s4MJyK8GcNaps\n"
"KK176yz6scc28qPELFj/y1/h2AG4OLySvvrWAzQZz5OZq1bw8utyxmO5tePpxOHd\n"
"6xRSYhLt/1gi3wZqW8mrBUQ4oQiG9Zq137VKalMCgYEAox//RvNqi67/YkfClC6v\n"
"JHZh7sO4u+pd25yIk7buEg+vGURt1YBpwD8F3GVcsuhQX2tsj3NFdiLrjSy6OjCy\n"
"cX0tVJ3Zh4JJarDEqH3F86nqgdRHj5mSHHDSEsaaHjaQ/x72EolS7UwpKlQf5Tq0\n"
"C/c9UtXuLFEMf9KhEsS/5nECgYEArN3BdJcuC29Feme9IL9qn+xwPIXoLb4B3hHi\n"
"7gCwwDBENz7uHEIiCJAQ0oQqUPNqEzI8EXPDJR1nZ+1RSCVh9bV0royUcCzffIMc\n"
"BSnbxVxApiV5y9FJI74W1FzPcfpVmAPMke3VbMj4ZsB0JPAR3xHBRpk5Bt93m5sd\n"
"cjqkJH8CgYEAjUSzXwUj6xp2qyPpeovgxoWHxNsiTyFsSvI+da6HDhASm1ypyPxt\n"
"Zr3Iw6aTEa5NTBAJDPwLPt3Y/fyiEVPN2g+wStx+XLtQMsNVYYxU/+WH53oT3Z1L\n"
"nunN9t2p/Fwf4r24ss+Mpw2wgos6eNxHKt1p7jdFP8wN3nqThcOI0aM=\n"
"-----END RSA PRIVATE KEY-----\n"
"\n"
;
int test_cert_1_len = 0;

char *test_cert_2 =
"-----BEGIN CERTIFICATE-----\n"
"MIICpTCCAY2gAwIBAgIBATANBgkqhkiG9w0BAQQFADAWMRQwEgYDVQQDFAt0ZXN0\n"
"X2NlcnRfMjAeFw0wNDExMDQyMzQ3MTJaFw0wNTExMDQyMzQ3MTJaMBYxFDASBgNV\n"
"BAMUC3Rlc3RfY2VydF8yMIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEA\n"
"tRx1nwGbzonWdRB7hlfRV7H9eR3tULWKLxzAxlDpFu85sc6qUnXRybtNQV7tIYMi\n"
"ccjS7fxq/dzIWu/+CxiIlNoLHY+tb5lzvu89OY2cMmCx6ufj/vrP1FpbchRl0wi7\n"
"Q8eJ/plHRRXEboA8Y6Aj7S/TKqhlp/PewDpraNuAMy7AsdaD71hMs96tBeTRirBh\n"
"X90O9sM4Mtv4dZH8JdL/LuJR5tijtixcvcgWiMpLVcpWpHM1Q2x5ed62Acqs1mxA\n"
"jQSqDcjJw7QEwBUAG5Y44rjM5ZuYzBCOHzD4byZxO5jylfXYfdiaoAGU24uS5DrV\n"
"dfc6Ct4Aih7nf1ckF5TpOQIDAQABMA0GCSqGSIb3DQEBBAUAA4IBAQCwOfeaSzpL\n"
"Xeg+4gzqt7oBZDRhsLPR63qAt+f4nitJJUgsyaKE6jULBr5K3N3S1Naetw2Rsbwg\n"
"h1xtXHJzPsuIbMPuhNXgL2cn/ZHhKeTAaFCvbvGyUtelXkWTLLCmRl6xZZDTCs28\n"
"zJJDVkOdE+U2+CgCvegicmtFn8gMcg+03FEUQc8ES1ef3PTNFCfrK9OF4IlxoMyL\n"
"ZpsJFdHxtEBFI2uIZiD/nrjGNa1XhP52sFWNAYdr6L8KKjNiBTMy3TaVnlXPP7LA\n"
"kprAm9VhmEubvZJAykVktijDMbHBUcljvRzmOlfEdF5fyXD4Rvt6P2abXT+Q+fwr\n"
"iF2oo/2tEtQS\n"
"-----END CERTIFICATE-----\n"
"-----BEGIN RSA PRIVATE KEY-----\n"
"MIIEpAIBAAKCAQEAtRx1nwGbzonWdRB7hlfRV7H9eR3tULWKLxzAxlDpFu85sc6q\n"
"UnXRybtNQV7tIYMiccjS7fxq/dzIWu/+CxiIlNoLHY+tb5lzvu89OY2cMmCx6ufj\n"
"/vrP1FpbchRl0wi7Q8eJ/plHRRXEboA8Y6Aj7S/TKqhlp/PewDpraNuAMy7AsdaD\n"
"71hMs96tBeTRirBhX90O9sM4Mtv4dZH8JdL/LuJR5tijtixcvcgWiMpLVcpWpHM1\n"
"Q2x5ed62Acqs1mxAjQSqDcjJw7QEwBUAG5Y44rjM5ZuYzBCOHzD4byZxO5jylfXY\n"
"fdiaoAGU24uS5DrVdfc6Ct4Aih7nf1ckF5TpOQIDAQABAoIBAH+SO1u0U15pgxFU\n"
"ed4Ib6IY8tVkeiw2o85Jr5RBm94Wxgnz689HxG0XrPRV/Cx15UZG0iTmfyqwYMSF\n"
"dem070gCSoAZG26XpYshk3u9vv9RUyGHE/cgDlKjDvTNX2gGJ3o3zID00Nnd2k6H\n"
"r4HxZD0HSfQrR+fFBOtcexoSOzxRC/c99h3/uPo+6DH0+/qgBcOVZreq9xxIdET1\n"
"evDRzw8Mnh1mjPWaWhzKJSQvlJaiuzBXiVkuH//kY1PBDeued9Y5qhF9ozGwSC79\n"
"RZouEwk0D3JRzFwW39T7kC/HBLQ1zZVcNkNywaeRD6aaxzUI5gJqIPbVUHccvOtD\n"
"L1YWq/ECgYEA2cP4AyWrL1A2Ul49TT82PZber6AToMskOr5euvz2gsWaqywLTcP5\n"
"CA60rfZlSHhLupfxyJuOiLtjiX8LjyO2xB2/4jE/iW97X8Z4Iqi8odX19nj4gj9M\n"
"bsrYWWd+xLR30D5JyKj2kRaDQpN1ReJFIfDaFgv0JBmzXi3ooQPOPk0CgYEA1Oj3\n"
"n1xYpRV86zZB8zgsSI3utFt5Nnvg1303l3rxTgG5SNqvdx5rTFkeO9sqQGoM/Mee\n"
"vAaEHMDrRrq4rIJjZHwroUNyZOq1a58NFl8SGozipLNhaH3lqv7QkLna5rXuY0e3\n"
"+c50nfqhUA9GpH7cqxbHSZ9lJ23yE8TKPOhdhJ0CgYEAkySaoa3DRN4DRpbM/zQ2\n"
"N1mJMMyOVrjMyzC1G84v118kSoMhRDOmQu6E4YM7HsPCD9v5kN6jafqMoGyK5/Zu\n"
"2Ydsj69NSlVc3PNjW6/+fa/wWdGVwZR51ecUVxzaiJmU184u1vpsWM8IdGTk+gqR\n"
"QgTXp8v99KK1E28b4CiNJIUCgYApXnLu1gSrXvd8xHggCCQinOFqzfNJcNyJQl1Q\n"
"46SLYa5cx5Ecdh6nIRHM05LsHS8U1ryzEBkPdTTB83n6mwMw/SeFGBPcI/bISIlE\n"
"dYiQ/Br1oZ0lroC3rvdbh6FO/r6pL8BBm6nxIEfDxJQXVkcOP0jNt1jyRlypFq2t\n"
"FAFHyQKBgQCVCCMBlAr8a6D5psQHPovKV/lpdSafRNmei8Qk437eaURS6t8eiTm2\n"
"BnR7LzzcZq3ZzHmTVG9ESH8KEZ2sCx0DM9AizDtdc2fomV5nEuGUvAG+chiVeOnA\n"
"csOwKmTBAh4fRUEcOVW37gebjCeLuOAJHAIQJcKWIJPMdyKHsWM0mw==\n"
"-----END RSA PRIVATE KEY-----\n"
"\n"
;
int test_cert_2_len = 0;

char *test_cert_3 =
"-----BEGIN CERTIFICATE-----\n"
"MIICpTCCAY2gAwIBAgIBATANBgkqhkiG9w0BAQQFADAWMRQwEgYDVQQDFAt0ZXN0\n"
"X2NlcnRfMzAeFw0wNDExMDQyMzQ3MTdaFw0wNTExMDQyMzQ3MTdaMBYxFDASBgNV\n"
"BAMUC3Rlc3RfY2VydF8zMIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEA\n"
"3lzHY2q8miT4nm/soGDbgnwt1TuicT0AtZmuJjeokgjDD/zax1zrCns0z2VMr4HN\n"
"tqzXkaBho4jZmukybDdi8O9t3SKD2WGiVpRiNm3do2I9f9vNN3AzVGWDVy7WD6xo\n"
"l7S0108IGo/qHkFohl2fiQESNE7eewU/zxtS0jI78OeL0Fx90QBlXZEQI18PmRSV\n"
"szsx+y5GyJIHvidatfQyowwkGFS2DH82gvEe52wNLS+6GmkQU15SoNEaOm+I2Mcy\n"
"C4wp5K/nfJRqKcNCvbeXV9qJbtmZfF2KTzoQ3Z5W+E+f3Tu1xWanXY7/HrovNHY3\n"
"DkjNhDYWL5JzhsFpQbQ9CwIDAQABMA0GCSqGSIb3DQEBBAUAA4IBAQA5PPj0Sdz0\n"
"Eo0Ok06NAghSE9pPiOdwPxuZ53rOOkpETpUb+ZyAxBCXutnoCBuDJC9M3e5c7WTM\n"
"BRviFHQafRlAF6SWyDAfKeej7+3ltRgLlP6xAN9nzvvCATfEI8DNruf4rKgOzrU7\n"
"h0MhJ2Ab2nE2q1kDHWK2n3yqYyhwI02PAJnYF+JEja1ePheDDTVWXxGNQ5Tla7L+\n"
"n8Mvd+RpT8APM+bvFXsx/ZWQsxXgziQHWEdBkeYsWIFnpzMPBvwFaW0cB763bFY3\n"
"iCWbIg4Azrk2yne1djIC+1XOhN2617cEnDErQsYpDZa3uejE/jNCu6L0HgQ9UUz7\n"
"0+UrVAEEllki\n"
"-----END CERTIFICATE-----\n"
"-----BEGIN RSA PRIVATE KEY-----\n"
"MIIEpQIBAAKCAQEA3lzHY2q8miT4nm/soGDbgnwt1TuicT0AtZmuJjeokgjDD/za\n"
"x1zrCns0z2VMr4HNtqzXkaBho4jZmukybDdi8O9t3SKD2WGiVpRiNm3do2I9f9vN\n"
"N3AzVGWDVy7WD6xol7S0108IGo/qHkFohl2fiQESNE7eewU/zxtS0jI78OeL0Fx9\n"
"0QBlXZEQI18PmRSVszsx+y5GyJIHvidatfQyowwkGFS2DH82gvEe52wNLS+6GmkQ\n"
"U15SoNEaOm+I2McyC4wp5K/nfJRqKcNCvbeXV9qJbtmZfF2KTzoQ3Z5W+E+f3Tu1\n"
"xWanXY7/HrovNHY3DkjNhDYWL5JzhsFpQbQ9CwIDAQABAoIBABoNCJcPJDVy62W0\n"
"eyZ28oChB8WZjPhzSyf7bzmu/6LMP/4Zg5AjxK2frZo2pftR28XxDfhTr9y25beO\n"
"ZofjHUZY5qgnqQbIt3opPDBgQr0nf0kBeVNrY7gZhH+sTwvbCQ3s79rhuXNTNPyC\n"
"cTp8sXodlAJo0wvam5UqAFP76qXYYtLALYGEmp+M3tTivmb0HaP+oiuqywog2m/W\n"
"7Zz2kZuRWduTpXs4dQcoN0ymXBF+kGLco6UDsIziP4QV6Mr7H+T9NGPF7APVQqMS\n"
"56OfrfUV7V53SACN9jFgZAagvx6U0N4qdnmSqCfcqghy3gC8a/U/CXFyAla8n3e+\n"
"IXvKLzECgYEA8LlKu0RojzjTOeUtSsPZNBlHCG9ku2bVQf+llUyKEoVmLQEX11GA\n"
"vbM5xZt75OX2a6u03KZ2rpk+UClFwdFwRrfubhg79wllVhol2YkFwuFzUmr7qHeg\n"
"pD8jS98FMk0VTSutwngMDyNm/Xu2c6+VPyxn+3riWTZHOVf9+YE6yScCgYEA7Hkx\n"
"5KKHS01ed0sDDA/DZZlHHJls5ZMWT2xwps6zl5vqeAbZm+JW9A5spcEdAwG3z1oQ\n"
"ecWZoosdR1rvVTxsOpwyywlkRowUygfUAFxk9QulWWk0e1OgVT9IIrfjK8KAsWs5\n"
"o/JtAiu5pdeqOjtg+n58Xw6dB/ftmJhWbxZw830CgYEAlqujdusudtaefxJO8wj6\n"
"nn1j1IfEiPeoa5LT1Ur+8PDGL00L2x2MczAG7QSQ24iEYIgswB8BEe+g7ROoMdQh\n"
"bmIZvpc0kT80Slw7bJkzHfN29xM3SQ73YVaThR2mZwGQX6kDLUFuattBz07ruCUi\n"
"SVRGEUWs/yHWGrJUqSKV5OkCgYEA1BxNvZhvpQLsJjL5s7r7+MkJOUUBoddSnsFV\n"
"NrqX6vjm0uzKCoJ2nonFuNREkqra1joiJygwi9Ue2R9fKGR/a/8prGdrM8B48VMe\n"
"PYii4g6lLVMpySmU2oJd0Q6CzYC08O+OD1nnn5fa+UA+p+cVMPt90cII5tnjnAmh\n"
"FhnaDP0CgYEAqzNWky0LvK07YsBUGo7o4QODOuPeeGXA2lpEFj5OAqRQf+BT9OAM\n"
"z1jTCXPveVys7NySS/BYSIqFl68Hy+upju0JFKKyYP5YgG5qO6veCTRFG7IWu1kL\n"
"XFS6uoX/fWmfSyojtHGHw+iIDdofz2dQRb4bHljxekkzQPMCTLHmi5Y=\n"
"-----END RSA PRIVATE KEY-----\n"
"\n"
;
int test_cert_3_len = 0;

char *test_cert_4 =
"-----BEGIN CERTIFICATE-----\n"
"MIICpTCCAY2gAwIBAgIBATANBgkqhkiG9w0BAQQFADAWMRQwEgYDVQQDFAt0ZXN0\n"
"X2NlcnRfNDAeFw0wNDExMDQyMzQ3MjRaFw0wNTExMDQyMzQ3MjRaMBYxFDASBgNV\n"
"BAMUC3Rlc3RfY2VydF80MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEA\n"
"rx+XCzdca11pabh5V3h5og+duxsCGvL5ys6nj128ZMWnKtMyKe+N7QmFwFi2lEVx\n"
"kMaDwes+u69kNisCkJjxX6IQtoKP6R13BnKbuWhnX2n0NzHTGeJxxsYZr/KzV8dj\n"
"ZmNfF7FJhnsOFjPShHvjS7q5SzTMLrKeOI21bL6f9dxYOBNbz9SGfJb0aqU346BM\n"
"Kne5476RDyD3IymjN27n7giyGVQizfQL91UkYU62Wm8bcytc2mfdziEtW997S4c2\n"
"ObRhW4rvrjPsanSRSpBqw46s0gBZZNL6HLujSyxWoJdrXYo98Gy9158mzCEg14pn\n"
"Yj6Pbtte7+w58V9u4z60fQIDAQABMA0GCSqGSIb3DQEBBAUAA4IBAQBNnB+56p4j\n"
"JecQvbCZY2Jz5Ce0laTt3xHIjjTpgsMXX2mDq6ZSQEolGYNnLMaKRz6cKvCMZ8z1\n"
"B5Yk3qZsSUnZ4c1zA2GnxD+tLbAeVgBAay31+Gtq71YIVhLXcTjGn5rCsJs5vJ81\n"
"ZrmOahGinb7FZ2fBYKACqBEYFB8hmjHxJUeeJ4j8qsA75fhTP7pw5Md9qPJ98xnS\n"
"SDAaHBTy/QGy2PY3T+P7lNkV6biaP3H03+EisGLX8MM01xE9ztse1493EAMMWhTr\n"
"vqTe11g+7tuQ1EmZwrMe75ttYsGHml1C1eOP3wE9ivzCYEbna79LGHk+zgbamF2v\n"
"yHHEZacYUqXk\n"
"-----END CERTIFICATE-----\n"
"-----BEGIN RSA PRIVATE KEY-----\n"
"MIIEpAIBAAKCAQEArx+XCzdca11pabh5V3h5og+duxsCGvL5ys6nj128ZMWnKtMy\n"
"Ke+N7QmFwFi2lEVxkMaDwes+u69kNisCkJjxX6IQtoKP6R13BnKbuWhnX2n0NzHT\n"
"GeJxxsYZr/KzV8djZmNfF7FJhnsOFjPShHvjS7q5SzTMLrKeOI21bL6f9dxYOBNb\n"
"z9SGfJb0aqU346BMKne5476RDyD3IymjN27n7giyGVQizfQL91UkYU62Wm8bcytc\n"
"2mfdziEtW997S4c2ObRhW4rvrjPsanSRSpBqw46s0gBZZNL6HLujSyxWoJdrXYo9\n"
"8Gy9158mzCEg14pnYj6Pbtte7+w58V9u4z60fQIDAQABAoIBAB/PzD9UWiyCtoBm\n"
"61vxXZ8L2SpA+LvugWPO8G9BptRjdz93R/iINK7neO188AW1K7ER7qNOWYH8lWal\n"
"n3ym28n0hbaxtvSwzotUTGd8yLJza9KO8XsVQHfMSqREoUGDzdGuoXqLUQDl36Q2\n"
"27oHSz3ZeIlp2OFuvwEk9N0BL8uYY1RQGsJTA5lw0YZimw2+gqk4lgU9LHvEyDQD\n"
"lD9MgUFMxVTFrX4fn7TztJbREhsPWponSR11RWr34soOPK/69r+AVwQafS64r2om\n"
"bdIcKZi+uk5o5xTDIcR0rdWGSASXO3k9OMnpJidQghNwXlp/nE1UV980RAf4Cybt\n"
"H/sm7x0CgYEA2i1WDaV5ZMR2Magc7FkBgCxnpHDIiU5UDA8lgYgc2Iozj7cuaSgj\n"
"GJXEEgJ+XQsm9oQjjoq81zWc5JXRwd6UGbMpYoC1xWZWVklCc2cCZrtTNKWDWvht\n"
"KIL7PAL0ksXnMZ5aiSi87ze9eniCibjgA/hjsH97z6RqVwKg4LPh7EcCgYEAzXuJ\n"
"sfnOSPQXql7Wy5DzpCbvkLCWC1hwHAZSszO+rypq3yFK2y0SN7qEoY0pT5DWGLrz\n"
"hbL8GM3cTbiBe8qrciSMnmFZIFzxIVjqNZ4DN218UrYrzqvUJGXwm7QTxyWk+lKZ\n"
"kVFam8035OHVpNeIVLpUNg5FXQO6EptN9vdIbxsCgYEAwa2xUJfutbd5vQch9eXl\n"
"YSUKNH4yZ0FzdukTQHPWN5wnvZK9znfk//QjhGFWT76QrhILhoQ+s0VSq9EbzeCK\n"
"1G2pdVoRAtWgGyUZqlxtU0lDyd6MRvt125q68EEnYAvy+qc6c7wITMvMlqXYK+5E\n"
"XwHnbU0eWev4wpkAu7iz/A0CgYAjt9W3mjTBeayjcNjliczQO/Rosklir3zNYkv9\n"
"2oDK8hShtKfOcYc5KLLlYiRMMGEG63hpRTONHDuvRuM7IX+r166Z2VIkzgMcSNht\n"
"eYoVmHKD7iavRi8aJJ4ucp79fw2uda08FWgqrQGKR7dbcBvCnqEExHZKNNujYFut\n"
"Ek8FAQKBgQCI+LSY7asm6z1yj83hCnu5K1/FlbvcArYkUWz9axdAmoUvsCj5uz2a\n"
"WVllrmFMAERvg2E5HJJeXQixg81YEuF+y8B5C5RsFfrE5uEumK3TD4IFL07vNMf+\n"
"uaHjGapXRlx5x45SOh3y9VQ0JPbTAh60FzxuzeOKmYbher9YYaBG2w==\n"
"-----END RSA PRIVATE KEY-----\n"
"\n"
;
int test_cert_4_len = 0;
