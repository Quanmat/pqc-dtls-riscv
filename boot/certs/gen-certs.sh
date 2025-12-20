#!/bin/bash

export LD_LIBRARY_PATH="../../liboqs/build/lib:$LD_LIBRARY_PATH"
OQS_PROVIDER_PATH="../../oqs-provider/_build/lib"
PROVIDER_FLAGS="-provider-path ${OQS_PROVIDER_PATH} -provider oqsprovider"

openssl list -signature-algorithms $PROVIDER_FLAGS > /dev/null || {
    echo "WARNING: oqsprovider not loading..."
    PROVIDER_FLAGS=""
}

ALG="mldsa44"
DAYS=365

CA_SUBJ="/CN=My PQ CA"
SRV_SUBJ="/CN=test-server"
CLI_SUBJ="/CN=test-client"

[ "$1" = "clean" ] && rm -f *.crt *.der *.h *.key *.csr *.srl *.pem 2>/dev/null && exit

echo "Generating PQC Certificates using $ALG..."

# CA
openssl genpkey -algorithm "$ALG" -out CA.key -outform DER $PROVIDER_FLAGS
openssl req -x509 -new -key CA.key -out CA.der -days "$DAYS" -subj "$CA_SUBJ" -nodes -outform DER $PROVIDER_FLAGS
openssl x509 -in CA.der -inform DER -out CA.pem -outform PEM

gen_cert() {
    local name=$1 subj=$2
    openssl genpkey -algorithm "$ALG" -out "$name.key" -outform DER $PROVIDER_FLAGS
    openssl req -new -key "$name.key" -out "$name.csr" -subj "$subj" -nodes -outform DER $PROVIDER_FLAGS
    openssl pkey -in "$name.key" -inform DER -out "$name.key.pem" -outform PEM
}

# Server + Client keys/CSRs
gen_cert server "$SRV_SUBJ"
gen_cert client "$CLI_SUBJ"

# Sign server + client
openssl x509 -req -in server.csr -CA CA.der -CAkey CA.key -CAcreateserial -out server.der -days "$DAYS" -outform DER
openssl x509 -req -in client.csr -CA CA.der -CAkey CA.key -CAcreateserial -out client.der -days "$DAYS" -outform DER

# Convert to PEM
openssl x509 -in server.der -inform DER -out server.pem -outform PEM
openssl x509 -in client.der -inform DER -out client.pem -outform PEM

# Verify
openssl verify -CAfile CA.pem server.pem
openssl verify -CAfile CA.pem client.pem

# Generate headers for client
xxd -i CA.der      > CA_der.h
xxd -i server.der  > server_der.h
xxd -i client.der  > client_der.h
xxd -i client.key  > client_key.h
