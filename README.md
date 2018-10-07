![EOS TITAN](./eos_logo_white.jpg "EOS TITAN")

[https://eostitan.com](https://eostitan.com)

# DelphiOracle

The DelphiOracle dApp acts as a multi-party source of truth, designed to provide the near-realtime price of the EOS/USD pair to other smart contracts or to external users.

The dApp allows the currently elected block producers and other qualified oracles to push the price of EOS expressed in USD, at a frequency of up to 1 minute.

When a new datapoint is pushed to the contract, the contract will perform an approxmative, continuous moving average calculation over the last 21 periods and will store the value in RAM.

Consumer contracts or external applications can retrieve the last price and use it for their needs.

As more block producers and oracles will begin pushing the value at a 1 minute interval, confidence and accuracy of the value will increase.

This repository provides the code to the contract, as well as updating and fetching scripts written in node.js.

The fetching script use cryptocompare.com's api to retrieve the EOS/USD price.

**Note:** *price must be pushed as integer, using the last 4 digits to represent the value after the decimal separator (10,000th of a dollar precision)*

*Example: a value for EOS/USD of $5.85 would be pushed like this:*

```
cleos push action eostitantest write '{"owner":"delphioracle", "value":58500}' -p delphioracle@active
```

# Compile and deploy oracle.cpp

Clone repository

```
cd delphioracle
cd contract
eosio-cpp oracle.cpp -o oracle.wasm
cleos set code <eoscontract> oracle.wasm
cleos set abi <eoscontract> oracle.abi
```

# Set up and run updater.js

```
cd scripts
npm install
```

Copy sample.env to .env and update values

```
node updater.js
```


# Retrieve the last data point

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