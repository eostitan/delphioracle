#!/bin/bash

cleos set account permission $1 active --add-code
cleos push action $1 configure  "$(cat configure.json)" -p $1
