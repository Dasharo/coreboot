/* SPDX-License-Identifier: GPL-2.0-only */

#include <getopt.h>
#include <stdbool.h>
#include <stdio.h>
#include <sys/stat.h>

#ifndef WOLFSSL_USER_SETTINGS
	#include <wolfssl/options.h>
#endif

#include <wolfssl/ssl.h>
#include <wolfssl/wolfcrypt/rsa.h>
#include <wolfssl/wolfcrypt/signature.h>
#include <wolfssl/wolfcrypt/hash.h>
#include <wolfssl/wolfcrypt/error-crypt.h>

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))
#endif

#define WARN(...) fprintf(stderr, "WARNING: " __VA_ARGS__)
#define ERROR(...) fprintf(stderr, "ERROR: " __VA_ARGS__)

#define RSA_KEY_SIZE			2048
#define RSA_KEY_EXPONENT		0x10001
#define DER_FILE_BUFFER			2048 /* max DER size */

#define MANIFEST_SIZE			1024

#define SB_MANIFEST_MAGIC		0x4d425624	/* $VBM in LE */
#define SB_MANIFEST_VERSION		0x1
#define SB_MANIFEST_SIZE		MANIFEST_SIZE

#define KEY_MANIFEST_MAGIC		0x4d4b5324	/* $SKM in LE */
#define KEY_MANIFEST_VERSION		0x1
#define KEY_MANIFEST_PAD_SIZE		(4096 - MANIFEST_SIZE)
#define KEY_MANIFEST_RESERVED_SIZE	456
#define KEY_MANIFEST_SIZE		(MANIFEST_SIZE + KEY_MANIFEST_PAD_SIZE)

#define IBB_SIZE			(0x20000 - SB_MANIFEST_SIZE)

static const char tool_version[] = "0.1";

struct manifest_header {
	uint32_t magic;
	uint32_t version;
	uint32_t size;
	uint32_t svn;
} __attribute__((packed));

struct key_signature {
	uint8_t modulus[256];
	uint32_t exponent;
	uint8_t signature[256];
} __attribute__((packed));

struct manifest {
	struct manifest_header header;
	uint32_t km_id;				/* Only for Key Manifest */
	uint8_t hash[WC_SHA256_DIGEST_SIZE];	/* IBB hash of SBM, Key hash for KM */
	union {
		struct {
			uint32_t reserved1;
			uint32_t reserved2[8];
			uint8_t oem_data[400];
			uint8_t reserved3[20];
		} sb;
		uint8_t reserved[KEY_MANIFEST_RESERVED_SIZE];
	};
	struct key_signature key_sig;
} __attribute__((packed));

_Static_assert(sizeof(struct manifest) == MANIFEST_SIZE, "Wrong size of Manifest structure");

static struct params {
	const char *output;
	const char *file_name;
	const char *oem_data_file;
	const char *sbm_key_file;
	const char *km_key_file;
	bool override_km_id;
	uint32_t km_id;
	bool override_svn;
	uint32_t svn;
} params;

static off_t get_file_size(FILE *f)
{
	off_t fsize;
	fseek(f, 0, SEEK_END);
	fsize = ftell(f);
	fseek(f, 0, SEEK_SET);
	return fsize;
}

static bool mem_is_zero(const byte *memory, word32 size)
{
	for (word32 i = 0; i < size; i++) {
		if (memory[i] == 0)
			continue;
		else
			return false;
	}

	return true;
}

static void hexdump(const void *buffer, word32 len, byte cols, word32 indent)
{
	word32 i, j;

	if (len > 0) {
		for (j = 0; j < indent; j++)
			printf("\t");
	}

	for (i = 0; i < len + ((len % cols) ? (cols - len % cols) : 0); i++)
	{
		/* print hex data */
		if (i < len) {
			printf("%02X ", ((byte*)buffer)[i] & 0xFF);
		}

		if (i % cols == (word32)(cols - 1)) {
			printf("\n");
			if (i != (len - 1)) {
				for (j = 0; j < indent; j++)
					printf("\t");
			}
		}
	}
}

static void print_sha256(const byte *buffer)
{
	for (word32 i = 0; i < WC_SHA256_DIGEST_SIZE; i++)
		printf("%02X", buffer[i]);

	printf("\n");
}

static void print_magic(uint32_t magic)
{
	printf("%c%c%c%c\n",
		(unsigned char)(magic & 0xff),
		(unsigned char)((magic >> 8) & 0xff),
		(unsigned char)((magic >> 16) & 0xff),
		(unsigned char)((magic >> 24) & 0xff));
}

static int swap_bytes(byte *buffer, word32 len)
{
	byte *tmp;

	if (len <= 0 || buffer == NULL)
		return EXIT_FAILURE;

	tmp = malloc(len);

	if (!tmp)
		return EXIT_FAILURE;

	memcpy(tmp, buffer, len);

	for (word32 i = 0; i < len; i++) {
		buffer[i] = tmp[len - 1 - i];
	}

	free(tmp);

	return 0;
}

static int rsa_load_key_file(const char *keyFile, RsaKey *rsaKey)
{
	int ret = EXIT_FAILURE;
	FILE *file;
	byte *buffer = NULL;
	word32 bufSize = DER_FILE_BUFFER;
	byte *derBuffer = NULL;
	word32 derBufSize = DER_FILE_BUFFER;
	word32 bytes = 0;
	word32 idx = 0;

	file = fopen(keyFile, "rb");
	if (file) {
		buffer = malloc(bufSize);
		if (buffer) {
			bytes = fread(buffer, 1, bufSize, file);
		}
		fclose(file);
	}

	derBuffer = malloc(derBufSize);

	if (buffer != NULL && bytes > 0 && derBuffer != NULL) {
		ret = wc_KeyPemToDer(buffer, bytes, derBuffer, derBufSize, NULL);

		if (ret < 0) {
			free(derBuffer);
		} else {
			free(buffer);
			buffer = derBuffer;
			bytes = (word32)ret;
		}

		ret = wc_RsaPrivateKeyDecode(buffer, &idx, rsaKey, (word32)bytes);
	}

	if (buffer) {
		free(buffer);
	}

	return ret;
}

static int calculate_key_hash(RsaKey *rsaKey, byte *key_hash)
{
	int ret;
	wc_Sha256 sha256;
	uint8_t modulus[256];
	uint32_t exponent = 0;
	word32 e_size = sizeof(exponent);
	word32 n_size = sizeof(modulus);

	ret = wc_RsaFlattenPublicKey(rsaKey, (byte *)&exponent, &e_size,
					      (byte *)&modulus, &n_size);
	if (ret != 0) {
		ERROR("Failed to extract Secure Boot Manifest Key exponent and modulus!\n");
		return ret;
	}

	if (swap_bytes(modulus, n_size)) {
		ERROR("Failed to change RSA key modulus endianess!\n");
		return EXIT_FAILURE;
	}

	ret = wc_InitSha256(&sha256);
	if (ret != 0) {
		ERROR("Failed to initialize SHA256 context\n");
		return ret;
	}

	ret = wc_Sha256Update(&sha256, modulus, n_size);
	if (ret != 0) {
		ERROR("Failed to calculate SHA256\n");
		return ret;
	}

	/* e_size returned is 3, but we have to take whole uint32_t */
	ret = wc_Sha256Update(&sha256, (byte *)&exponent, sizeof(exponent));
	if (ret != 0) {
		ERROR("Failed to calculate SHA256\n");
		return ret;
	}

	ret = wc_Sha256Final(&sha256, key_hash);
	if (ret != 0) {
		ERROR("Failed to generate SHA256 hash\n");
		return ret;
	}

	if (swap_bytes(key_hash, WC_SHA256_DIGEST_SIZE)) {
		ERROR("Failed to change SHA256 hash endianess!\n");
		return EXIT_FAILURE;
	}

	return ret;
}

static int calculate_ibb_hash(const byte *buffer, byte *ibb_hash)
{
	int ret;
	wc_Sha256 sha256;

	ret = wc_InitSha256(&sha256);
	if (ret != 0) {
		ERROR("Failed to initialize SHA256 context\n");
		return ret;
	}

	ret = wc_Sha256Update(&sha256, buffer, IBB_SIZE);
	if (ret != 0) {
		ERROR("Failed to calculate SHA256\n");
		return ret;
	}

	ret = wc_Sha256Final(&sha256, ibb_hash);
	if (ret != 0) {
		ERROR("Failed to generate SHA256 hash\n");
		return ret;
	}

	if (swap_bytes(ibb_hash, WC_SHA256_DIGEST_SIZE)) {
		ERROR("Failed to change SHA256 hash endianess!\n");
		return EXIT_FAILURE;
	}

	return ret;
}

static int create_sb_manifest(struct manifest *man, byte* romBuf, word32 romLen, byte* oemData,
			      word32 oemDataLen, word32 manifestSVN)
{
	int ret;
	byte *sha256_buffer;
	size_t file_size = IBB_SIZE + SB_MANIFEST_SIZE;

	if (romLen < file_size) {
		ERROR("ROM file too small\n");
		return EXIT_FAILURE;
	}

	if (wc_HashGetDigestSize(WC_HASH_TYPE_SHA256) != 32) {
		ERROR("Hash type SHA256 not supported!\n");
		return EXIT_FAILURE;
	}

	memset((byte *)man, 0, MANIFEST_SIZE);

	man->header.magic = SB_MANIFEST_MAGIC;
	man->header.version = SB_MANIFEST_VERSION;
	man->header.svn = manifestSVN;
	man->header.size = MANIFEST_SIZE;

	if (oemData != NULL && oemDataLen > 0) {
		if (oemDataLen <= sizeof(man->sb.oem_data))
			memcpy((byte *)man->sb.oem_data, oemData, oemDataLen);
		else
			WARN("OEM Data file length too big, ignoring it.\n");
	}

	ret = calculate_ibb_hash(&romBuf[romLen - IBB_SIZE], man->hash);
	if (ret != 0) {
		ERROR("Failed to calcualte IBB hash!\n");
		return ret;
	}

	sha256_buffer = man->hash;
	printf("SHA256 of the IBB (Little Endian):\n");
	print_sha256(sha256_buffer);
	printf("\n");

	return ret;
}

static int create_key_manifest(struct manifest *man, word32 romLen, word32 manifestSVN)
{
	int ret;
	RsaKey sbmKey;
	size_t file_size = IBB_SIZE + SB_MANIFEST_SIZE + KEY_MANIFEST_SIZE;

	if (romLen < file_size) {
		ERROR("ROM file too small\n");
		return EXIT_FAILURE;
	}

	if (wc_HashGetDigestSize(WC_HASH_TYPE_SHA256) != 32) {
		ERROR("Hash type SHA256 not supported!\n");
		return EXIT_FAILURE;
	}

	memset((byte *)man, 0, MANIFEST_SIZE);

	man->header.magic = KEY_MANIFEST_MAGIC;
	man->header.version = KEY_MANIFEST_VERSION;
	man->header.svn = manifestSVN;
	man->header.size = MANIFEST_SIZE;

	if (params.override_km_id)
		man->km_id = params.km_id;
	else
		man->km_id = 1;

	ret = wc_InitRsaKey(&sbmKey, NULL);
	if (ret != 0) {
		ERROR("Init RSA key failed! %d\n", ret);
		return ret;
	}

	ret = rsa_load_key_file(params.sbm_key_file, &sbmKey);
	if (ret != 0) {
		ERROR("Failed to load Secure Boot Manifest Key!\n");
		return ret;
	}

	ret = calculate_key_hash(&sbmKey, man->hash);
	if (ret != 0) {
		ERROR("Failed to calcualte Secure Boot key hash!\n");
		wc_FreeRsaKey(&sbmKey);
		return ret;
	}

	printf("SHA256 of the Secure Boot Manifest Key (Little Endian):\n");
	print_sha256(man->hash);
	printf("\n");

	wc_FreeRsaKey(&sbmKey);
	return ret;
}

static int rsa_sign_manifest(struct manifest *man, const char *key_path)
{
	int ret;
	RsaKey rsaKey;
	WC_RNG rng;
	byte *sigBuf = NULL;
	word32 sigLen;
	word32 e_size, n_size;
	struct key_signature *key_struct = &man->key_sig;

	ret = wc_InitRng(&rng);
	if (ret != 0) {
		ERROR("Init RNG failed! %d\n", ret);
		goto exit;
	}

	ret = wc_InitRsaKey(&rsaKey, NULL);
	if (ret != 0) {
		ERROR("Init RSA key failed! %d\n", ret);
		goto exit;
	}

	ret = rsa_load_key_file(key_path, &rsaKey);
	if (ret != 0) {
		ERROR("Failed to load RSA key %s! (%d)\n", key_path, ret);
		goto exit;
	}

	e_size = sizeof(key_struct->exponent);
	n_size = sizeof(key_struct->modulus);

	ret = wc_RsaFlattenPublicKey(&rsaKey, (byte *)&key_struct->exponent, &e_size,
					      (byte *)key_struct->modulus, &n_size);

	if (ret != 0) {
		ERROR("Failed to write key modulus and exponent! %d\n", ret);
		goto exit;
	}

	/* Change endianess of the modulus to LE */
	if (swap_bytes((byte *)key_struct->modulus, n_size)) {
		ERROR("Failed to change RSA Key modulus endianess!\n");
		goto exit;
	}

	if (n_size * 8 != RSA_KEY_SIZE) {
		ERROR("Incorrect RSA key size! %d != 2048\n", n_size * 8);
		ret = EXIT_FAILURE;
		goto exit;
	}

	if (key_struct->exponent != RSA_KEY_EXPONENT) {
		ERROR("Incorrect RSA key size! 0x%x != 0x10001\n", key_struct->exponent);
		ret = EXIT_FAILURE;
		goto exit;
	}

	/* Verify hash type is supported */
	if (wc_HashGetDigestSize(WC_HASH_TYPE_SHA256) != 32) {
		ERROR("Hash type SHA256 not supported!\n");
		ret = EXIT_FAILURE;
		goto exit;
	}

	sigLen = wc_SignatureGetSize(WC_SIGNATURE_TYPE_RSA_W_ENC, &rsaKey, sizeof(rsaKey));
	if (sigLen != sizeof(key_struct->signature)) {
		ERROR("RSA Signature size check fail! %d\n", sigLen);
		ret = EXIT_FAILURE;
		goto exit;
	}

	sigBuf = malloc(sigLen);
	if (!sigBuf) {
		ERROR("RSA Signature malloc failed!\n");
		ret = EXIT_FAILURE;
		goto exit;
	}

	ret = wc_SignatureGenerate(WC_HASH_TYPE_SHA256, WC_SIGNATURE_TYPE_RSA_W_ENC,
				   (byte *)man,
				   MANIFEST_SIZE - sizeof(struct key_signature),
				   sigBuf, &sigLen,
				   &rsaKey, sizeof(rsaKey),
				   &rng);
	if (ret < 0) {
		ERROR("RSA signature generation failed (%d)!\n", ret);
		goto exit;
	}

	ret = wc_SignatureVerify(WC_HASH_TYPE_SHA256, WC_SIGNATURE_TYPE_RSA_W_ENC,
				 (byte *)man,
				 MANIFEST_SIZE - sizeof(struct key_signature),
				 sigBuf, sigLen,
				 &rsaKey, sizeof(rsaKey));
	if (ret < 0) {
		ERROR("RSA Signature Verification failed: (%d)\n", ret);
		goto exit;
	}

	if (swap_bytes(sigBuf, sigLen)) {
		ERROR("Failed to change RSA Signature endianess!\n");
		goto exit;
	}

	memcpy(key_struct->signature, sigBuf, sigLen);
exit:
	if (sigBuf) {
		free(sigBuf);
	}
	wc_FreeRsaKey(&rsaKey);
	wc_FreeRng(&rng);

	return ret;
}

static int save_buffer_to_file(const char *filename, byte *buffer, word32 bufLen)
{
	int ret = 0;
	FILE* file = NULL;

	file = fopen(filename, "wb");
	if (file == NULL) {
		ERROR("Failed to create file %s!\n", filename);
		ret = EXIT_FAILURE;
		goto exit;
	}

	if (fwrite(buffer, 1, bufLen, file) != bufLen) {
		ERROR("Error writing %s file! %d", filename, ret);
		ret = EXIT_FAILURE;
		goto exit;
	}

exit:
	if (file) {
		fclose(file);
	}

	return ret;
}

static int load_file_to_buffer(const char *filename, byte **fileBuf, word32 *fileLen)
{
	int ret = 0;
	FILE *file = NULL;

	file = fopen(filename, "rb");
	if (file == NULL) {
		ERROR("File %s does not exist!\n", filename);
		ret = EXIT_FAILURE;
		goto exit;
	}

	*fileLen = get_file_size(file);
	/*
	 * To avoid undefined behavior when reading from potentially unaligned
	 * pointer, check that the file size is divisible by 4 bytes. Both manifests
	 * have such sizes (see SB_MANIFEST_SIZE and KEY_MANIFEST_SIZE), so this
	 * wouldn't fail for valid manifests. In case of coreboot.rom, the offsets are
	 * calculated relatively to the end of file, so the result may be misaligned if
	 * file size is misaligned. Instead of repeating checks for proper alignment in
	 * cmd_verify() and cmd_print(), do it here.
	 */
	if (*fileLen % sizeof(uint32_t)) {
		ERROR("File size isn't a multiple of 4 bytes!\n");
		ret = EXIT_FAILURE;
		goto exit;
	}

	*fileBuf = malloc(*fileLen);
	if (!*fileBuf) {
		ERROR("File buffer malloc failed!\n");
		ret = EXIT_FAILURE;
		goto exit;
	}

	if (fread(*fileBuf, 1, *fileLen, file) != *fileLen) {
		ERROR("Error reading file! %d", ret);
		ret = EXIT_FAILURE;
		goto exit;
	}

exit:
	if (file) {
		fclose(file);
	}

	return ret;
}

static int inject_manifest_to_binary(const char *filename, byte *fileBuf, word32 fileLen,
				     struct manifest *man)
{
	word32 file_size, offset;

	file_size = IBB_SIZE + SB_MANIFEST_SIZE;
	offset = fileLen - IBB_SIZE - SB_MANIFEST_SIZE;

	if (man->header.magic == KEY_MANIFEST_MAGIC) {
		file_size += KEY_MANIFEST_SIZE;
		offset -= KEY_MANIFEST_SIZE;
		/* Key Manifest is 4K big with 3K padded with zeros after the manifest */
		memset(&fileBuf[offset], 0, KEY_MANIFEST_SIZE);
	} else if (man->header.magic != SB_MANIFEST_MAGIC) {
		ERROR("Invalid manifest type\n");
		return EXIT_FAILURE;
	}

	if (fileLen < file_size) {
		ERROR("ROM file too small to fit IBB and manifest(s)\n");
		return EXIT_FAILURE;
	}

	memcpy(&fileBuf[offset], (byte *)man, MANIFEST_SIZE);

	return save_buffer_to_file(filename, fileBuf, fileLen);
}

static int cmd_generate_sbm(void)
{
	int ret = 0;
	word32 fileLen;
	byte *fileBuf = NULL;
	word32 oemDataFileLen = 0;
	byte *oemDataFileBuf = NULL;
	word32 manifest_svn = params.override_svn ? params.svn : 2;
	struct manifest man;

	if (params.oem_data_file) {
		ret = load_file_to_buffer(params.oem_data_file, &oemDataFileBuf, &oemDataFileLen);
		if (ret < 0) {
			ERROR("Failed to load OEM Data file\n");
			goto exit;
		}
	}

	if (params.file_name) {
		ret = load_file_to_buffer(params.file_name, &fileBuf, &fileLen);
		if (ret < 0) {
			goto exit;
		}
	} else {
		ERROR("coreboot image file not given\n");
		return -1;
	}

	if (!params.sbm_key_file) {
		ERROR("Secure Boot Manifest key file not given\n");
		return -1;
	}

	if (manifest_svn > 63) {
		ERROR("Secure Boot Manifest SVN %d invalid, allowed range: 0-63\n", manifest_svn);
		ret = EXIT_FAILURE;
		goto exit;
	}

	ret = create_sb_manifest(&man, fileBuf, fileLen, oemDataFileBuf, oemDataFileLen, manifest_svn);
	if (ret < 0) {
		goto exit;
	}

	ret = rsa_sign_manifest(&man, params.sbm_key_file);
	if (ret < 0) {
		goto exit;
	}

	ret = inject_manifest_to_binary(params.file_name, fileBuf, fileLen, &man);
	if (ret < 0) {
		goto exit;
	}

	printf("Secure Boot Manifest generated successfully\n");

	if (params.output) {
		printf("Saving Secure Boot Manifest as %s\n", params.output);
		ret = save_buffer_to_file(params.output, (byte *)&man, MANIFEST_SIZE);
	} else {
		printf("Not saving Secure Boot Manifest as a file, not requested to\n");
	}

exit:
	if (fileBuf) {
		free(fileBuf);
	}

	if (oemDataFileBuf) {
		free(oemDataFileBuf);
	}

	return ret;
}

static int cmd_generate_km(void)
{
	int ret = 0;
	word32 fileLen;
	byte *fileBuf = NULL;
	word32 manifest_svn = params.override_svn ? params.svn : 2;
	struct manifest man;

	if (params.file_name) {
		ret = load_file_to_buffer(params.file_name, &fileBuf, &fileLen);
		if (ret < 0) {
			goto exit;
		}
	} else {
		ERROR("coreboot image file not given\n");
		return EXIT_FAILURE;
	}

	if (!params.km_key_file) {
		ERROR("Key Manifest key file not given\n");
		ret = EXIT_FAILURE;
		goto exit;
	}

	if (!params.sbm_key_file) {
		ERROR("Secure Boot Manifest key file not given\n");
		ret = EXIT_FAILURE;
		goto exit;
	}

	if (manifest_svn > 15 || manifest_svn == 0) {
		ERROR("Key Manifest SVN %d invalid, allowed range: 1-15\n", manifest_svn);
		ret = EXIT_FAILURE;
		goto exit;
	}

	if (params.override_km_id) {
		if (params.km_id == 0 || params.km_id > 15) {
			ERROR("Key Manifest ID %d invalid, allowed range: 1-15\n",
				params.km_id);
			ret = EXIT_FAILURE;
			goto exit;
		}
	}

	ret = create_key_manifest(&man, fileLen, manifest_svn);
	if (ret < 0) {
		goto exit;
	}

	ret = rsa_sign_manifest(&man, params.km_key_file);
	if (ret < 0) {
		goto exit;
	}

	ret = inject_manifest_to_binary(params.file_name, fileBuf, fileLen, &man);
	if (ret < 0) {
		goto exit;
	}

	printf("Key Manifest generated successfully\n");

	if (params.output) {
		printf("Saving Key Manifest as %s\n", params.output);
		ret = save_buffer_to_file(params.output, (byte *)&man, MANIFEST_SIZE);
	} else {
		printf("Not saving Key Manifest as a file, not requested to\n");
	}

exit:
	/* Free */
	if (fileBuf) {
		free(fileBuf);
	}

	return ret;
}

static int print_key_hash_from_manifest(const struct manifest *man)
{
	int ret;
	wc_Sha256 sha256;
	const struct key_signature *key_struct;
	uint8_t key_hash[32];

	key_struct = &man->key_sig;

	ret = wc_InitSha256(&sha256);
	if (ret != 0) {
		ERROR("Failed to initialize SHA256 context\n");
		return ret;
	}

	ret = wc_Sha256Update(&sha256, key_struct->modulus, sizeof(key_struct->modulus));
	if (ret != 0) {
		ERROR("Failed to calculate SHA256\n");
		return ret;
	}

	ret = wc_Sha256Update(&sha256, (byte *)&key_struct->exponent,
			      sizeof(key_struct->exponent));
	if (ret != 0) {
		ERROR("Failed to calculate SHA256\n");
		return ret;
	}

	ret = wc_Sha256Final(&sha256, key_hash);
	if (ret != 0) {
		ERROR("Failed to generate SHA256 hash\n");
		return ret;
	}

	if (swap_bytes(key_hash, WC_SHA256_DIGEST_SIZE)) {
		ERROR("Failed to change SHA256 hash endianess!\n");
		return EXIT_FAILURE;
	}

	print_sha256(key_hash);

	return ret;
}

static void print_manifest_header(const struct manifest_header *mh)
{
	printf("\tMagic: ");
	print_magic(mh->magic);
	printf("\tVersion: %d\n", mh->version);
	printf("\tSize: %d\n", mh->size);
	printf("\tSvn: %d\n", mh->svn);
}


static void print_key_signature(const struct key_signature *ks)
{
	printf("\tKey modulus:\n");
	hexdump(ks->modulus, sizeof(ks->modulus), 16, 2);
	printf("\tKey exponent: 0x%x\n", ks->exponent);
	printf("\tSignature:\n");
	hexdump(ks->signature, sizeof(ks->signature), 16, 2);
}

static void print_key_manifest(const struct manifest *km)
{
	printf("Key Manifest:\n");
	print_manifest_header(&km->header);
	printf("\tKey Manifest ID: 0x%02x\n", km->km_id);
	printf("\tSB Key hash: ");
	print_sha256(km->hash);
	print_key_signature(&km->key_sig);
	printf("\n");
	printf("\tFYI: Secure Boot Key Manifest Key SHA256 hash:\n\n\t");
	print_key_hash_from_manifest(km);
	printf("\n\n");
}

static void print_sb_manifest(const struct manifest *sbm)
{
	printf("Secure Boot Manifest:\n");
	print_manifest_header(&sbm->header);
	printf("\tIBB hash: ");
	print_sha256(sbm->hash);
	printf("\tOEM Data:\n");
	hexdump(sbm->sb.oem_data, sizeof(sbm->sb.oem_data), 16, 2);
	print_key_signature(&sbm->key_sig);
	printf("\n");
	printf("\tFYI: Secure Boot Manifest Key SHA256 hash:\n\n\t");
	print_key_hash_from_manifest(sbm);
	printf("\n\n");
}

static int cmd_print(void)
{
	int ret = 0;
	word32 fileLen;
	byte *fileBuf = NULL;
	bool sbm_found = false;
	bool km_found = false;
	word32 offset;

	if (params.file_name) {
		ret = load_file_to_buffer(params.file_name, &fileBuf, &fileLen);
		if (ret < 0) {
			goto exit;
		}
	} else {
		ERROR("Image file not given\n");
		return EXIT_FAILURE;
	}

	/* Maybe it is SB manifest or KM manifest file (with IBB or not) */
	if (fileLen == MANIFEST_SIZE || fileLen == (IBB_SIZE + SB_MANIFEST_SIZE)) {
		if (*(uint32_t *)fileBuf == SB_MANIFEST_MAGIC) {
			printf("Input file seems to be a Secure Boot Manifest\n\n");
			print_sb_manifest((struct manifest *)fileBuf);
		} else if (*(uint32_t *)fileBuf == KEY_MANIFEST_MAGIC) {
			printf("Input file seems to be a Key Manifest\n\n");
			print_key_manifest((struct manifest *)fileBuf);
		} else {
			ERROR("Input file invalid size\n");
			ret = EXIT_FAILURE;
		}
	} else if (fileLen == KEY_MANIFEST_SIZE ||
		   fileLen == (IBB_SIZE + SB_MANIFEST_SIZE + KEY_MANIFEST_SIZE)) {
		if (*(uint32_t *)fileBuf == KEY_MANIFEST_MAGIC) {
			printf("Input file seems to be a Key Manifest\n\n");
			print_key_manifest((struct manifest *)fileBuf);
			if (fileLen > KEY_MANIFEST_SIZE &&
			    *(uint32_t *)&fileBuf[KEY_MANIFEST_SIZE] == SB_MANIFEST_MAGIC) {
				print_sb_manifest((struct manifest *)
						  &fileBuf[KEY_MANIFEST_SIZE]);
			}
		} else {
			ERROR("Input file invalid size\n");
			ret = EXIT_FAILURE;
		}
	} else if (fileLen > (IBB_SIZE + SB_MANIFEST_SIZE + KEY_MANIFEST_SIZE)) {
		/* Treat it as a full binary */
		offset = fileLen - IBB_SIZE - SB_MANIFEST_SIZE - KEY_MANIFEST_SIZE;

		if (*(uint32_t *)&fileBuf[offset] == KEY_MANIFEST_MAGIC) {
			print_key_manifest((struct manifest *)&fileBuf[offset]);
			km_found = true;
		}

		offset = fileLen - IBB_SIZE - SB_MANIFEST_SIZE;

		if (*(uint32_t *)&fileBuf[offset] == SB_MANIFEST_MAGIC) {
			print_sb_manifest((struct manifest *)&fileBuf[offset]);
			sbm_found = true;
		}

		if (!sbm_found && !km_found) {
			ERROR("Input file invalid, manifests not found\n");
			ret = EXIT_FAILURE;
		}
	} else {
		ERROR("Input file invalid size\n");
		ret = EXIT_FAILURE;
	}

exit:
	if (fileBuf) {
		free(fileBuf);
	}

	return ret;
}

static int rsa_verify_manifest_signature(const struct manifest *man, const char *key_path)
{
	int ret;
	RsaKey rsaKey;
	struct key_signature key_struct;
	uint8_t modulus[256];
	uint32_t exponent = 0;
	word32 e_size = sizeof(exponent);
	word32 n_size = sizeof(modulus);

	memcpy(&key_struct, &man->key_sig, sizeof(key_struct));

	ret = wc_InitRsaKey(&rsaKey, NULL);
	if (ret != 0) {
		ERROR("Init RSA key failed! %d\n", ret);
		goto exit;
	}

	ret = rsa_load_key_file(key_path, &rsaKey);
	if (ret != 0) {
		ERROR("Failed to load RSA key %s! (%d)\n", key_path, ret);
		goto exit;
	}

	ret = wc_RsaFlattenPublicKey(&rsaKey, (byte *)&exponent, &e_size,
					      (byte *)&modulus, &n_size);
	if (ret != 0) {
		ERROR("Failed to extract Secure Boot Manifest Key exponent and modulus!\n");
		return ret;
	}

	if (swap_bytes(modulus, sizeof(modulus))) {
		ERROR("Failed to change RSA Signature endianess!\n");
		ret = EXIT_FAILURE;
		goto exit;
	}

	if (memcmp(modulus, key_struct.modulus, sizeof(modulus))) {
		ERROR("RSA key modulus does not match the key used in manifest!\n");
		ret = EXIT_FAILURE;
		goto exit;
	}

	if (exponent != key_struct.exponent) {
		ERROR("RSA key exponent does not match the key used in manifest!\n");
		ERROR("%x != %x\n", exponent, key_struct.exponent);
		ret = EXIT_FAILURE;
		goto exit;
	}

	if (swap_bytes(key_struct.signature, sizeof(key_struct.signature))) {
		ERROR("Failed to change RSA Signature endianess!\n");
		ret = EXIT_FAILURE;
		goto exit;
	}

	ret = wc_SignatureVerify(WC_HASH_TYPE_SHA256, WC_SIGNATURE_TYPE_RSA_W_ENC,
				 (byte *)man,
				 MANIFEST_SIZE - sizeof(struct key_signature),
				 key_struct.signature, sizeof(key_struct.signature),
				 &rsaKey, sizeof(rsaKey));

	if (ret != 0) {
		ERROR("Manifest signature verification failed!\n");
		goto exit;
	}

exit:
	wc_FreeRsaKey(&rsaKey);

	return ret;
}

static int verify_sb_key_hash(const struct manifest *man)
{
	int ret;
	RsaKey rsaKey;
	uint8_t key_hash[32];

	ret = wc_InitRsaKey(&rsaKey, NULL);
	if (ret != 0) {
		ERROR("Init RSA key failed! %d\n", ret);
		goto exit;
	}

	ret = rsa_load_key_file(params.sbm_key_file, &rsaKey);
	if (ret != 0) {
		ERROR("Failed to load RSA key %s!\n", params.sbm_key_file);
		goto exit;
	}

	ret = calculate_key_hash(&rsaKey, key_hash);
	if (ret != 0) {
		ERROR("Failed to calculate RSA key hash %s!\n", params.sbm_key_file);
		goto exit;
	}

	if (memcmp(key_hash, man->hash, WC_SHA256_DIGEST_SIZE)) {
		ERROR("Secure Boot Key SHA256 hash does not match the key hash in the Key"
			" Manifest!\n");
		print_sha256(key_hash);
		print_sha256(man->hash);
		ret = EXIT_FAILURE;
	}

exit:
	wc_FreeRsaKey(&rsaKey);

	return ret;
}

static int verify_ibb_hash(const struct manifest *man, const byte *fileBuf, int fileLen)
{
	int ret;
	uint8_t ibb_hash[32];

	ret = calculate_ibb_hash(&fileBuf[fileLen - IBB_SIZE], ibb_hash);
	if (ret != 0) {
		ERROR("Failed to calcualte IBB hash!\n");
		return ret;
	}

	if (memcmp(ibb_hash, man->hash, WC_SHA256_DIGEST_SIZE)) {
		ERROR("Calculated IBB SHA256 hash does not match the IBB hash in Secure Boot"
			" Manifest!\n");
		ret = EXIT_FAILURE;
	}

	return ret;
}

static int cmd_verify(void)
{
	int ret = 0;
	word32 fileLen;
	byte *fileBuf = NULL;
	word32 offset;
	const struct manifest *km = NULL;
	const struct manifest *sbm = NULL;

	if (params.file_name) {
		ret = load_file_to_buffer(params.file_name, &fileBuf, &fileLen);
		if (ret < 0) {
			goto exit;
		}
	} else {
		ERROR("Image file not given\n");
		return EXIT_FAILURE;
	}

	if (fileLen >= (IBB_SIZE + SB_MANIFEST_SIZE + KEY_MANIFEST_SIZE)) {
		/* Treat it as a full binary */
		offset = fileLen - IBB_SIZE - SB_MANIFEST_SIZE - KEY_MANIFEST_SIZE;

		if (*(uint32_t *)&fileBuf[offset] == KEY_MANIFEST_MAGIC) {
			km = (struct manifest *)&fileBuf[offset];
		}

		offset = fileLen - IBB_SIZE - SB_MANIFEST_SIZE;

		if (*(uint32_t *)&fileBuf[offset] == SB_MANIFEST_MAGIC) {
			sbm = (struct manifest *)&fileBuf[offset];
		}

		if (km == NULL && sbm == NULL) {
			ERROR("Input file invalid, manifests not found\n");
			ret = EXIT_FAILURE;
			goto exit;
		}
	} else if (fileLen == (IBB_SIZE + SB_MANIFEST_SIZE)) {
		offset = fileLen - IBB_SIZE - SB_MANIFEST_SIZE;

		if (*(uint32_t *)&fileBuf[offset] == SB_MANIFEST_MAGIC) {
			sbm = (struct manifest *)&fileBuf[offset];
		}
	} else {
		ERROR("Input file invalid size\n");
		ret = EXIT_FAILURE;
		goto exit;
	}

	if (!params.sbm_key_file) {
		ERROR("Secure Boot Manifest key file not given\n");
		ret = EXIT_FAILURE;
		goto exit;
	}

	if (km != NULL && !params.km_key_file) {
		ERROR("Key Manifest key file not given, but Key Manifest found in the input"
			" file\n");
		ERROR("Can not verify Key Manifest!\n");
		ret = EXIT_FAILURE;
		goto exit;
	}

	if (sbm == NULL) {
		ERROR("Secure Boot Manifest not found in the input file, but is required!\n");
		ret = EXIT_FAILURE;
		goto exit;
	}

	/* Verify hash type is supported */
	if (wc_HashGetDigestSize(WC_HASH_TYPE_SHA256) != 32) {
		ERROR("Hash type SHA256 not supported!\n");
		ret = EXIT_FAILURE;
		goto exit;
	}

	if (km != NULL) {
		printf("Verifying Key Manifest...\n");

		ret = 0;
		if (km->header.size != MANIFEST_SIZE) {
			ERROR("Invalid Key Manifest size field! (%d != 1024)\n",
			km->header.size);
			ret--;
		}

		if (km->km_id == 0 || km->km_id > 15) {
			ERROR("Invalid Key Manifest ID field! (%d %s)\n",
				km->km_id, km->km_id == 0 ? "== 0" : "> 15");
			ret--;
		}

		if (km->key_sig.exponent != RSA_KEY_EXPONENT) {
			ERROR("Incorrect RSA key exponent in the Key Manifest!"
				" 0x%x != 0x10001\n", km->key_sig.exponent);
			ret--;
		}

		if (!mem_is_zero(km->reserved, sizeof(km->reserved))) {
			ERROR("Key Manifest reserved space is not zeroed!\n");
			ret--;
		}

		if (!mem_is_zero((byte *)(km + 1), KEY_MANIFEST_PAD_SIZE)) {
			ERROR("Key Manifest is not zero-padded to 4KB!\n");
			ret--;
		}

		ret -= verify_sb_key_hash(km);
		ret -= rsa_verify_manifest_signature(km, params.km_key_file);

		if (ret == 0) {
			printf("Key Manifest verification successful!\n\n");
			printf("TXE should be provisioned with the following key hash:\n");
			print_key_hash_from_manifest(km);
			printf("TXE should be provisioned with the following Key Manifest ID:"
				" 0x%02x\n\n", km->km_id);
		} else {
			printf("Key Manifest verification FAILED!\n\n");
			goto exit;
		}
	}

	printf("Verifying Secure Boot Manifest...\n");

	ret = 0;
	if (sbm->header.size != MANIFEST_SIZE) {
		ERROR("Invalid Secure Boot Manifest size field! (%d != 1024)\n",
			sbm->header.size);
		ret--;
	}

	if (sbm->key_sig.exponent != RSA_KEY_EXPONENT) {
		ERROR("Incorrect RSA key exponent in the Secure Boot Manifest!"
			" 0x%x != 0x10001\n", sbm->key_sig.exponent);
		ret--;
	}

	ret -= verify_ibb_hash(sbm, fileBuf, fileLen);
	ret -= rsa_verify_manifest_signature(sbm, params.sbm_key_file);

	if (ret == 0) {
		printf("Secure Boot Manifest verification successful!\n\n");
		if (km == NULL) {
			printf("TXE should be provisioned with the following key hash:\n");
			print_key_hash_from_manifest(sbm);
		}
	} else {
		printf("Secure Boot Manifest verification FAILED!\n\n");
	}

exit:
	if (fileBuf) {
		free(fileBuf);
	}

	return ret;
}

static int cmd_keyhash(void)
{
	int ret;
	RsaKey key;
	byte keyhash[WC_SHA256_DIGEST_SIZE];

	if (!params.file_name) {
		ERROR("Key file not given\n");
		return EXIT_FAILURE;
	}

	if (wc_HashGetDigestSize(WC_HASH_TYPE_SHA256) != WC_SHA256_DIGEST_SIZE) {
		ERROR("Hash type SHA256 not supported!\n");
		return EXIT_FAILURE;
	}

	ret = wc_InitRsaKey(&key, NULL);
	if (ret != 0) {
		ERROR("Init RSA key failed! %d\n", ret);
		return ret;
	}

	ret = rsa_load_key_file(params.file_name, &key);
	if (ret != 0) {
		ERROR("Failed to load Secure Boot Manifest Key!\n");
		goto exit;
	}

	ret = calculate_key_hash(&key, keyhash);
	if (ret != 0) {
		ERROR("Failed to calculate RSA key hash of '%s'!\n", params.file_name);
		goto exit;
	}

	printf("SHA256 of RSA key '%s':\n", params.file_name);
	print_sha256(keyhash);

exit:
	wc_FreeRsaKey(&key);
	return ret;
}

static struct command {
	const char *name;
	const char *optstring;
	int (*cb)(void);
} commands[] = {
	{ "print", "h?", cmd_print },
	{ "keyhash", "h?", cmd_keyhash },
	{ "verify", "h?", cmd_verify },
	{ "generate-sbm", "o:h?", cmd_generate_sbm },
	{ "generate-km", "o:h?", cmd_generate_km },
};

enum {
	LONGOPT_START = 256,
	LONGOPT_SBM_KEY_FILE = LONGOPT_START,
	LONGOPT_SBM_OEM_DATA,
	LONGOPT_KM_KEY_FILE,
	LONGOPT_KM_ID,
	LONGOPT_MANIFEST_SVN,
	LONGOPT_END,
};

static struct option long_options[] = {
	{"help",            0,                 0, 'h'},
	{"version",         0,                 0, 'v'},
	{"output",          required_argument, 0, 'o'},
	{"sbm-key-file",    required_argument, 0, LONGOPT_SBM_KEY_FILE},
	{"oem-data",        required_argument, 0, LONGOPT_SBM_OEM_DATA},
	{"km-key-file",     required_argument, 0, LONGOPT_KM_KEY_FILE},
	{"km-id",           required_argument, 0, LONGOPT_KM_ID},
	{"svn",             required_argument, 0, LONGOPT_MANIFEST_SVN},
	{NULL,              0,                 0,  0 }
};

static bool valid_opt(word32 i, int c)
{
	/* Check if it is one of the optstrings supported by the command. */
	if (strchr(commands[i].optstring, c))
		return true;

	/*
	 * Check if it is one of the non-ASCII characters. Currently, the
	 * non-ASCII characters are only checked against the valid list
	 * irrespective of the command.
	 */
	return c >= LONGOPT_START && c < LONGOPT_END;
}

static void usage(const char *name)
{
	printf("txesbmantool: Utility for generating TXE Secure Boot manifests\n"
	       "USAGE:\n"
	       " %s FILE COMMAND\n\n"
	       " %s --help\n\n"
	       " %s --version\n\n"
	       "COMMANDs:\n"
	       " print\n"
	       " keyhash\n"
	       " verify --sbm-key-file <FILE> [--km-key-file <FILE>]\n"
	       " generate-km --sbm-key-file <FILE> --km-key-file <FILE> [--km-id <ID>] [--svn <SVN>] [-o <FILE>]\n"
	       " generate-sbm --sbm-key-file <FILE> [--oem-data <FILE>] [--svn <SVN>] [-o <FILE>]\n"
	       "\nOPTIONS:\n"
	       " -o <FILE>               : Output manifest file (optional)\n"
	       " --sbm-key-file <FILE>   : Secure Boot (SB) Manifest key file\n"
	       " --km-key-file <FILE>    : Key Manifest key file\n"
	       " --km-id <ID>            : Key Manifest ID number (default 1)\n"
	       " --oem-data <FILE>       : Optional OEM data file to be included in SB Manifest\n"
	       " --svn <SVN>             : Security Version Number for the manifest (default 2),\n"
	       "                           Key Manifest SVN range 1-15, SB Manifest SVN range 0-63\n"
	       "\n",
	       name, name, name);
}

static int no_file_cmd_args(int argc, char **argv)
{
	int c;
	int option_index;

	c = getopt_long(argc, argv, "?hv", long_options, &option_index);

	switch (c) {
	case 'v':
		printf("Version: %s\n", tool_version);
		return 0;
	case 'h':
	case '?':
		usage(argv[0]);
		return 0;
	default:
		usage(argv[0]);
		return 1;
	}

	usage(argv[0]);
	return 1;
}

int main(int argc, char **argv)
{
	if (argc < 3) {
		return no_file_cmd_args(argc, argv);
	}

	const char *prog_name = argv[0];
	const char *file_name = argv[1];
	const char *cmd = argv[2];

	word32 i;
	params.file_name = file_name;
	params.override_km_id = false;
	params.override_svn = false;

	for (i = 0; i < ARRAY_SIZE(commands); i++) {
		if (strcmp(cmd, commands[i].name))
			continue;

		int c;
		int option_index;

		while (1) {
			c = getopt_long(argc, argv, commands[i].optstring,
					long_options, &option_index);

			if (c == -1)
				break;

			if (!valid_opt(i, c)) {
				if (c < LONGOPT_START) {
					ERROR("Invalid option -- '%c'\n", c);
				} else {
					ERROR("Invalid option -- '%d'\n", c);
				}
				usage(prog_name);
				return 1;
			}

			switch (c) {
			case 'o':
				params.output = optarg;
				break;
			case LONGOPT_SBM_KEY_FILE:
				params.sbm_key_file = optarg;
				break;
			case LONGOPT_SBM_OEM_DATA:
				params.oem_data_file = optarg;
				break;
			case LONGOPT_KM_KEY_FILE:
				params.km_key_file = optarg;
				break;
			case LONGOPT_KM_ID:
				params.km_id = atoi(optarg);
				params.override_km_id = true;
				break;
			case LONGOPT_MANIFEST_SVN:
				params.svn = atoi(optarg);
				params.override_svn = true;
				break;
			case 'h':
			case '?':
			default:
				usage(prog_name);
				return 1;
			}
		}

		break;
	}

	if (i == ARRAY_SIZE(commands)) {
		printf("No command matches %s!\n", cmd);
		usage(prog_name);
		return 1;
	}

	if (commands[i].cb())
		return 1;

	return 0;
}
