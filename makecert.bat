openssl req -config req.cfg -x509 -out cert.pem -newkey rsa:1024 -keyout key.pem -passout pass:mypass
move /y key.pem key.pem.org
openssl rsa -in key.pem.org -out key.pem -passin pass:mypass
openssl verify cert.pem
move /y cert.pem ..\etc\kdesk.cert
move /y key.pem ..\etc\kdesk.key
