"""Exceptions."""

import logging

logger = logging.getLogger(__name__)


class ConnectionError(Exception):
    def __init__(self, *, response, url, payload, error):
        try:
            text = response.text
        except AttributeError:
            text = ""
        msg = [
            f"{response=}",
            f"{text=}",
            f"{url=}",
            f"{payload=}",
            f"{error=}",
        ]
        msg = ";\n".join(msg)
        super().__init__(self, msg)
