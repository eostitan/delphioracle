import struct
import hashlib
import binascii

from datetime import datetime, timedelta
from cryptos import ecdsa_raw_sign, hash_to_int, encode_privkey, decode, encode, \
    hmac, fast_multiply, G, inv, N, decode_privkey, get_privkey_format, random_key, encode_pubkey, privtopub

from base58 import b58decode, b58encode

from .ds import DataStream

DATE_TIME_FORMAT = '%Y-%m-%dT%H:%M:%S'

C = int((2 ** 256 - 1) / 2)

def is_canonical(r, s):
    return r <= C and s <= C

def endian_reverse_u32(x):
    x = x & 0xFFFFFFFF
    return (((x >> 0x18) & 0xFF)) \
           | (((x >> 0x10) & 0xFF) << 0x08) \
           | (((x >> 0x08) & 0xFF) << 0x10) \
           | (((x) & 0xFF) << 0x18)

def get_tapos_info(block_id):
    block_id_bin = bytes.fromhex(block_id)

    hash0 = struct.unpack("<Q", block_id_bin[0:8])[0]
    hash1 = struct.unpack("<Q", block_id_bin[8:16])[0]

    ref_block_num = endian_reverse_u32(hash0) & 0xFFFF
    ref_block_prefix = hash1 & 0xFFFFFFFF

    return ref_block_num, ref_block_prefix

def deterministic_generate_k_nonce(msghash, priv, nonce):
    v = b'\x01' * 32
    k = b'\x00' * 32
    priv = encode_privkey(priv, 'bin')
    msghash = encode(hash_to_int(msghash) + nonce, 256, 32)
    k = hmac.new(k, v + b'\x00' + priv + msghash, hashlib.sha256).digest()
    v = hmac.new(k, v, hashlib.sha256).digest()
    k = hmac.new(k, v + b'\x01' + priv + msghash, hashlib.sha256).digest()
    v = hmac.new(k, v, hashlib.sha256).digest()
    return decode(hmac.new(k, v, hashlib.sha256).digest(), 256)

def ecdsa_raw_sign_nonce(msghash, priv, nonce):
    z = hash_to_int(msghash)
    k = deterministic_generate_k_nonce(msghash, priv, nonce)

    r, y = fast_multiply(G, k)
    s = inv(k, N) * (z + r * decode_privkey(priv)) % N

    v, r, s = 27 + ((y % 2) ^ (0 if s * 2 < N else 1)), r, s if s * 2 < N else N - s
    if 'compressed' in get_privkey_format(priv):
        v += 4
    return v, r, s

def sign_hash(h, pk):
    nonce = 0
    while True:
        v, r, s = ecdsa_raw_sign_nonce(h, pk, nonce)
        if is_canonical(r, s):
            signature = '00%02x%064x%064x' % (v, r, s)
            break
        nonce += 1
    sig = DataStream(bytes.fromhex(signature)).unpack_signature()
    return sig

def sign_bytes(ds, pk):
    nonce = 0
    m = hashlib.sha256()
    m.update(ds.getvalue())
    return sign_hash(m.digest(), pk)

def sign_tx(chain_id, tx, pk):
    zeros = '0000000000000000000000000000000000000000000000000000000000000000'

    ds = DataStream()
    ds.pack_checksum256(chain_id)

    dsp = DataStream()
    dsp.pack_transaction(tx)
    packed_trx = dsp.getvalue()

    ds.write(packed_trx)
    ds.pack_checksum256(zeros)

    sig = sign_bytes(ds, pk)
    if 'signatures' not in tx:
        tx['signatures'] = []
    tx['signatures'].append(sig)

    m = hashlib.sha256()
    m.update(packed_trx)
    tx_id = binascii.hexlify(m.digest()).decode('utf-8')

    return tx_id, tx

def ripmed160(data):
    h = hashlib.new('ripemd160')
    h.update(data)
    return h.digest()

def gen_key_pair():
    pk   = random_key()
    wif  = encode_privkey(pk, 'wif')
    data = encode_pubkey(privtopub(pk),'bin_compressed')
    data = data + ripmed160(data)[:4]
    pubkey = "EOS" + b58encode(data).decode('utf8')
    return wif, pubkey

def get_pub_key(pk):
    data = encode_pubkey(privtopub(pk),'bin_compressed')
    data = data + ripmed160(data)[:4]
    pubkey = "EOS" + b58encode(data).decode('utf8')
    return pubkey

def build_action(action, auth, data):
    return {
        "account": action['account'],
        "name": action['name'],
        "authorization": [{
            "actor": auth['actor'],
            "permission": auth['permission']
        }],
        "data": binascii.hexlify(data).decode('utf-8')
    }

def build_transaction(expiration, ref_block_num, ref_block_prefix, actions):
    if type(actions) == dict: actions = [actions]
    return {
        "expiration": expiration,
        "ref_block_num": ref_block_num,
        "ref_block_prefix": ref_block_prefix,
        "max_net_usage_words": 0,
        "max_cpu_usage_ms": 0,
        "delay_sec": 0,
        "context_free_actions": [],
        "actions": actions,
        "transaction_extensions": [],
        "signatures": [],
        "context_free_data": []
    }

def build_push_transaction_body(signature, packed_trx):
    return {
        "signatures": [
            signature
        ],
        "compression": False,
        "packed_context_free_data": "",
        "packed_trx": packed_trx
    }

def get_expiration(now, delta=0):
    return (now + timedelta(seconds=delta)).strftime(DATE_TIME_FORMAT)

def create_tx(chain_id, block_id, expiration, privkey, fn_action_builder, params):
    ref_block_num, ref_block_prefix = get_tapos_info(block_id)

    u_tx = build_transaction(expiration, ref_block_num, ref_block_prefix, fn_action_builder(*params) )

    tx_id, signed_tx = sign_tx(chain_id, u_tx, privkey)
    return tx_id, signed_tx
