#!/bin/bash

eosio-cpp -abigen -I ../include/delphioracle -I ../include/eosio.contracts/contracts/eosio.system/include -o ../build/delphioracle.wasm ../src/delphioracle.cpp
sed -i 's/asset_type/uint16/g' ../build/delphioracle.abi
