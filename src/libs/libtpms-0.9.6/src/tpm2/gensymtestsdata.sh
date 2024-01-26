#!/bin/bash

function do_aes() {
    local data="$1"
    local osslflag="$2"

    for keysize in 128 192 256; do
        tmp=AES_KEY_${keysize}
        key=$(eval echo \$$tmp)
        for mode in ecb cbc cfb ofb ctr; do
            cipher="aes-${keysize}-${mode}"
            bs=$((128 / 8))
            iv=""
            ivparm=""
            case $mode in
            ecb)
                ;;
            ctr)
                v=255
                for ((c=0; c < bs; c++)); do
                   iv="$(printf "%02x" $v)${iv}"
                   v=$((v - 1))
                done
                ivparm="-iv ${iv}"
                ;;
            *)
                for ((c=0; c < bs; c++)); do
                   iv="${iv}$(printf "%02x" $c)"
                done
                ivparm="-iv ${iv}"
                ;;
            esac
            echo -n "$cipher: "
            openssl enc -e -K "${key}" ${ivparm} -${cipher} -in <(echo -en "$data") ${osslflag} | \
                od -t x1 -w128 -An | \
                sed -n 's/ \([a-f0-9]\{2\}\)/ 0x\1/pg'
        done
    done
}

function do_camellia() {
    local data="$1"
    local osslflag="$2"

    for keysize in 128 192 256; do
        tmp=CAMELLIA_KEY_${keysize}
        key=$(eval echo \$$tmp)
        for mode in ecb cbc cfb ofb ctr; do
            cipher="camellia-${keysize}-${mode}"
            bs=$((128 / 8))
            iv=""
            ivparm=""
            case $mode in
            ecb)
                ;;
            ctr)
                v=255
                for ((c=0; c < bs; c++)); do
                   iv="$(printf "%02x" $v)${iv}"
                   v=$((v - 1))
                done
                ivparm="-iv ${iv}"
                ;;
            *)
                for ((c=0; c < bs; c++)); do
                   iv="${iv}$(printf "%02x" $c)"
                done
                ivparm="-iv ${iv}"
                ;;
            esac
            echo -n "$cipher: "
            openssl enc -e -K "${key}" ${ivparm} -${cipher} -in <(echo -en "$data") ${osslflag} | \
                od -t x1 -w128 -An | \
                sed -n 's/ \([a-f0-9]\{2\}\)/ 0x\1/pg'
        done
    done
}

function do_tdes() {
    local data="$1"
    local osslflag="$2"

    for keysize in 128 192; do
        tmp=TDES_KEY_${keysize}
        key=$(eval echo \$$tmp)
        for mode in ecb cbc cfb ofb; do
            cipher="des-ede3-${mode}"
            iv=""
            ivparm=""
            bs=8
            case $mode in
            ecb)
                ;;
            *)
                for ((c=0; c < bs; c++)); do
                   iv="${iv}$(printf "%02x" $c)"
                done
                ivparm="-iv ${iv}"
                ;;
            esac
            echo -n "$cipher [${keysize}]: "
            case $mode in
            ecb|cbc)
                if [[ "${osslflag}" =~ "nopad" ]]; then
                    echo " Not supported without padding to blocksize"
                    continue
                fi
                ;;
            esac
            openssl enc -e -K "${key}" ${ivparm} -${cipher} -in <(echo -en "$data") ${osslflag} | \
                od -t x1 -w128 -An | \
                sed -n 's/ \([a-f0-9]\{2\}\)/ 0x\1/pg'
        done
    done
}

function do_sm4() {
    local data="$1"
    local osslflag="$2"

    for keysize in 128; do
        tmp=SM4_KEY_${keysize}
        key=$(eval echo \$$tmp)
        for mode in ecb cbc cfb ofb ctr; do
            cipher="sm4-${mode}"
            bs=$((128 / 8))
            iv=""
            ivparm=""
            case $mode in
            ecb)
                ;;
            ctr)
                v=255
                for ((c=0; c < bs; c++)); do
                   iv="$(printf "%02x" $v)${iv}"
                   v=$((v - 1))
                done
                ivparm="-iv ${iv}"
                ;;
            *)
                for ((c=0; c < bs; c++)); do
                   iv="${iv}$(printf "%02x" $c)"
                done
                ivparm="-iv ${iv}"
                ;;
            esac
            echo -n "$cipher: "
            openssl enc -e -K "${key}" ${ivparm} -${cipher} -in <(echo -en "$data") ${osslflag} | \
                od -t x1 -w128 -An | \
                sed -n 's/ \([a-f0-9]\{2\}\)/ 0x\1/pg'
        done
    done
}


AES_KEY_128='2b7e151628aed2a6abf7158809cf4f3c'
AES_KEY_192='8e73b0f7da0e6452c810f32b809079e562f8ead2522c6b7b'
AES_KEY_256='603deb1015ca71be2b73aef0857d77811f352c073b6108d72d9810a30914dff4'
AES_DATA_IN='\x6b\xc1\xbe\xe2\x2e\x40\x9f\x96\xe9\x3d\x7e\x11\x73\x93\x17\x2a\xae\x2d\x8a\x57\x1e\x03\xac\x9c\x9e\xb7\x6f\xac\x45\xaf\x8e\x51'

echo "----- AES -----"
do_aes "${AES_DATA_IN}" ""
echo "---------------"

# We need to extend the 128 bit key to be 192 bit key otherwise 3rd schedule is all zeroes
TDES_KEY_128=${AES_KEY_128}${AES_KEY_128:0:16}
TDES_KEY_192=${AES_KEY_192}
TDES_DATA_IN=${AES_DATA_IN}

echo "----- TDES -----"
do_tdes "${TDES_DATA_IN}" ""
echo "----------------"


echo "---- TDES (short input) -----"
do_tdes "\x31\x32\x33\x34\x35" "-nopad"
echo "----------------"

CAMELLIA_KEY_128=${AES_KEY_128}
CAMELLIA_KEY_192=${AES_KEY_192}
CAMELLIA_KEY_256=${AES_KEY_256}
CAMELLIA_DATA_IN=${AES_DATA_IN}

echo "----- CAMELLIA -----"
do_camellia "${CAMELLIA_DATA_IN}" ""
echo "--------------------"

if [ -n "$(openssl enc -ciphers | grep sm4)" ]; then
    SM4_KEY_128='0123456789abcdeffedcba9876543210'
    SM4_DATA_IN='\xaa\xaa\xaa\xaa\xbb\xbb\xbb\xbb\xcc\xcc\xcc\xcc\xdd\xdd\xdd\xdd\xee\xee\xee\xee\xff\xff\xff\xff\xaa\xaa\xaa\xaa\xbb\xbb\xbb\xbb'

    echo "-------- SM4 -------"
    do_sm4 "${SM4_DATA_IN}" ""
    echo "--------------------"
fi
