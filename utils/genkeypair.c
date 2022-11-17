
#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>

#include "RSA/source/global.h"
#include "RSA/source/rsaref.h"

#define KEYSIZE	512

static void WritePubKey(R_RSA_PUBLIC_KEY *key);
static void WritePrivKey(R_RSA_PRIVATE_KEY *key);

main(int argc, char *argv[])
{
	struct timeval now;
	unsigned int bytesleft; unsigned char randbyte;
	R_RANDOM_STRUCT weewee;
	R_RSA_PROTO_KEY protoKey;
	R_RSA_PUBLIC_KEY public_key;
	R_RSA_PRIVATE_KEY private_key;

	/* Initialize silly random struct */
	R_RandomInit(&weewee);
	gettimeofday(&now, NULL);
	srand(now.tv_usec);
	for ( R_GetRandomBytesNeeded(&bytesleft, &weewee);
						bytesleft > 0; --bytesleft ) {
		randbyte = (rand()%256);
		R_RandomUpdate(&weewee, &randbyte, 1);
	}
  
	protoKey.bits = KEYSIZE;
	protoKey.useFermat4 = 1;
  
	printf("This may take some time...\n");
	if (R_GeneratePEMKeys(&public_key, &private_key, &protoKey, &weewee)) {
		fprintf(stderr, "Error generating keys!\n");
		exit(1);
	}
	WritePubKey(&public_key);
	WritePrivKey(&private_key);
	printf("Done!\n");
}

static void WriteBigInt(FILE *output, char *label, char *bigint, int size)
{
	fprintf(output, "\t/* %s */ { ", label);
	while ( size-- > 0 )
		fprintf(output, "0x%.2X, ", (*(bigint++)&0xFF));
	fprintf(output, "},\n");
}

static void WritePubKey(R_RSA_PUBLIC_KEY *key)
{
	FILE *output;

	if ( (output=fopen("public_key.h", "w")) == NULL ) {
		perror("Can't open public_key.h for writing");
		return;
	}
	fprintf(output, "/* RSA Public Key */\n\n");
	fprintf(output, "static R_RSA_PUBLIC_KEY public_key = {\n");
	fprintf(output, "\t/* bits */ %d,\n", key->bits);
	WriteBigInt(output, "modulus", key->modulus, sizeof(key->modulus));
	WriteBigInt(output, "exponent", key->exponent, sizeof(key->exponent));
	fprintf(output, "};\n\n");
	fclose(output);
}
static void WritePrivKey(R_RSA_PRIVATE_KEY *key)
{
	FILE *output;

	if ( (output=fopen("private_key.h", "w")) == NULL ) {
		perror("Can't open private_key.h for writing");
		return;
	}
	fprintf(output, "/* RSA Private Key */\n\n");
	fprintf(output, "static R_RSA_PRIVATE_KEY private_key = {\n");
	fprintf(output, "\t/* bits */ %d,\n", key->bits);
	WriteBigInt(output, "modulus", key->modulus, sizeof(key->modulus));
	WriteBigInt(output, "public exponent", key->publicExponent,
					sizeof(key->publicExponent));
	WriteBigInt(output, "exponent", key->exponent, sizeof(key->exponent));
	fprintf(output, "{\n");
	WriteBigInt(output, "prime 1", key->prime[0], sizeof(key->prime[0]));
	WriteBigInt(output, "prime 2", key->prime[1], sizeof(key->prime[1]));
	fprintf(output, "},\n{\n");
	WriteBigInt(output, "prime exponent 1", key->primeExponent[0],
					sizeof(key->primeExponent[0]));
	WriteBigInt(output, "prime exponent 2", key->primeExponent[1],
					sizeof(key->primeExponent[1]));
	fprintf(output, "},\n");
	WriteBigInt(output, "coefficient", key->coefficient,
					sizeof(key->coefficient));
	fprintf(output, "};\n\n");
	fclose(output);
}
