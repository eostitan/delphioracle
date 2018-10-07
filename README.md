![EOS TITAN](./eos_logo_white.jpg "EOS TITAN")

[https://eostitan.com](https://eostitan.com)

# DelphiOracle

The DelphiOracle dApp acts as a multi-party source of truth, designed to provide the near-realtime price of the EOS/USD pair to other smart contracts or to external users.

The dApp allows the currently elected block producers and other qualified oracles to push the price of EOS expressed in USD, at a maximum frequency of 1 minute.

When a new datapoint is pushed to the contract, the contract will perform a continuous moving average calculation over the last 21 values pushed.

Consumer contracts or external applications can retrieve the last price and use it for their needs.

As more block producers and oracles will begin pushing the value at a 1 minute interval, confidence and accuracy of the value will increase.

This repository provides the code to the contract, as well as an updating scripts written in node.js for oracles and block producers to use. Ideally, block producers and oracles would use their own mechanism to retrieve the data, using various sources.

The upating script use cryptocompare.com's api to retrieve the EOS/USD price.

This contract has been deployed to the CryptoKylin testnet, on account delphioracle.

## Compile and deploy oracle.cpp

Clone repository

```
cd delphioracle
cd contract
eosio-cpp oracle.cpp -o oracle.wasm
cleos set code <eoscontract> oracle.wasm
cleos set abi <eoscontract> oracle.abi
```

## Push value to the contract

Qualified oracles and currently elected block producers can call the contract up to once every minute, to provide the current price of EOS/USD.

**Note:** *price must be pushed as integer, using the last 4 digits to represent the value after the decimal separator (10,000th of a dollar precision)*

Example: a value for EOS/USD of $5.85 pushed by block producer acryptotitan to delphioracle contract would look like this:

```
cleos push action delphioracle write '{"owner":"acryptotitan", "value":58500}' -p acryptotitan@active
```


## Set up and run updater.js

Updater.js is a nodejs module meant to retrieve the EOS/USD price using cryptocompare.com's API, and push the result to the DelphiOracle smart contract automatically and continuously.

```
cd scripts
npm install
```

Copy sample.env to .env and update values

```
node updater.js
```


## Retrieve the last data point

**Note:** *Use average / 10000 to get the actual value.*

```
cleos get table <eoscontract> <eoscontract> eosusdstore --limit 1
```

Sample output:
```
{
  "rows": [{
      "id": "18446744073709551508",
      "owner": "acryptotitan",
      "value": 56800,
      "accumulator": 1194100,
      "average": 56863,
      "timestamp": "1538937978500000"
    }
  ],
  "more": true
}
```