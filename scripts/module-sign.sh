#!/bin/sh

# https://docs.redhat.com/en/documentation/red_hat_enterprise_linux/8/html/managing_monitoring_and_updating_the_kernel/signing-a-kernel-and-modules-for-secure-boot_managing-monitoring-and-updating-the-kernel

YOUR_STUFF=$HOME/.module-sign-stuff
DBDIR=/etc/pki/pesign
ORG_NAME="your org name"
NICKNAME="your keys nickname"
FILENAME_NOEXT=sb_cert
CERT_PUBKEY=$FILENAME_NOEXT.cer
PKCS12_ENC_PRIVKEY=$FILENAME_NOEXT.p12
RAW_PRIVKEY=$FILENAME_NOEXT.priv
HASH_SIGN_ALGO=sha256
MODULE_TO_SIGN=src/kernel/sci-detect.ko

keygen() {
	echo " --- keygen --- "
	sudo efikeygen \
			--dbdir $DBDIR \
			--self-sign \
			--module \
			--common-name "CN=${ORG_NAME}" \
			--nickname "$NICKNAME"
	echo ""
}

extract_cert_pubkey() {
	echo " --- extract cert pubkey ---"
	sudo certutil \
			-d $DBDIR \
			-n "$NICKNAME" \
			-Lr \
			> $CERT_PUBKEY
	echo ""
}

mok_import_cert() {
	echo " --- mok import --- "
	sudo mokutil \
		--import $CERT_PUBKEY
	echo ""
}	

extract_enc_privkey() {
	echo " --- pkcs 12 extract privkey --- "
	sudo pk12util \
		-o $PKCS12_ENC_PRIVKEY \
		-n "$NICKNAME" \
		-d $DBDIR
	echo ""
}

decrypt_privkey() {
	echo " --- decrypt privkey --- "
	sudo openssl \
		pkcs12 \
		-in $PKCS12_ENC_PRIVKEY \
		-out $RAW_PRIVKEY \
		-nocerts \
		-nodes
	echo ""
}

sign_module() {
	echo " --- sign module --- "

	perms=400

	echo -e "    \t* IMPORTANT: chowning **TO YOU** ($USER:$USER) and chmodding ($perms) the following files: "
	echo -e "    \t\t:: $YOUR_STUFF/$RAW_PRIVKEY (UNENCRYPTED PRIVATE KEY)           "
	echo -e "    \t\t:: $YOUR_STUFF/$CERT_PUBKEY (X.509 cert carrying public key)     "

	sudo chown $USER:$USER $YOUR_STUFF/$RAW_PRIVKEY
	sudo chown $USER:$USER $YOUR_STUFF/$CERT_PUBKEY
	chmod $perms $YOUR_STUFF/$RAW_PRIVKEY
	chmod $perms $YOUR_STUFF/$CERT_PUBKEY

	/lib/modules/$(uname -r)/build/scripts/sign-file \
		$HASH_SIGN_ALGO \
		$YOUR_STUFF/$RAW_PRIVKEY \
		$YOUR_STUFF/$CERT_PUBKEY \
		$MODULE_TO_SIGN && echo "sign-file ok"
	echo ""
}

print_usage_and_exit() {
	echo "usage: $0 <what-to-do>"
	echo "<what-to-do> can be one of the following:"
	for wtd in $what_to_do; do
		if [[ $wtd != $default ]]; then
			echo -e "\t [*] $wtd"
		else
			echo -e "\t [D] $wtd (default if omitted)"
		fi
	done
	exit 1
}

what_to_do="keygen extract-cert mok-import extract-encrypted-privkey decrypt-privkey sign-module"
default="sign-module"

if [ $# -gt 1 ]; then
	print_usage_and_exit
fi

chosen=$default

if [ $# -eq 1 ]; then
	chosen=$1
fi

if [ $UID -eq 0 ]; then
	echo "don't run this as root"
	print_usage_and_exit
fi

if [ ! -d $YOUR_STUFF ]; then
	echo "creating $YOUR_STUFF..."
	mkdir -p $YOUR_STUFF
fi

pushd $YOUR_STUFF >>/dev/null

case $chosen in
	"keygen")
		keygen
		;&
	"extract-cert")
		extract_cert_pubkey
		;&
	"mok-import")
		mok_import_cert
		;&
	"extract-encrypted-privkey")
		extract_enc_privkey
		;&
	"decrypt-privkey")
		decrypt_privkey
		;&
	"sign-module")
		popd >>/dev/null
		sign_module
		;;
	*)
		popd >>/dev/null
		echo "invalid option: $chosen"
		print_usage_and_exit
		;;
esac
