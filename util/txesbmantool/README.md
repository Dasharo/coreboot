# txesbmantool

`txesbmantool` - utility generating the TXE Secure Boot manifests
(abbreviation of TXE Secure Boot Manifest Tool). The utility aims to help
develop, sign, and automate the creation process of TXE Secure Boot-supported
images.

## Suported commands

The txesbmantool supports the following commands:

```bash
txesbmantool: Utility for generating TXE Secure Boot manifests
USAGE:
 ./txesbmantool FILE COMMAND

 ./txesbmantool --help

 ./txesbmantool --version

COMMANDs:
 print
 keyhash
 verify --sbm-key-file <FILE> [--km-key-file <FILE>]
 generate-km --sbm-key-file <FILE> --km-key-file <FILE> [--km-id <ID>] [--svn <SVN>] [-o <FILE>]
 generate-sbm --sbm-key-file <FILE> [--oem-data <FILE>] [--svn <SVN>] [-o <FILE>]

OPTIONS:
 -o <FILE>               : Output manifest file (optional)
 --sbm-key-file <FILE>   : Secure Boot (SB) Manifest key file
 --km-key-file <FILE>    : Key Manifest key file
 --km-id <ID>            : Key Manifest ID number (default 1)
 --oem-data <FILE>       : Optional OEM data file to be included in SB Manifest
 --svn <SVN>             : Security Version Number for the manifest (default 2),
                           Key Manifest SVN range 1-15, SB Manifest SVN range 0-63
```

### `print`

`print` - parses the input `FILE` and prints Key Manifest and Secure Boot
Manifest if found. Example output:

```bash
txesbmantool coreboot.rom print
Key Manifest:
    Magic: $SKM
    Version: 1
    Size: 1024
    Svn: 2
    Key Manifest ID: 0x01
    SB Key hash: E671B83E4BB9AAC11D1578D10999922EB4EBFCFB0A1E30A7A37DAF33163FA2BB
    Key modulus:
        45 40 DC B5 21 AF 0A ED A0 1F DF 15 66 92 98 E1 
        D6 3E 08 61 3E BC 45 8B 9B E7 A7 66 F3 56 DC 56 
        94 12 D1 D3 CB D1 F7 25 60 DA C9 DC 3C 8A 94 68 
        73 59 AB 99 CE 5F B8 CE F9 FD 9F 2B 5C FB FA B7 
        35 0C 5B 23 68 64 81 44 AB F3 BE 27 A3 10 A2 2E 
        F7 A1 F4 31 6A 59 17 6A A0 89 4B 6C 39 58 1F 4D 
        97 6A 62 05 48 1C 21 22 A5 35 1A 64 A7 A0 A3 42 
        20 0E CC C7 71 99 24 83 DD 8F 4B 14 61 1A E0 2E 
        70 52 2C 82 00 22 08 02 C7 17 0B 4D 9B DD BB A7 
        66 63 12 E7 74 4C 5E 9B C0 0C BC EC 49 D3 5B 6D 
        49 E5 E0 20 96 8E 4B 39 74 28 DA 7A 7F 20 95 E4 
        7F 5C 4D 38 C9 57 12 49 04 BC B7 0B ED ED A2 69 
        12 83 0C C8 CE 56 3B 8D 04 A2 27 EA D1 05 52 51 
        E4 A6 E8 3B 00 58 1D 9E 4D 33 00 9D 91 B6 36 AC 
        AA 48 C9 24 AD 50 59 E2 E4 00 B7 A5 9C 0E BC 0A 
        DF CB 47 A1 B7 F1 20 38 09 F2 BF EE 34 CA 52 C9 
    Key exponent: 0x10001
    Signature:
        4C 00 31 A2 E6 DB 99 EC C6 B3 84 AE 9E 28 BC AE 
        18 1A F8 09 B9 5A 38 6F A4 15 93 D5 F6 E6 C2 A5 
        FC 9D 37 3E 80 41 0B AB 44 C7 86 33 26 1A CB AF 
        BC FD D6 8B 18 CA 65 75 15 5D 32 75 85 0B AB 86 
        82 B4 24 67 2C FE DD 2E BB 69 9F B8 4B 1D 80 84 
        DE 09 C6 80 8B 46 0D D8 4C 4C 75 EC F2 61 67 CA 
        6B A5 39 20 20 D3 29 72 65 98 82 A0 41 ED FF 9E 
        68 44 CE 47 5D 90 DC C1 8E 7F 19 C6 A9 9A 27 DE 
        16 B7 12 A5 5D EA 61 53 4B 17 E7 E3 5E 71 04 FE 
        29 DE D4 0C 44 B2 19 1E A3 41 29 C6 C6 AE 4D C0 
        CA 97 A0 53 B2 55 D5 CE E6 B1 45 5B 38 34 2A C1 
        16 5F 56 F4 66 87 8D 8C 75 89 D1 6F 2D 9E 0B FC 
        BF 16 C1 2F 53 DD AA 0A 0B 46 14 A5 0D 68 8F 24 
        45 F4 FB 4E 4A 99 8F 33 B6 D0 F9 70 42 ED F5 F6 
        63 87 BB 8C EB DD 6C 8E B5 62 EE EB C5 CC 94 DA 
        E5 B6 BF 1F 4A CA F6 F1 E8 D7 7C 79 AF 3B 6A C7 
Secure Boot Manifest:
    Magic: $VBM
    Version: 1
    Size: 1024
    Svn: 2
    IBB hash: C27838A5EB12A57CE3880896C83E7E437EF6D4DF94B8808A36C2779B0B3DE731
    OEM Data:
        00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 
        00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 
        00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 
        00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 
        00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 
        00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 
        00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 
        00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 
        00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 
        00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 
        00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 
        00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 
        00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 
        00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 
        00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 
        00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 
        00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 
        00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 
        00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 
        00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 
        00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 
        00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 
        00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 
        00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 
        00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 
    Key modulus:
        59 8E 0E 9C 78 BA 84 9E 08 70 A0 2E 03 C9 EC 60 
        51 AD 75 77 6A BD D8 51 1E 2A B4 2C 3E 14 8C A9 
        5A AF 36 0F 6F 17 8C E3 C2 E6 40 0F 8A DF E0 1F 
        FC 31 74 31 A6 D3 E0 D5 01 4A D6 40 11 A5 CB 3A 
        19 01 D6 4C 52 E0 47 B2 2E C2 3F CD E0 1B 53 51 
        9D C0 7D 6E D8 8D D7 D9 8E C3 FE 85 19 19 29 92 
        11 C3 B4 53 63 E6 61 12 C4 8B F9 0D 5F DB D1 50 
        64 CD 78 CB 10 0B 84 92 42 AE 1E 81 4E 57 CF 9F 
        D9 85 CA 4A F4 CE DA 03 BC 5E 0D 7C BD E9 EB 13 
        C3 C3 C2 4D EF 21 08 15 5A EC EA E8 F8 CD C0 DE 
        9B 57 55 DF B8 C4 40 27 18 29 A5 98 85 09 5C EF 
        DE 39 B5 A8 F5 69 D7 D2 1F A9 A4 9F D2 61 C5 82 
        1D 5E D0 3B 69 CA DF 8B A0 3F E4 C4 85 15 2D EB 
        F6 32 5F 70 1E DF 93 21 AD F7 65 40 E2 4B B0 26 
        D2 DA F1 3C F0 9A A9 80 3C E6 F9 71 46 C1 5F 50 
        65 BE 70 80 3A 1A DA 51 79 75 04 B3 3F 17 FF AF 
    Key exponent: 0x10001
    Signature:
        DF 5F B8 66 76 0F CC A9 93 C7 28 0F EF 32 14 7A 
        1A 23 04 2A 75 85 24 0A 85 7F E5 B8 CE 24 44 C1 
        D6 73 5B D6 30 BD A3 66 15 85 6A 3A 8E FC 8F 56 
        75 71 28 86 0F F3 E3 1C 20 A4 2E 33 15 45 E5 24 
        1A BB 69 A0 FD D8 7A A1 34 2E 7A 40 60 E1 4C 51 
        D3 BB 73 BC 1A A5 43 E5 6F B6 54 4F 31 2A 65 9B 
        DD 1A BE 45 5A 7A E5 15 44 76 EC 94 89 33 14 11 
        C6 18 39 ED 02 D7 B3 5C 56 23 2A 58 18 87 EE 2A 
        0A F5 FD E9 5F 94 B3 92 17 21 59 3E A4 0F CF 4A 
        99 95 33 FB AD 73 CF 1C 7F 1F CB 31 EC 2D 57 0D 
        8B 3E DB CF D4 F0 62 26 26 0F D9 17 DD BC 3C 1F 
        28 76 E4 81 D5 CF 04 BB 5D 33 82 D8 EB DA 64 45 
        14 A9 24 90 5D 93 E8 D6 34 1C 0C 2B 6F 18 4F DF 
        45 6E EE 2A FB 00 D2 B7 BC 36 9D D9 91 FF A7 39 
        39 4D 96 48 DD 7C 43 29 73 7F 6A 8A 73 F2 B0 57 
        E4 B7 2A 34 A1 F1 83 A5 7F A7 17 6C 1A E2 EC 26 
```

### `verify`

`verify` - parses the input `FILE` and verifies whether the input `FILE` has
been correctly signed for TXE Secure Boot. Additional input parameters are
paths to the private signing keys to perform verification. If the image is
expected to have a Key Manifest, one must also pass the key used to sign Key
Manifest. The tool will also print the expected TXE image configuration.
Example output:

```bash
./txesbmantool coreboot.rom verify \
    --sbm-key-file sb_privkey_sample.pem \
    --km-key-file oem_privkey_sample.pem 
Verifying Key Manifest...
Key Manifest verification successful!

TXE image should be provisioned with the following key hash:
AE6491B401B639DB9FA6E86D22FACE37F846E506328FDBF4327802C271FFE6FF
TXE should be provisioned with the following Key Manifest ID: 0x01

Verifying Secure Boot Manifest...
Secure Boot Manifest verification is successful!
```

If the Key Manifest is found in the binary but the `--km-key-file` was not
passed with a valid key, the tool will output an error:

### `generate-km`

`generate-km` - creates a signed Key Manifest and injects it into the input
`FILE`. Secure Boot Manifest key is taken as `--sbm-key-file <FILE>` parameter
and Key Manifest key as `--km-key-file <FILE>` parameter. Key Manifest is used
to authorize a Secure Boot Manifest key that will be eligible to sign Secure
Boot Manifests. One may optionally provide a custom Key Manifest ID
(`--km-id`), SVN (`--svn`), and save the resulting Key Manifest as a separate
file with `-o` parameter. Example:

```bash
./txesbmantool coreboot.rom generate-km \
    --sbm-key-file sb_privkey_sample.pem \
    --km-key-file oem_privkey_sample.pem \
    --km-id 1 --svn 2
SHA256 of the Secure Boot Manifest Key (Little Endian):
E671B83E4BB9AAC11D1578D10999922EB4EBFCFB0A1E30A7A37DAF33163FA2BB

Key Manifest generated successfully
```

### `generate-sbm`

`generate-sbm` - creates a signed Secure Boot Manifest and injects it to the
input `FILE`. Secure Boot Manifest key is taken as `--sbm-key-file <FILE>`
parameter. The Secure Boot Manifest contains the hash of the IBB that is being
verified by TXE. One may optionally provide a custom SVN (`--svn`) and save
the resulting Key Manifest as a separate file with `-o` parameter. Example:

```bash
./txesbmantool coreboot.rom generate-sbm \
    --sbm-key-file sb_privkey_sample.pem \
    --svn 2
SHA256 of the IBB (Little Endian):
C27838A5EB12A57CE3880896C83E7E437EF6D4DF94B8808A36C2779B0B3DE731

Secure Boot Manifest generated successfully
```

The utility may also take an additional max 400 bytes block of data as
`--oem-data` parameter. It is not used by hardware, however, it is verified as
a part of the manifest verification and can be used for any purpose.
Currently, coreboot does not use it at all, as there is no need for it.

### `keyhash`

`keyhash` - takes a key as an input `FILE` and prints out the SHA256 of the
key modulus+exponent. It is useful when preparing the TXE image for Secure
Boot, where one needs to provide the SHA256 of the key to be set in the
fuses/emulated fuses. Example:

```bash
./txesbmantool oem_privkey_sample.pem keyhash
SHA256 of RSA key 'oem_privkey_sample.pem':
AE6491B401B639DB9FA6E86D22FACE37F846E506328FDBF4327802C271FFE6FF
```

In the above example, the hash corresponds to the key used to sign the Key
Manifest. The same hash can be found in the example output of `verify`
command.
