#!/usr/bin/env python3
"""
Generate a Windows Security Catalog (.cat) for UEFI firmware updates.

The catalog structure is modelled on real firmware update catalogs produced
by Microsoft's WHQL signing infrastructure (signtool / makecab / inf2cat).
It is a PKCS#7 v1 SignedData whose encapContentInfo holds a Certificate
Trust List (CTL) with the following layout:

    CTL ::= SEQUENCE {
        subjectUsage     SEQUENCE { OID szOID_CATALOG_LIST }
        listIdentifier   OCTET STRING (16 random bytes — required)
        ctlThisUpdate    UTCTime
        subjectAlgorithm AlgorithmIdentifier {
                             OID  1.3.6.1.4.1.311.12.1.3  (multi-hash marker)
                             NULL
                         }
        ctlEntries       SEQUENCE OF CTLEntry
        [0] IMPLICIT     CTL extensions (OS compat string, optional HWID)
    }

Each file gets TWO CTLEntry records — one keyed by SHA-1 (no SPC) and one
keyed by SHA-256 (with SpcIndirectDataContent).  Each entry carries:
    1. szOID_CAT_NAMEVALUE_FLAGS  (1.3.6.1.4.1.311.12.2.3)
       SET { [2] PRIMITIVE {} }                ← [0] for PE, [2] for non-PE
    2. szOID_CAT_NAMEVALUE "OSAttr"  e.g. "2:10.0"
    3. szOID_CAT_NAMEVALUE "File"    e.g. "firmware.bin"

The SHA-256 entry additionally carries:
    4. SpcIndirectDataContent  (1.3.6.1.4.1.311.2.1.4)
       using SpcLink           (1.3.6.1.4.1.311.2.1.25) for non-PE files.

Inside szOID_CAT_NAMEVALUE attributes:
    • attribute name  = BMPString (UTF-16-BE, no null terminator) — standard DER
    • attribute value = OCTET STRING containing null-terminated UTF-16-LE — Windows-specific

Certificate requirements
------------------------
  Windows Update submission:
    Submit the package to Microsoft Hardware Dev Center.

  Direct distribution:
    Sign with an EV code-signing certificate from a Microsoft-trusted CA.

  Development / test-signing mode on Windows:
    Any certificate works; enable test signing with:
      bcdedit /set testsigning on
    Import the signing cert into BOTH stores (Local Machine):
      certutil -addstore Root cert.pem
      certutil -addstore TrustedPublisher cert.pem

Usage
-----
  capsule_catalog.py --cert cert.pem --key key.pem \\
      [--hwid 'UEFI\\RES_{GUID}'] --out firmware.cat \\
      firmware.inf firmware.bin

Requirements
------------
  osslsigncode >= 2.0  (dnf install osslsigncode / apt install osslsigncode)
"""

import argparse
import hashlib
import os
import secrets
import struct
import subprocess
import sys
import tempfile
import zlib
from datetime import datetime, timezone


# ---------------------------------------------------------------------------
# Minimal ASN.1 / DER helpers — no external dependencies
# ---------------------------------------------------------------------------

def _length(n: int) -> bytes:
    if n < 0x80:
        return bytes([n])
    b = n.to_bytes((n.bit_length() + 7) // 8, 'big')
    return bytes([0x80 | len(b)]) + b


def _tlv(tag: int | bytes, value: bytes) -> bytes:
    t = bytes([tag]) if isinstance(tag, int) else tag
    return t + _length(len(value)) + value


def _seq(*items: bytes) -> bytes:
    return _tlv(0x30, b''.join(items))


def _set(*items: bytes) -> bytes:
    return _tlv(0x31, b''.join(items))


def _oid(dotted: str) -> bytes:
    parts = [int(x) for x in dotted.split('.')]
    body = bytes([40 * parts[0] + parts[1]])
    for n in parts[2:]:
        if n == 0:
            body += bytes([0])
        else:
            enc: list[int] = []
            while n:
                enc.append(n & 0x7F)
                n >>= 7
            enc.reverse()
            body += bytes([(b | 0x80) if i < len(enc) - 1 else b
                           for i, b in enumerate(enc)])
    return _tlv(0x06, body)


def _octet(data: bytes) -> bytes:
    return _tlv(0x04, data)


def _null() -> bytes:
    return b'\x05\x00'


def _utctime(dt: datetime) -> bytes:
    return _tlv(0x17, dt.strftime('%y%m%d%H%M%SZ').encode('ascii'))


def _int(n: int) -> bytes:
    if n == 0:
        return b'\x02\x01\x00'
    nb = (n.bit_length() + 8) // 8
    b = n.to_bytes(nb, 'big')
    while len(b) > 1 and b[0] == 0 and not (b[1] & 0x80):
        b = b[1:]
    return _tlv(0x02, b)


def _bmp(s: str) -> bytes:
    """BMPString (tag 0x1E): UTF-16-BE, no null terminator — standard DER."""
    return _tlv(0x1E, s.encode('utf-16-be'))


def _wstr(s: str) -> bytes:
    """OCTET STRING containing null-terminated UTF-16-LE — Windows catalog value encoding."""
    return _octet(s.encode('utf-16-le') + b'\x00\x00')


# ---------------------------------------------------------------------------
# OIDs
# ---------------------------------------------------------------------------

_OID_SIGNED_DATA      = '1.2.840.113549.1.7.2'
_OID_CTL              = '1.3.6.1.4.1.311.10.1'    # szOID_CTL
_OID_CAT_LIST         = '1.3.6.1.4.1.311.12.1.1'  # szOID_CATALOG_LIST
_OID_CAT_MULTI_HASH   = '1.3.6.1.4.1.311.12.1.3'  # subjectAlgorithm: multi-hash marker
_OID_NAME_VALUE       = '1.3.6.1.4.1.311.12.2.1'  # szOID_CAT_NAMEVALUE
_OID_CAT_FLAGS        = '1.3.6.1.4.1.311.12.2.3'  # file type flag
_OID_SPC              = '1.3.6.1.4.1.311.2.1.4'   # SpcIndirectDataContent
_OID_SPC_LINK         = '1.3.6.1.4.1.311.2.1.25'  # SpcLink (non-PE files)
_OID_SHA1             = '1.3.14.3.2.26'
_OID_SHA256           = '2.16.840.1.101.3.4.2.1'

# File-type flag value inside the szOID_CAT_FLAGS SET.
# [2] PRIMITIVE length 0 for non-PE files; [0] PRIMITIVE length 0 for PE.
_FLAG_NON_PE = bytes([0x82, 0x00])
_FLAG_PE     = bytes([0x80, 0x00])

# SpcLink for non-PE: SEQUENCE { OID spcLink, [2]{ [0]{} } }
# [2] constructed (0xA2), length 2; [0] primitive (0x80), length 0
_SPC_LINK_NON_PE = _seq(
    _oid(_OID_SPC_LINK),
    _tlv(0xA2, _tlv(0x80, b'')),   # [2] EXPLICIT { [0] PRIMITIVE {} }
)


# ---------------------------------------------------------------------------
# File hashing
# ---------------------------------------------------------------------------

def _sha1_file(path: str) -> bytes:
    h = hashlib.sha1()
    with open(path, 'rb') as f:
        for chunk in iter(lambda: f.read(1 << 16), b''):
            h.update(chunk)
    return h.digest()


def _sha256_file(path: str) -> bytes:
    h = hashlib.sha256()
    with open(path, 'rb') as f:
        for chunk in iter(lambda: f.read(1 << 16), b''):
            h.update(chunk)
    return h.digest()


# ---------------------------------------------------------------------------
# Per-file CTL attributes
# ---------------------------------------------------------------------------

def _cat_flags_attr(is_pe: bool = False) -> bytes:
    """
    szOID_CAT_FLAGS attribute (OID 1.3.6.1.4.1.311.12.2.3).

    SET contains a single context-specific primitive tag:
      [0] for PE / executable files
      [2] for non-PE / data files
    """
    flag = _FLAG_PE if is_pe else _FLAG_NON_PE
    return _seq(_oid(_OID_CAT_FLAGS), _set(flag))


def _name_value_attr(name: str, value: str) -> bytes:
    """
    szOID_CAT_NAMEVALUE per-file attribute.

    Structure (used inside per-file CTLEntry attributes SET):
        SEQUENCE {
            OID  szOID_CAT_NAMEVALUE
            SET {
                SEQUENCE {
                    BMPString   <name>          (UTF-16-BE, standard DER)
                    INTEGER     0x10010001      (CRYPTCAT_ATTR_* flags)
                    OCTET STRING <value>        (null-terminated UTF-16-LE)
                }
            }
        }

    Note: SEQUENCE sits directly inside SET — no extra OCTET STRING wrapper.
    """
    inner = _seq(
        _bmp(name),
        _int(0x10010001),
        _wstr(value),
    )
    return _seq(_oid(_OID_NAME_VALUE), _set(inner))


def _spc_attr(sha256: bytes) -> bytes:
    """
    SpcIndirectDataContent attribute for non-PE files.

    Structure:
        SEQUENCE {
            OID  SpcIndirectDataContent
            SET {
                SEQUENCE {
                    SEQUENCE {
                        OID  SpcLink (1.3.6.1.4.1.311.2.1.25)
                        [2] { [0] {} }        ← empty link
                    }
                    SEQUENCE {
                        SEQUENCE { OID sha256, NULL }
                        OCTET STRING <sha256>
                    }
                }
            }
        }
    """
    digest = _seq(
        _seq(_oid(_OID_SHA256), _null()),
        _octet(sha256),
    )
    inner = _seq(_SPC_LINK_NON_PE + digest)
    return _seq(_oid(_OID_SPC), _set(inner))


# ---------------------------------------------------------------------------
# CTL entry builders
# ---------------------------------------------------------------------------

def _ctl_entry_sha1(path: str, os_val: str) -> bytes:
    """
    SHA-1 CTL entry: file type flag + OSAttr + File name (no SPC).
    Subject identifier is the SHA-1 hash of the file (20 bytes).
    """
    sha1     = _sha1_file(path)
    filename = os.path.basename(path)

    attrs = _set(
        _cat_flags_attr(is_pe=False) +
        _name_value_attr('OSAttr', os_val) +
        _name_value_attr('File', filename)
    )
    return _seq(_octet(sha1), attrs)


def _ctl_entry_sha256(path: str, os_val: str) -> bytes:
    """
    SHA-256 CTL entry: file type flag + OSAttr + File name + SpcIndirectDataContent.
    Subject identifier is the SHA-256 hash of the file (32 bytes).
    """
    sha256   = _sha256_file(path)
    filename = os.path.basename(path)

    attrs = _set(
        _cat_flags_attr(is_pe=False) +
        _name_value_attr('OSAttr', os_val) +
        _name_value_attr('File', filename) +
        _spc_attr(sha256)
    )
    return _seq(_octet(sha256), attrs)


# ---------------------------------------------------------------------------
# Global CTL extension attributes (different wrapper than per-file attributes)
# ---------------------------------------------------------------------------

def _name_value_ext(name: str, value: str) -> bytes:
    """
    szOID_CAT_NAMEVALUE for the [0] CTL extensions block.

    Structure (note OCTET STRING wrapper instead of SET):
        SEQUENCE {
            OID  szOID_CAT_NAMEVALUE
            OCTET STRING {
                SEQUENCE {
                    BMPString   <name>
                    INTEGER     0x10010001
                    OCTET STRING <value>   (null-terminated UTF-16-LE)
                }
            }
        }
    """
    inner = _seq(
        _bmp(name),
        _int(0x10010001),
        _wstr(value),
    )
    return _seq(_oid(_OID_NAME_VALUE), _octet(inner))


# ---------------------------------------------------------------------------
# CTL builder
# ---------------------------------------------------------------------------

def build_ctl(files: list[str], timestamp: datetime,
              hwid: str = '', os_val: str = '2:10.0') -> bytes:
    """
    Return the DER-encoded CTL binary.

    Each file in *files* produces two CTLEntry records (SHA-1 + SHA-256).

    *hwid*   — if provided (e.g. 'UEFI\\RES_{GUID}'), added to CTL extensions
               as HWID1; helps Windows match the catalog to the firmware device.
    *os_val* — OS compatibility attribute value, e.g. '2:10.0' (NT 10.0 / Win10).
    """
    # 16-byte random list identifier (required by CryptCATOpen)
    list_id = secrets.token_bytes(16)

    # Two entries per file
    entries = b''.join(
        _ctl_entry_sha1(f, os_val) + _ctl_entry_sha256(f, os_val)
        for f in files
    )

    # CTL extensions: OS string (required) + optional HWID
    ext_attrs = _name_value_ext('OS', '_v100_X64')
    if hwid:
        ext_attrs += _name_value_ext('HWID1', hwid)

    ctl_body = (
        _seq(_oid(_OID_CAT_LIST)) +                    # subjectUsage
        _octet(list_id) +                               # listIdentifier
        _utctime(timestamp) +                           # ctlThisUpdate
        _seq(_oid(_OID_CAT_MULTI_HASH), _null()) +     # subjectAlgorithm
        _tlv(0x30, entries) +                           # SEQUENCE OF CTLEntry
        _tlv(0xA0, _tlv(0x30, ext_attrs))              # [0] EXPLICIT { SEQUENCE { exts } }
    )
    return _tlv(0x30, ctl_body)


# ---------------------------------------------------------------------------
# Unsigned PKCS#7 wrapper
#
# CTL bytes are placed directly under the [0] EXPLICIT tag in encapContentInfo
# — without an OCTET STRING wrapper (PKCS#7 v1 legacy format).
# osslsigncode then adds the certificate and signerInfo.
# ---------------------------------------------------------------------------

def _unsigned_pkcs7(ctl: bytes) -> bytes:
    encap = _seq(
        _oid(_OID_CTL),
        _tlv(0xA0, ctl),      # [0] EXPLICIT { CTL bytes, no OCTET STRING }
    )
    signed_data = _seq(
        _tlv(0x02, b'\x01'),   # version INTEGER 1
        _tlv(0x31, b''),        # digestAlgorithms SET {} (empty)
        encap,
        _tlv(0x31, b''),        # signerInfos SET {} (empty)
    )
    return _seq(
        _oid(_OID_SIGNED_DATA),
        _tlv(0xA0, signed_data),
    )


# ---------------------------------------------------------------------------
# Sign via osslsigncode
# ---------------------------------------------------------------------------

def sign(ctl: bytes, cert: str, key: str, out: str) -> None:
    """Sign the CTL with osslsigncode and write the signed .cat file."""
    unsigned = _unsigned_pkcs7(ctl)

    with tempfile.NamedTemporaryFile(suffix='.cat', delete=False) as tmp:
        tmp.write(unsigned)
        unsigned_path = tmp.name

    try:
        try:
            result = subprocess.run(
                [
                    'osslsigncode', 'sign',
                    '-certs', cert,
                    '-key',   key,
                    '-h',     'sha256',
                    '-in',    unsigned_path,
                    '-out',   out,
                ],
                capture_output=True,
                text=True,
            )
        except FileNotFoundError:
            sys.exit(
                'osslsigncode not found — install with:\n'
                '  dnf install osslsigncode   (Fedora/RHEL)\n'
                '  apt install osslsigncode   (Debian/Ubuntu)'
            )
        if result.returncode != 0:
            sys.exit(
                f'osslsigncode failed (exit {result.returncode}):\n'
                f'{result.stderr.rstrip()}'
            )
    finally:
        os.unlink(unsigned_path)


# ---------------------------------------------------------------------------
# Certificate generation
# ---------------------------------------------------------------------------

def gen_cert(cert_out: str, key_out: str, subject: str) -> None:
    result = subprocess.run(
        [
            'openssl', 'req', '-x509', '-newkey', 'rsa:2048',
            '-keyout', key_out,
            '-out',    cert_out,
            '-days',   '3650',
            '-nodes',
            '-subj',   subject,
            '-addext', 'basicConstraints=critical,CA:TRUE',
            '-addext', 'keyUsage=critical,digitalSignature,keyCertSign,cRLSign',
            '-addext', 'extendedKeyUsage=codeSigning',
        ],
        capture_output=True,
        text=True,
    )
    if result.returncode != 0:
        sys.exit(f'openssl error:\n{result.stderr.rstrip()}')


# ---------------------------------------------------------------------------
# Windows Cabinet (.cab) reader/writer
# ---------------------------------------------------------------------------

_CAB_SIG         = b'MSCF'
_COMPRESS_STORE  = 0
_COMPRESS_MSZIP  = 1

_FMT_HEADER  = '<4sIIIIIBBHHHHH'
_FMT_FOLDER  = '<IHH'
_FMT_FILE    = '<IIHHHH'
_FMT_DATA    = '<IHH'


def _is_cabinet(path: str) -> bool:
    try:
        with open(path, 'rb') as f:
            return f.read(4) == _CAB_SIG
    except OSError:
        return False


def _mszip_decompress(blocks: list[bytes]) -> bytes:
    output = b''
    for block in blocks:
        if len(block) < 2 or block[:2] != b'CK':
            raise ValueError('Invalid MSZIP block: missing CK signature')
        compressed = block[2:]
        if not output:
            output += zlib.decompress(compressed, -15)
        else:
            history = output[-32768:]
            n = len(history)
            stored = bytes([0x00]) + struct.pack('<HH', n, n ^ 0xFFFF) + history
            d = zlib.decompressobj(-15)
            d.decompress(stored)
            output += d.decompress(compressed)
    return output


def read_cabinet(path: str) -> dict[str, bytes]:
    with open(path, 'rb') as f:
        data = f.read()

    hdr_sz = struct.calcsize(_FMT_HEADER)
    (sig, _, _, _, coff_first_file, _, ver_min, ver_maj,
     n_folders, n_files, flags, _, _) = struct.unpack_from(_FMT_HEADER, data)

    if sig != _CAB_SIG:
        raise ValueError(f'Not a cabinet file (magic: {sig!r})')

    cb_hdr_res = cb_fold_res = cb_data_res = 0
    off = hdr_sz
    if flags & 0x0004:
        cb_hdr_res, cb_fold_res, cb_data_res = struct.unpack_from('<HBB', data, off)
        off += 4 + cb_hdr_res

    folders: list[dict] = []
    for _ in range(n_folders):
        coff_start, n_data, compress = struct.unpack_from(_FMT_FOLDER, data, off)
        off += struct.calcsize(_FMT_FOLDER) + cb_fold_res
        compress_type = compress & 0x0F
        if compress_type not in (_COMPRESS_STORE, _COMPRESS_MSZIP):
            raise ValueError(f'Unsupported cabinet compression type: {compress_type}')
        folders.append({'coff': coff_start, 'n_data': n_data,
                        'compress': compress_type})

    files: list[dict] = []
    off = coff_first_file
    for _ in range(n_files):
        cb_file, uoff, i_folder, date, time, attribs = struct.unpack_from(
            _FMT_FILE, data, off)
        off += struct.calcsize(_FMT_FILE)
        end = data.index(b'\x00', off)
        name = data[off:end].decode('ascii', errors='replace').replace('\\', '/')
        off = end + 1
        files.append({'size': cb_file, 'folder_off': uoff,
                      'folder_idx': i_folder, 'name': os.path.basename(name)})

    folder_data: list[bytes] = []
    for folder in folders:
        off = folder['coff']
        blocks: list[bytes] = []
        for _ in range(folder['n_data']):
            _, cb_data, _ = struct.unpack_from(_FMT_DATA, data, off)
            off += struct.calcsize(_FMT_DATA) + cb_data_res
            blocks.append(data[off: off + cb_data])
            off += cb_data

        if folder['compress'] == _COMPRESS_STORE:
            folder_data.append(b''.join(blocks))
        else:
            folder_data.append(_mszip_decompress(blocks))

    result: dict[str, bytes] = {}
    for f in files:
        idx = f['folder_idx']
        if idx >= 0xFFFD:
            continue
        raw = folder_data[idx]
        result[f['name']] = raw[f['folder_off']: f['folder_off'] + f['size']]

    return result


def write_cabinet(path: str, files: dict[str, bytes],
                  timestamp: datetime | None = None) -> None:
    if timestamp is None:
        timestamp = datetime.now(timezone.utc)

    y, mo, d = timestamp.year - 1980, timestamp.month, timestamp.day
    h, mi, s = timestamp.hour, timestamp.minute, timestamp.second
    dos_date = (y << 9) | (mo << 5) | d
    dos_time = (h << 11) | (mi << 5) | (s >> 1)

    BLOCK = 32768

    items: list[tuple[str, bytes, int]] = []
    folder_off = 0
    for name, data in files.items():
        items.append((name, data, folder_off))
        folder_off += len(data)

    coff_first_file = 36 + 8
    file_entries_size = sum(struct.calcsize(_FMT_FILE) + len(n.encode('ascii')) + 1
                            for n, _, _ in items)
    coff_cab_start = coff_first_file + file_entries_size

    all_data = b''.join(d for _, d, _ in items)
    blocks = [all_data[i: i + BLOCK] for i in range(0, max(len(all_data), 1), BLOCK)]
    data_size = sum(struct.calcsize(_FMT_DATA) + len(b) for b in blocks)
    cb_cabinet = coff_cab_start + data_size

    cab = bytearray()
    cab += struct.pack(_FMT_HEADER,
                       _CAB_SIG, 0, cb_cabinet, 0,
                       coff_first_file, 0,
                       3, 1, 1, len(items), 0, 0, 0)
    cab += struct.pack(_FMT_FOLDER, coff_cab_start, len(blocks), _COMPRESS_STORE)

    for name, data, foff in items:
        cab += struct.pack(_FMT_FILE, len(data), foff, 0, dos_date, dos_time, 0x20)
        cab += name.encode('ascii') + b'\x00'

    for block in blocks:
        cab += struct.pack(_FMT_DATA, 0, len(block), len(block))
        cab += block

    dir_ = os.path.dirname(os.path.abspath(path))
    fd, tmp = tempfile.mkstemp(dir=dir_)
    try:
        os.write(fd, bytes(cab))
        os.close(fd)
        os.replace(tmp, path)
    except Exception:
        os.close(fd)
        os.unlink(tmp)
        raise


# ---------------------------------------------------------------------------
# Entry point
# ---------------------------------------------------------------------------

def main() -> None:
    ap = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )
    ap.add_argument('--cert', default='firmware-signing.crt', metavar='cert.pem')
    ap.add_argument('--key',  default='firmware-signing.key', metavar='key.pem')
    ap.add_argument('--gen-cert', action='store_true',
                    help='Generate a self-signed code-signing certificate')
    ap.add_argument('--subject', default='/CN=Firmware Test Signing',
                    metavar='/CN=...')
    ap.add_argument('--hwid', default='', metavar='UEFI\\RES_{GUID}',
                    help='Hardware ID to embed in CTL extensions (e.g. '
                         '"UEFI\\\\RES_{12345678-...}"); helps Windows match '
                         'the catalog to the firmware device')
    ap.add_argument('--out', default=None, metavar='firmware.cat')
    ap.add_argument('--update-cabinet', action='store_true',
                    help='Rewrite the input cabinet with the .cat added')
    ap.add_argument('files', nargs='+',
                    help='Files to catalog, or a single .cab to read from')
    args = ap.parse_args()

    if args.gen_cert:
        print(f'Generating self-signed certificate → {args.cert}, {args.key}',
              file=sys.stderr)
        gen_cert(args.cert, args.key, args.subject)
        print('WARNING: self-signed certificate is for test-signing only.',
              file=sys.stderr)
    else:
        missing = [f for f in [args.cert, args.key] if not os.path.isfile(f)]
        if missing:
            sys.exit('error: file(s) not found: ' + ', '.join(missing))

    files = args.files
    cab_tmpdir: str | None = None
    cab_contents: dict[str, bytes] | None = None
    cab_path: str | None = None

    if len(files) == 1 and _is_cabinet(files[0]):
        cab_path = files[0]
        print(f'Reading cabinet: {cab_path}', file=sys.stderr)
        try:
            cab_contents = read_cabinet(cab_path)
        except (ValueError, struct.error) as e:
            sys.exit(f'error reading cabinet: {e}')

        cab_tmpdir = tempfile.mkdtemp(prefix='capsule_catalog_')
        for name, data in cab_contents.items():
            dest = os.path.join(cab_tmpdir, name)
            with open(dest, 'wb') as fh:
                fh.write(data)
        files = sorted(os.path.join(cab_tmpdir, n) for n in cab_contents)
        print(f'  Contents: {[os.path.basename(f) for f in files]}',
              file=sys.stderr)

        if args.out is None:
            args.out = cab_path + '.cat'
    else:
        if args.update_cabinet:
            sys.exit('error: --update-cabinet requires a single cabinet file as input')
        missing_files = [f for f in files if not os.path.isfile(f)]
        if missing_files:
            sys.exit('error: file(s) not found: ' + ', '.join(missing_files))

    if args.out is None:
        sys.exit('error: --out is required when not passing a cabinet')

    try:
        ts = datetime.now(timezone.utc).replace(second=0, microsecond=0)

        print(f'Building CTL for {len(files)} file(s)...', file=sys.stderr)
        ctl = build_ctl(files, ts, hwid=args.hwid)

        print(f'Signing → {args.out}', file=sys.stderr)
        sign(ctl, args.cert, args.key, args.out)

        if args.update_cabinet and cab_contents is not None and cab_path is not None:
            with open(args.out, 'rb') as fh:
                cat_data = fh.read()
            cat_name = os.path.basename(args.out)
            updated = dict(cab_contents)
            updated[cat_name] = cat_data
            print(f'Updating cabinet: {cab_path} (adding {cat_name})',
                  file=sys.stderr)
            write_cabinet(cab_path, updated, ts)
            print(f'Updated: {cab_path}')
    finally:
        if cab_tmpdir:
            import shutil
            shutil.rmtree(cab_tmpdir, ignore_errors=True)

    print(f'Created: {args.out}')


if __name__ == '__main__':
    main()
