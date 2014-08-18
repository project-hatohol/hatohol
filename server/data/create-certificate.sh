#!/bin/sh

set -u

run()
{
    "$@"
    if test $? -ne 0; then
	echo "Failed $@"
	exit 1
    fi
}

base_dir="."
ca_dir="${base_dir}/CA"
config_file="${base_dir}/openssl.cnf"

create_ca()
{
    cat > "${config_file}" <<EOF
[ ca ]
default_ca = local_ca

[ local_ca ]
dir			= ${ca_dir}
certs			= \$dir/certs
crl_dir         	= \$dir/crl
database	        = \$dir/index.txt 
new_certs_dir		= \$dir/newcerts
certificate		= \$dir/cacert.pem
serial			= \$dir/serial
crlnumber		= \$dir/crlnumber
crl			= \$dir/crl.pem
private_key		= \$dir/private/cakey.pem
RANDFILE		= \$dir/private/.rand

x509_extensions		= certificate_extensions

default_days		= 3650
default_crl_days	= 30
default_md		= default

policy			= policy_anything

[ certificate_extensions ]
basicConstraints	= CA:false

[ req ]
default_bits		= 2048
distinguished_name	= req_distinguished_name
x509_extensions		= req_extensions

string_mask		= utf8only

[ req_distinguished_name ]
commonName		= hostname

[ req_extensions ]
subjectKeyIdentifier	= hash
authorityKeyIdentifier	= keyid:always,issuer
basicConstraints	= CA:true
keyUsage		= cRLSign, keyCertSign

[ client_ca_extensions ]
basicConstraints	= CA:false
keyUsage		= digitalSignature
extendedKeyUsage	= 1.3.6.1.5.5.7.3.2

[ server_ca_extensions ]
basicConstraints	= CA:false
keyUsage		= keyEncipherment
extendedKeyUsage	= 1.3.6.1.5.5.7.3.1

[ policy_anything ]
countryName		= optional
stateOrProvinceName	= optional
localityName		= optional
organizationName	= optional
organizationalUnitName	= optional
commonName		= supplied
emailAddress		= optional
EOF

    run mkdir -p \
        "${ca_dir}/certs" \
        "${ca_dir}/newcerts" \
        "${ca_dir}/private"
    run chmod 700 "${ca_dir}/private"
    run echo 01 > "${ca_dir}/serial"
    run touch "${ca_dir}/index.txt"

    run openssl req \
        -x509 \
        -config "${config_file}" \
        -newkey rsa:2048 \
        -keyout "${ca_dir}/private/cakey.pem" \
        -out "${ca_dir}/cacert.pem" \
        -outform PEM \
        -subj /CN=LocalCA/ \
        -nodes
    run openssl x509 \
        -in "${ca_dir}/cacert.pem" \
        -out "${ca_dir}/cacert.cer" \
        -outform DER
}

create_server_certificate()
{
    run mkdir -p server

    run openssl genrsa -out server/key.pem 2048
    run chmod 600 server/key.pem
    run openssl req \
        -new \
        -key server/key.pem \
        -out server/req.pem \
        -outform PEM \
        -subj /CN=$(hostname)/O=server/ \
        -nodes

    run openssl ca \
        -config "${config_file}" \
        -in server/req.pem \
        -out server/cert.pem \
        -notext \
        -batch -extensions server_ca_extensions
}

create_client_certificate()
{
    run mkdir -p client

    run openssl genrsa -out client/key.pem 2048
    run chmod 600 client/key.pem
    run openssl req \
        -new \
        -key client/key.pem \
        -out client/req.pem \
        -outform PEM \
        -subj /CN=$(hostname)/O=client/ \
        -nodes

    run openssl ca \
        -config "${config_file}" \
        -in client/req.pem \
        -out client/cert.pem \
        -notext \
        -batch \
        -extensions \
        client_ca_extensions
}

create_ca
create_server_certificate
create_client_certificate
