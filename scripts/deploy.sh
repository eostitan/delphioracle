#!/bin/bash

cleos --wallet-url http://keosd:8901 set contract $1 ../build/delphioracle/ delphioracle.wasm delphioracle.abi
