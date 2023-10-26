import io
import struct
import time
import hashlib
import binascii
import calendar
from datetime import datetime
from base58 import b58decode, b58encode
from collections import OrderedDict


def ripmed160(data):
    h = hashlib.new('ripemd160')
    h.update(data)
    return h.digest()


def char_to_symbol(c):
    if c >= ord('a') and c <= ord('z'):
        return (c - ord('a')) + 6
    if c >= ord('1') and c <= ord('5'):
        return (c - ord('1')) + 1
    return 0


def string_to_name(s):
    if len(s) > 13: raise Exception("invalid string length")
    name = 0
    i = 0
    while i < min(len(s), 12):
        name |= (char_to_symbol(ord(s[i])) & 0x1f) << (64 - 5 * (i + 1))
        i += 1
    if len(s) == 13:
        name |= char_to_symbol(ord(s[12])) & 0x0F
    return name


def name_to_string(n, strip_dots=True):
    charmap = ".12345abcdefghijklmnopqrstuvwxyz"
    s = bytearray(13 * b'.')
    tmp = n
    for i in range(13):
        c = charmap[tmp & (0x0f if i == 0 else 0x1f)]
        s[12 - i] = ord(c)
        tmp >>= (4 if i == 0 else 5)

    s = s.decode('utf8')
    if strip_dots:
        s = s.strip('.')
    return s


def string_to_symbol(precision, s):
    l = len(s)
    if l > 7: raise Exception("invalid symbol {0}".format(s))
    result = 0
    for i in range(l):
        if ord(s[i]) < ord('A') or ord(s[i]) > ord('Z'):
            raise Exception("invalid symbol {0}".format(s))
        else:
            result |= (int(ord(s[i])) << (8 * (1 + i)))

    result |= int(precision)
    return result

def symbol_code_to_uint64(sc):
    l = len(sc)
    if l > 7: raise Exception("invalid symbol code {0}".format(sc))
    result = 0
    for i in range(l):
        if ord(sc[i]) < ord('A') or ord(sc[i]) > ord('Z'):
            raise Exception("invalid symbol code {0}".format(sc))
        else:
            result |= (int(ord(sc[i])) << (8 * (i)))
    return result

def uint64_to_symbol_code(n):
    mask = 0xFF
    v = n
    res = ''
    for i in range(7):
        if v == 0: break
        res += chr(v & mask)
        v >>= 8
    return res

def symbol_to_string(symbol):
    return symbol

def asset_to_uint64_pair(a):
    parts = a.split()
    if len(parts) != 2: raise Exception("invalid asset {0}".format(a))

    nums = parts[0].split(".")
    num = ''.join(nums)
    return int(num), string_to_symbol(len(nums[1]) if len(nums) > 1 else 0, parts[1])


class DataStream():

    def __init__(self, stream=None):
        if not stream:
            stream = io.BytesIO()
        else:
            stream = io.BytesIO(stream)
        self.stream = stream

    def write(self, v):
        self.stream.write(v)

    def read(self, n):
        return self.stream.read(n)

    def getvalue(self):
        return bytearray(self.stream.getvalue())

    def pack_array(self, type, values):
        self.pack_varuint32(len(values))
        for v in values:
            getattr(self, 'pack_' + type)(v)

    def unpack_array(self, type):
        l = self.unpack_varuint32()
        res = []
        for i in range(l):
            res.append(getattr(self, 'unpack_' + type)())
        return res

    def pack_extension(self, v):
        self.pack_uint16(v[0])
        self.pack_bytes(v[1])

    def unpack_extension(self):
        return [
            self.unpack_uint16(),
            self.unpack_bytes()
        ]

    def pack_bool(self, v):
        self.write(b'\x01') if v else self.write(b'\x00')

    def unpack_bool(self):
        return True if self.read(1) else False

    def pack_int8(self, v):
        self.write(struct.pack("<b", v))

    def unpack_int8(self):
        return struct.unpack("<b", self.read(1))[0]

    def pack_uint8(self, v):
        self.write(struct.pack("<B", v))

    def unpack_uint8(self):
        return struct.unpack("<B", self.read(1))[0]

    def pack_int16(self, v):
        self.write(struct.pack("<h", v))

    def unpack_int16(self):
        return struct.unpack("<h", self.read(2))[0]

    def pack_uint16(self, v):
        self.write(struct.pack("<H", v))

    def unpack_uint16(self):
        return struct.unpack("<H", self.read(2))[0]

    def pack_int32(self, v):
        self.write(struct.pack("<i", v))

    def unpack_int32(self):
        return struct.unpack("<i", self.read(4))[0]

    def pack_uint32(self, v):
        self.write(struct.pack("<I", v))

    def unpack_uint32(self):
        return struct.unpack("<I", self.read(4))[0]

    def pack_int64(self, v):
        self.write(struct.pack("<q", v))

    def unpack_int64(self):
        return struct.unpack("<q", self.read(8))[0]

    def pack_uint64(self, v):
        self.write(struct.pack("<Q", v))

    def unpack_uint64(self):
        return struct.unpack("<Q", self.read(8))[0]

    def pack_int128(self, v):
        raise Exception("not implementd")

    def unpack_int128(self):
        raise Exception("not implementd")

    def pack_uint128(self, v):
        raise Exception("not implementd")

    def unpack_uint128(self):
        raise Exception("not implementd")

    def pack_varint32(self, v):
        raise Exception("not implementd")

    def unpack_varint32(self):
        raise Exception("not implementd")

    def pack_varuint32(self, v):
        val = v
        while True:
            b = val & 0x7f
            val >>= 7
            b |= ((val > 0) << 7)
            self.pack_uint8(b)
            if not val:
                break

    def unpack_varuint32(self):
        v = 0;
        b = 0;
        by = 0
        while True:
            b = self.unpack_uint8()
            v |= ((b & 0x7f) << by)
            by += 7
            if not (b & 0x80 and by < 32):
                break
        return v

    def pack_float32(self, v):
        raise Exception("not implementd")

    def unpack_float32(self):
        raise Exception("not implementd")

    def pack_float64(self, v):
        raise Exception("not implementd")

    def unpack_float64(self):
        raise Exception("not implementd")

    def pack_float128(self, v):
        raise Exception("not implementd")

    def unpack_float128(self):
        raise Exception("not implementd")

    def pack_time_point(self, v):
        raise Exception("not implementd")

    def unpack_time_point(self):
        raise Exception("not implementd")

    def pack_time_point_sec(self, v):
        t = int(calendar.timegm(datetime.strptime(v, '%Y-%m-%dT%H:%M:%S').timetuple()))
        self.pack_uint32(t)

    def unpack_time_point_sec(self):
        t = self.unpack_uint32()
        return datetime.fromtimestamp(t).strftime('%Y-%m-%dT%H:%M:%S')

    def pack_block_timestamp_type(self, v):
        raise Exception("not implementd")

    def unpack_block_timestamp_type(self):
        raise Exception("not implementd")

    def pack_account_name(self, v):
        self.pack_uint64(string_to_name(v))

    def unpack_account_name(self):
        return name_to_string(self.unpack_uint64())

    def pack_permission_level(self, v):
        self.pack_account_name(v["actor"])
        self.pack_account_name(v["permission"])

    def unpack_permission_level(self):
        return OrderedDict([
            ("actor", self.unpack_account_name()),
            ("permission", self.unpack_account_name())
        ])

    def pack_permission_level_weight(self, v):
        self.pack_permission_level(v["permission"])
        self.pack_int16(v["weight"])

    def unpack_permission_level_weight(self):
        return OrderedDict([
            ("permission", self.unpack_permission_level()),
            ("weight", self.unpack_int16())
        ])

    def pack_key_weight(self, v):
        self.pack_public_key(v["key"])
        self.pack_int16(v["weight"])

    def unpack_key_weight(self):
        return OrderedDict([
            ("key", self.unpack_public_key()),
            ("weight", self.unpack_int16())
        ])

    def pack_wait_weight(self, v):
        self.pack_uint32(v["wait_sec"])
        self.pack_int16(v["weight"])

    def unpack_wait_weight(self):
        return OrderedDict([
            ("wait_sec", self.unpack_uint32()),
            ("weight", self.unpack_int16())
        ])

    def pack_authority(self, v):
        self.pack_uint32(v["threshold"])
        self.pack_array('key_weight', v['keys'])
        self.pack_array('permission_level_weight', v['accounts'])
        self.pack_array('wait_weight', v['waits'])

    def unpack_authority(self):
        return OrderedDict([
            ("threshold", self.unpack_uint32()),
            ("keys", self.unpack_array('key_weight')),
            ("accounts", self.unpack_array('permission_level_weight')),
            ("waits", self.unpack_array('wait_weight'))
        ])

    def pack_action(self, v):
        self.pack_account_name(v["account"])
        self.pack_account_name(v["name"])
        self.pack_array("permission_level", v["authorization"])
        self.pack_bytes(bytes.fromhex(v["data"]))

    def unpack_action(self):
        return OrderedDict([
            ("account", self.unpack_account_name()),
            ("name", self.unpack_account_name()),
            ("authorization", self.unpack_array("permission_level")),
            ("data", self.unpack_bytes())
        ])

    def pack_transaction(self, v):
        self.pack_time_point_sec(v["expiration"])
        self.pack_uint16(v["ref_block_num"])
        self.pack_uint32(v["ref_block_prefix"])
        self.pack_varuint32(v["max_net_usage_words"])
        self.pack_uint8(v["max_cpu_usage_ms"])
        self.pack_varuint32(v["delay_sec"])
        self.pack_array("action", v["context_free_actions"])
        self.pack_array("action", v["actions"])
        self.pack_array("extension", v["transaction_extensions"])

    def pack_signed_transaction(self, v):
        self.pack_transaction(v)
        self.pack_array("signature", v["signatures"])
        self.pack_array("bytes", v["context_free_data"])

    def unpack_transaction(self):
        return OrderedDict([
            ("expiration", self.unpack_time_point_sec()),
            ("ref_block_num", self.unpack_uint16()),
            ("ref_block_prefix", self.unpack_uint32()),
            ("max_net_usage_words", self.unpack_varuint32()),
            ("max_cpu_usage_ms", self.unpack_uint8()),
            ("delay_sec", self.unpack_varuint32()),
            ("context_free_actions", self.unpack_array("action")),
            ("action", self.unpack_array("action")),
            ("transaction_extensions", self.unpack_array("extension")),
        ])

    def unpack_signed_transaction(self):
        r = self.unpack_transaction(self)
        r.update(OrderedDict([
            ("signatures", self.unpack_signature()),
            ("context_free_data", self.unpack_array("bytes"))
        ]))
        return r

    def pack_name(self, v):
        self.pack_account_name(v)

    def unpack_name(self):
        return self.unpack_account_name()

    def pack_bytes(self, v):
        if type(v) == int or type(v) == float:
            v = str(v)
        self.pack_varuint32(len(v) if v else 0)
        if type(v) == str: v = v.encode()
        if v: self.write(v)

    def unpack_bytes(self):
        l = self.unpack_varuint32()
        return self.read(l)

    def pack_string(self, v):
        if type(v) == bytearray:
            v = v.decode("utf-8")
        self.pack_bytes(v)

    def unpack_string(self):
        return self.unpack_bytes().decode('utf8')

    def pack_checksum160(self, v):
        raise Exception("not implementd")

    def unpack_checksum160(self):
        raise Exception("not implementd")

    def pack_checksum256(self, v):
        self.pack_sha256(v)

    def unpack_checksum256(self):
        return self.unpack_sha256()

    def pack_checksum512(self, v):
        raise Exception("not implementd")

    def unpack_checksum512(self):
        raise Exception("not implementd")

    def pack_public_key(self, v):
        if v.startswith("EOS"):
            data = b58decode(str(v[3:]))
            if len(data) != 33 + 4: raise Exception("invalid k1 key")
            if ripmed160(data[:-4])[:4] != data[-4:]: raise Exception("checksum failed")
            self.pack_uint8(0)
            self.write(data[:-4])
        elif v.startswith("PUB_R1_"):
            raise Exception("not implementd")
        else:
            raise Exception("invalid pubkey format")

    def unpack_public_key(self):
        t = self.unpack_uint8()
        if t == 0:
            data = self.read(33)
            data = data + ripmed160(data)[:4]
            return "EOS" + b58encode(data).decode('ascii')
        elif t == 1:
            raise Exception("not implementd")
        else:
            raise Exception("invalid binary pubkey")

    def pack_signature(self, v):

        def pack_signature_sufix(b58sig, sufix):
            data = b58decode(b58sig)
            if len(data) != 65 + 4: raise Exception("invalid {0} signature".format(sufix))
            if ripmed160(data[:-4] + sufix)[:4] != data[-4:]: raise Exception("checksum failed")
            self.pack_uint8(0)
            self.write(data[:-4])

        if v.startswith("SIG_K1_"):
            pack_signature_sufix(str(v[7:]), b"K1")
        elif v.startswith("SIG_R1_"):
            pack_signature_sufix(str(v[7:]), b"R1")
        else:
            raise Exception("invalid signature format")

    def unpack_signature(self):
        t = self.unpack_uint8()
        if t == 0:
            data = self.read(65)
            data = data + ripmed160(data + b"K1")[:4]
            return "SIG_K1_" + b58encode(data).decode("ascii")
        elif t == 1:
            raise Exception("not implementd")
        else:
            raise Exception("invalid binary signature")

    def pack_type_def(self, v):
        self.pack_string(v["new_type_name"])
        self.pack_string(v["type"])

    def unpack_type_def(self):
        return OrderedDict([
            ("new_type_name", self.unpack_string()),
            ("type", self.unpack_string())
        ])

    def pack_optional(self, type, v):
        if v is None:
            self.pack_uint8(0)
        else:
            self.pack_uint8(1)
            getattr(self, 'pack_' + type)(v)

    def unpack_optional(self, type):
        if not self.unpack_uint8():
            return None
        return getattr(self, 'unpack_' + type)()

    def pack_field_def(self, v):
        self.pack_string(v["name"])
        self.pack_string(v["type"])

    def unpack_field_def(self):
        return OrderedDict([
            ("name", self.unpack_string()),
            ("type", self.unpack_string())
        ])

    def pack_struct_def(self, v):
        self.pack_string(v["name"])
        self.pack_string(v["base"])
        self.pack_array("field_def", v["fields"])

    def unpack_struct_def(self):
        return OrderedDict([
            ("name", self.unpack_string()),
            ("base", self.unpack_string()),
            ("fields", self.unpack_array("field_def"))
        ])

    def pack_action_def(self, v):
        self.pack_account_name(v["name"])
        self.pack_string(v["type"])
        self.pack_string(v["ricardian_contract"])

    def unpack_action_def(self):
        return OrderedDict([
            ("name", self.unpack_account_name()),
            ("type", self.unpack_string()),
            ("ricardian_contract", self.unpack_string())
        ])

    def pack_table_def(self, v):
        self.pack_account_name(v["name"])
        self.pack_string(v["index_type"])
        self.pack_array("string", v["key_names"])
        self.pack_array("string", v["key_types"])
        self.pack_string(v["type"])

    def unpack_table_def(self):
        return OrderedDict([
            ("name", self.unpack_account_name()),
            ("index_type", self.unpack_string()),
            ("key_names", self.unpack_array("string")),
            ("key_types", self.unpack_array("string")),
            ("type", self.unpack_string())
        ])

    def pack_clause_pair(self, v):
        self.pack_string(v["id"])
        self.pack_string(v["body"])

    def unpack_clause_pair(self):
        return OrderedDict([
            ("id", self.unpack_string()),
            ("body", self.unpack_string()),
        ])

    def pack_error_message(self, v):
        self.pack_uint64(v["error_code"])
        self.pack_string(v["error_msg"])

    def unpack_error_message(self):
        return OrderedDict([
            ("error_code", self.unpack_uin64()),
            ("error_msg", self.unpack_string()),
        ])

    def pack_variant(self, v):
        self.pack_string(v["name"])
        self.pack_array("string", v["types"])

    def unpack_variant(self):
        return OrderedDict([
            ("name", self.unpack_string()),
            ("types", self.unpack_array("string")),
        ])

    def pack_abi(self, v):
        self.pack_string(v["version"])
        self.pack_array("type_def", v.get("types",[]))
        self.pack_array("struct_def", v.get("structs",[]))
        self.pack_array("action_def", v.get("actions",[]))
        self.pack_array("table_def", v.get("tables",[]))
        self.pack_array("clause_pair", v.get("ricardian_clauses",[]))
        self.pack_array("error_message", v.get("error_messages",[]))
        self.pack_array("extension", v.get("abi_extensions",[]))
        self.pack_array("variant", v.get("variants",[]))

    def unpack_abi(self):
        tmp = OrderedDict([
            ("version", self.unpack_string()),
            ("types", self.unpack_array("type_def")),
            ("structs", self.unpack_array("struct_def")),
            ("actions", self.unpack_array("action_def")),
            ("tables", self.unpack_array("table_def")),
            ("ricardian_clauses", self.unpack_array("clause_pair")),
            ("error_messages", self.unpack_array("error_message")),
            ("abi_extensions", self.unpack_array("extension"))
        ])

        try:
            tmp["variants"] = self.unpack_array("variant")
        except:
            pass
        return tmp

    def pack_symbol(self, v):
        raise Exception("not implementd")

    def unpack_symbol(self):
        v = self.unpack_uint64()
        precision = v & 0xFF
        v >>= 8
        return f"{precision},{uint64_to_symbol_code(v)}"

    def pack_symbol_code(self, v):
        self.pack_uint64(symbol_code_to_uint64(v))

    def unpack_symbol_code(self):
        return uint64_to_symbol_code(self.unpack_uint64())

    def pack_asset(self, v):
        a, s = asset_to_uint64_pair(v)
        self.pack_uint64(a)
        self.pack_uint64(s)

    def unpack_asset(self):
        amount = self.unpack_int64()
        symbol = self.unpack_symbol()
        return f"{amount},{symbol}"

    def pack_extended_asset(self, v):
        raise Exception("not implementd")

    def unpack_extended_asset(self):
        return OrderedDict([
            ("quantity", self.unpack_asset()),
            ("contract", self.unpack_name()),
        ])

    def pack_sha256(self, v):
        if len(v) != 64: raise Exception("Invalid sha256 length")
        self.write(bytes.fromhex(v))

    def unpack_sha256(self):
        d = self.read(32)
        return binascii.hexlify(d).decode('utf-8')

    def pack_chain_id_type(self, v):
        self.pack_sha256(v)

    def unpack_chain_id_type(self):
        self.unpack_sha256()

    def pack_block_id_type(self, v):
        self.pack_sha256(v)

    def unpack_block_id_type(self):
        self.unpack_sha256()

    def pack_tstamp(self, v):
        self.pack_uint64(v)

    def unpack_tstamp(self):
        self.unpack_uint64()

    def pack_handshake_message(self, v):
        self.pack_uint16(v.network_version)
        self.pack_chain_id_type(v.chain_id)
        self.pack_sha256(v.node_id)
        self.pack_public_key(v.key)
        self.pack_tstamp(v.time)
        self.pack_sha256(v.token)
        self.pack_signature(v.sig)
        self.pack_string(v.p2p_address)
        self.pack_uint32(v.last_irreversible_block_num)
        self.pack_block_id_type(v.last_irreversible_block_id)
        self.pack_uint32(v.head_num)
        self.pack_block_id_type(v.head_id)
        self.pack_string(v.os)
        self.pack_string(v.agent)
        self.pack_int16(v.generation)

    def unpack_handshake_message(self):
        return [
            self.unpack_uint16(),
            self.unpack_chain_id_type(),
            self.unpack_sha256(),
            self.unpack_public_key(),
            self.unpack_tstamp(),
            self.unpack_sha256(),
            self.unpack_signature(),
            self.unpack_string(),
            self.unpack_uint32(),
            self.unpack_block_id_type(),
            self.unpack_uint32(),
            self.unpack_block_id_type(),
            self.unpack_string(),
            self.unpack_string(),
            self.unpack_int16()
        ]

    def unpack_partial_transaction(self):
        return OrderedDict([
            ("version", self.unpack_varuint32()),
            ("expiration", self.unpack_time_point_sec()),
            ("ref_block_num", self.unpack_uint16()),
            ("ref_block_prefix", self.unpack_uint32()),
            ("max_net_usage_words", self.unpack_varuint32()),
            ("max_cpu_usage_ms", self.unpack_uint8()),
            ("delay_sec", self.unpack_varuint32()),
            ("transaction_extensions", self.unpack_array('extension')),
            ("signatures", self.unpack_array('signature')),
            ("context_free_data", self.unpack_bytes()),
        ])

    def unpack_augmented_transaction_trace(self):
        return OrderedDict([
            ("version", self.unpack_varuint32()),
            ("trace_id", self.unpack_checksum256()),
            ("status", self.unpack_uint8()),
            ("cpu_usage_us", self.unpack_uint32()),
            ("net_usage_words", self.unpack_varuint32()),
            ("trace_elapsed", self.unpack_int64()),
            ("net_usage", self.unpack_uint64()),
            ("scheduled", self.unpack_uint8()),
            ("action_traces", self.unpack_array("action_trace")),
            ("account_delta", self.unpack_optional('account_delta')),
            ("trace_error", self.unpack_optional('string')),
            ("trace_ec", self.unpack_optional('uint64')),
            ("dtrx_trace", self.unpack_optional('augmented_transaction_trace')),
            ("partial_transaction", self.unpack_optional('partial_transaction'))
        ])

    def unpack_account_delta(self):
        return OrderedDict([
            ("account", self.unpack_name()),
            ("delta", self.unpack_uint64()),
        ])

    def unpack_auth_sequence(self):
        return OrderedDict([
            ("account_name", self.unpack_name()),
            ("sequence", self.unpack_uint64()),
        ])

    def unpack_action_receipt(self):
        return OrderedDict([
            ("version", self.unpack_varuint32()),
            ("receiver", self.unpack_name()),
            ("act_digest", self.unpack_checksum256()),
            ("global_sequence", self.unpack_uint64()),
            ("recv_sequence", self.unpack_uint64()),
            ("auth_sequence", self.unpack_array("auth_sequence")),
            ("code_sequence", self.unpack_varuint32()),
            ("abi_sequence", self.unpack_varuint32()),
        ])

    def unpack_action_trace(self):
        return OrderedDict([
            ("version", self.unpack_varuint32()),
            ("action_ordinal", self.unpack_varuint32()),
            ("creator_action_ordinal", self.unpack_varuint32()),
            ("receipt", self.unpack_optional('action_receipt')),
            ("receiver", self.unpack_name()),
            ("action", self.unpack_action()),
            ("context_free", self.unpack_uint8()),
            ("elapsed", self.unpack_int64()),
            ("console", self.unpack_string()),
            ("account_delta", self.unpack_array('account_delta')),
            ("error", self.unpack_optional('string')),
            ("error_code", self.unpack_optional('uint64')),
        ])

    def unpack_stat_table(self):
        return OrderedDict([
            ("supply", self.unpack_asset()),
            ("max_supply",  self.unpack_asset()),
            ("issuer", self.unpack_name()),
            ("pool1", self.unpack_extended_asset()),
            ("pool2", self.unpack_extended_asset()),
            ("fee", self.unpack_int32()),
            ("fee_contract", self.unpack_name())
        ])


    def unpack_history_context_wrapper(self):
        return OrderedDict([
            ("dummy", self.unpack_varuint32()),
            ("code",  self.unpack_name()),
            ("scope", self.unpack_name()),
            ("table", self.unpack_name()),
            ("pk", self.unpack_uint64()),
            ("payer", self.unpack_name()),
            ("value", self.unpack_bytes())
        ])

    def unpack_state_row(self):
        return OrderedDict([
            ("present", self.unpack_uint8()),
            ("bytes", self.unpack_bytes())
        ])

    def unpack_table_delta(self):
        return OrderedDict([
            ("struct_version", self.unpack_varuint32()),
            ("name", self.unpack_string()),
            ("rows", self.unpack_array("state_row"))
        ])
    
    def pack_block_position(self, v):
        self.pack_uint32(v['block_num'])
        self.pack_checksum256(v['block_id'])

    def unpack_block_position(self):
        return OrderedDict([
            ("block_num", self.unpack_uint32()),
            ("block_id", self.unpack_checksum256())
        ])

    def pack_get_blocks_request_v0(self, v):
        self.pack_uint32(v['start_block_num'])
        self.pack_uint32(v['end_block_num'])
        self.pack_uint32(v['max_messages_in_flight'])
        self.pack_array("block_position", v['have_positions'])
        self.pack_uint8(v['irreversible_only'])
        self.pack_uint8(v['fetch_block'])
        self.pack_uint8(v['fetch_traces'])
        self.pack_uint8(v['fetch_deltas'])

    def unpack_get_status_result_v0(self):
        return OrderedDict([
            ("head", self.unpack_block_position()),
            ("last_irreversible", self.unpack_block_position()),
            ("trace_begin_block", self.unpack_uint32()),
            ("trace_end_block", self.unpack_uint32()),
            ("chain_state_begin_block", self.unpack_uint32()),
            ("chain_state_end_block", self.unpack_uint32())
        ])

    def unpack_get_blocks_result_v0(self):
        return OrderedDict([
            ("head", self.unpack_block_position()),
            ("last_irreversible", self.unpack_block_position()),
            ("this_block", self.unpack_optional('block_position')),
            ("prev_block", self.unpack_optional('block_position')),
            ("block", self.unpack_optional('bytes')),
            ("traces", self.unpack_optional('bytes')),
            ("deltas", self.unpack_optional('bytes'))
        ])
