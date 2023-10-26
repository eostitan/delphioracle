"""
Blockchain network main class and its derivatives.

Nodeos api reference:
https://developers.eos.io/manuals/eos/latest/nodeos/plugins/chain_api_plugin/api-reference/index
"""

import typing
from urllib.parse import urljoin

import httpx
import pydantic

from eospyo import exc


class Net(pydantic.BaseModel):
    """
    The net hold the connection information with the blockchain network api.
    """  # NOQA: D200

    host: pydantic.AnyHttpUrl

    def _request(
        self,
        *,
        endpoint: str,
        payload: typing.Optional[dict] = dict(),
        verb: str = "POST",
    ):
        url = urljoin(self.host, endpoint)

        try:
            resp = httpx.post(url, json=payload)
        except (
            httpx.TimeoutException,
            httpx.NetworkError,
            httpx.WriteError,
        ) as e:
            raise exc.ConnectionError(
                response=None, url=url, payload=payload, error=e
            )

        if resp.status_code > 299 and resp.status_code != 500:
            raise exc.ConnectionError(
                response=resp, url=url, payload=payload, error=None
            )

        return resp.json()

    def abi_bin_to_json(
        self, *, account_name: str, action: str, bytes: dict
    ) -> dict:
        endpoint = "/v1/chain/abi_bin_to_json"
        payload = dict(code=account_name, action=action, binargs=bytes.hex())
        data = self._request(endpoint=endpoint, payload=payload)
        return data["args"]

    def abi_json_to_bin(
        self, *, account_name: str, action: str, json: dict
    ) -> bytes:
        """
        Return a dict containing the serialized action data.

        https://developers.eos.io/manuals/eos/latest/nodeos/plugins/chain_api_plugin/api-reference/index#operation/abi_json_to_bin
        """
        endpoint = "/v1/chain/abi_json_to_bin"
        payload = dict(code=account_name, action=action, args=json)
        data = self._request(endpoint=endpoint, payload=payload)
        if "binargs" not in data:
            return data
        hex_ = data["binargs"]
        bytes_ = bytes.fromhex(hex_)
        return bytes_

    def get_info(self):
        endpoint = "/v1/chain/get_info"
        data = self._request(endpoint=endpoint)
        return data

    def get_account(self, *, account_name: str):
        """
        Return an account information.

        If no account is found, then raises an connection error
        https://developers.eos.io/manuals/eos/latest/nodeos/plugins/chain_api_plugin/api-reference/index#operation/get_account
        """
        endpoint = "/v1/chain/get_account"
        payload = dict(account_name=account_name)
        data = self._request(endpoint=endpoint, payload=payload)
        return data

    def get_abi(self, *, account_name: str):
        """
        Retrieve the ABI for a contract based on its account name.

        https://developers.eos.io/manuals/eos/latest/nodeos/plugins/chain_api_plugin/api-reference/index#operation/get_abi
        """
        endpoint = "/v1/chain/get_abi"
        payload = dict(account_name=account_name)
        data = self._request(endpoint=endpoint, payload=payload)
        if len(data) == 1:
            return None
        return data

    def get_block(self, *, block_num_or_id: str):
        """
        Return various details about a specific block on the blockchain.

        https://developers.eos.io/manuals/eos/latest/nodeos/plugins/chain_api_plugin/api-reference/index#operation/get_block
        """
        endpoint = "/v1/chain/get_block"
        payload = dict(block_num_or_id=block_num_or_id)
        data = self._request(endpoint=endpoint, payload=payload)
        return data

    def get_block_info(self, *, block_num: str):
        """
        Return a fixed-size smaller subset of the block data.

        Similar to get_block
        https://developers.eos.io/manuals/eos/latest/nodeos/plugins/chain_api_plugin/api-reference/index#operation/get_block_info
        """
        endpoint = "/v1/chain/get_block_info"
        payload = dict(block_num=block_num)
        data = self._request(endpoint=endpoint, payload=payload)
        return data

    def get_table_by_scope(
        self,
        code: str,
        table: str = None,
        lower_bound: str = None,
        upper_bound: str = None,
        limit: int = None,
        reverse: bool = None,
        show_payer: bool = None,
    ):
        """
        Return a dict with all tables and their scope.

        Similar to get_table_by_scope
        https://developers.eos.io/manuals/eos/latest/nodeos/plugins/chain_api_plugin/api-reference/index#operation/get_table_by_scope
        """
        endpoint = "/v1/chain/get_table_by_scope"
        payload = dict(
            code=code,
            table=table,
            lower_bound=lower_bound,
            upper_bound=upper_bound,
            limit=limit,
            reverse=reverse,
            show_payer=show_payer,
        )
        for k in list(payload.keys()):
            if payload[k] is None:
                del payload[k]
        data = self._request(endpoint=endpoint, payload=payload)
        return data

    def get_table_rows(
        self,
        code: str,
        table: str,
        scope: str,
        json: bool = True,
        index_position: str = None,
        key_type: str = None,
        encode_type: str = None,
        lower_bound: str = None,
        upper_bound: str = None,
        limit: int = None,
        reverse: int = None,
        show_payer: int = None,
    ):
        """
        Return a list with the rows in the table.

        Similar to get_table_rows
        https://developers.eos.io/manuals/eos/latest/nodeos/plugins/chain_api_plugin/api-reference/index#operation/get_table_rows

        Parameters:
        -----------
        json: bool = True
            Get the response as json
        """
        endpoint = "/v1/chain/get_table_rows"
        payload = dict(
            code=code,
            table=table,
            scope=scope,
            json=json,
            index_position=index_position,
            key_type=key_type,
            encode_type=encode_type,
            lower_bound=lower_bound,
            upper_bound=upper_bound,
            limit=limit,
            reverse=reverse,
            show_payer=show_payer,
        )
        for k in list(payload.keys()):
            if payload[k] is None:
                del payload[k]
        data = self._request(endpoint=endpoint, payload=payload)
        if "rows" in data:
            return data["rows"]
        else:
            return data

    def push_transaction(
        self,
        *,
        transaction: object,
        compression: bool = False,
        packed_context_free_data: str = "",
    ):
        """
        Send a transaction to the blockchain.

        https://developers.eos.io/manuals/eos/latest/nodeos/plugins/chain_api_plugin/api-reference/index#operation/push_transaction
        """
        endpoint = "/v1/chain/push_transaction"
        payload = dict(
            signatures=transaction.signatures,
            compression=compression,
            packed_context_free_data=packed_context_free_data,
            packed_trx=transaction.pack(),
        )
        data = self._request(endpoint=endpoint, payload=payload)
        return data


class WaxTestnet(Net):
    host: pydantic.HttpUrl = "https://testnet.waxsweden.org/"


class WaxMainnet(Net):
    host: pydantic.HttpUrl = "https://facings.waxpub.net"


class EosMainnet(Net):
    host: pydantic.HttpUrl = "https://api.eossweden.org"


class KylinTestnet(Net):
    host: pydantic.HttpUrl = "https://kylin.eossweden.org"


class Jungle3Testnet(Net):
    host: pydantic.HttpUrl = "https://jungle3.eossweden.org"


class TelosMainnet(Net):
    host: pydantic.HttpUrl = "https://telos.caleos.io/"


class TelosTestnet(Net):
    host: pydantic.HttpUrl = "https://testnet.telos.caleos.io"


class ProtonMainnet(Net):
    host: pydantic.HttpUrl = "https://proton.cryptolions.io"


class ProtonTestnet(Net):
    host: pydantic.HttpUrl = "https://testnet.protonchain.com"


class UosMainnet(Net):
    host: pydantic.HttpUrl = "https://uos.eosusa.news"


class FioMainnet(Net):
    host: pydantic.HttpUrl = "https://fio.cryptolions.io"


class Local(Net):
    host: pydantic.HttpUrl = "http://127.0.0.1:8888"


__all__ = [
    "Net",
    "EosMainnet",
    "KylinTestnet",
    "Jungle3Testnet",
    "TelosMainnet",
    "TelosTestnet",
    "ProtonMainnet",
    "ProtonTestnet",
    "UosMainnet",
    "FioMainnet",
    "WaxTestnet",
    "WaxMainnet",
    "Local",
]
