![EOS TITAN](./eos_logo_white.jpg "EOS TITAN")

[https://eostitan.com](https://eostitan.com)

# DelphiOracle

The DelphiOracle contract acts as a multi-party source of truth, designed to provide the near-realtime price of the EOS/USD pair to other smart contracts or to external users.

The contract allows the currently elected block producers and other pre-approved oracles to push the price of EOS expressed in USD, at a maximum frequency of 1 minute.

Pre-approved oracles are set manually as a mean to bootstrap the oracle's pricefeed. As more elected block producers start pushing values, pre-approved oracles will be removed.

When a new datapoint is pushed to the contract, the contract will remove the top 6 and bottom 6 values, and will perform a continuous moving average calculation over the remaining 9 values.

This provides the same DPOS byzantine fault tolerance guarantees, ensuring a reliable pricefeed even if up to 6 block producers are colluding or corrupt.

Consumer contracts or external applications can retrieve the last price and use it for their needs.

As more block producers and oracles will begin pushing the value at a 1 minute interval, confidence and accuracy of the value will increase.

This repository provides the code to the contract, as well as an updating script written in node.js for oracles and block producers to use. Ideally, block producers and oracles would use their own mechanism to retrieve the data, using various sources.

The updating script use cryptocompare.com's api to retrieve the EOS/USD price.

This contract has been deployed to EOS Mainnet and to CryptoKylin testnet, both on account delphioracle.

## Compile and deploy oracle.cpp (using eosio.cdt v.1.2.x)

Clone repository

```
cd delphioracle
cd contract
eosio-cpp oracle.cpp -o oracle.wasm
cleos set code <eoscontract> oracle.wasm
cleos set abi <eoscontract> oracle.abi
```

## Push value to the contract

Pre-approved oracles and currently elected block producers can call the contract up to once every minute, to provide the current price of the EOS/USD pair.

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

**Required:** Copy sample.env to .env and update values

Example file for CryptoKylin:

```
EOS_PROTOCOL='http'
EOS_HOST='api.kylin.alohaeos.com'
EOS_PORT='80'
EOS_KEY='5... <replace with private key>'
EOS_CHAIN='5fff1dae8dc8e2fc4d5b23b2c7665c97f9e9d8edf2b6485a86ba311c25639191'
ORACLE="acryptotitan"
CONTRACT="delphioracle"
FREQ=15000
ORACLE_PERMISSION="active"
```

Run script:

```
node updater.js
```

**Optional:** Create a custom permission for oracle write action. Custom oracle permission can be supplied in the .env file under ORACLE_PERMISSION (defaults to active).

```
cleos set account permission <eosaccount> <permissionname> <eoskey> <permissionparent>
cleos set action permission <eosaccount> <eoscontract> <action> <permissionname>
```

Example:

```
cleos set account permission eostitantest oracle EOS6JhWzHJWystQmEv8VbXTHVagf5LKRVjkowwsdqCNYDFxYZQEJ9 active
cleos set action permission eostitantest delphioracle write oracle
```

## Retrieve the last data point

**Note:** *Use average / 10000 to get the actual value.*

```
cleos get table <eoscontract> <eoscontract> eosusd --limit 1
```

Sample output:
```
{
  "rows": [{
      "id": "18446744073709551508",
      "owner": "acryptotitan",
      "value": 56800,
      "average": 56863,
      "timestamp": "1538937978500000"
    }
  ],
  "more": true
}
```