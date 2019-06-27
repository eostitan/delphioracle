![EOS TITAN](./eos_logo_white.jpg "EOS TITAN")

[https://eostitan.com](https://eostitan.com)

# DelphiOracle

The DelphiOracle contract acts as a multi-party source of truth, designed to provide the near-realtime price of the asset pairs to other smart contracts or to external users.

The contract allows the current top 105 block producers to push rates for various assets, at a maximum frequency of 1 minute per asset.

When a new datapoint is pushed to the contract, the contract will use the median from the last 21 datapoints.

This provides strong DPOS byzantine fault tolerance guarantees, ensuring a reliable pricefeed even if up to 10 block producers are colluding or corrupt.

Consumer contracts or external applications can retrieve the last price and use it for their needs.

As more block producers and oracles will begin pushing the value at a 1 minute interval, confidence and accuracy of the value will increase.

This repository provides the code to the contract, as well as an updating script written in node.js for oracles and block producers to use. Ideally, block producers and oracles would use their own mechanism to retrieve the data, using various sources.

The updating script use cryptocompare.com's api to retrieve the EOS/USD price.

## Incentive mechanism for BPs to push rates

Each time a BP pushes a datapoint, a counter for this BP is incremented. The contract supports an EOS transfer notification handler which splits any EOS reward sent to the contract between BPs that are pushing rates, proportionally to the number of datapoints they have pushed.

This allows for anyone relying on this pricefeed to incentivize BPs to join and to push rates, simply by transferring any amount of EOS to the contract.

BPs can claim these rewards by calling the claim function.

```
cleos push action <eoscontract> claim '{"owner":"<account>"}' -p <account>

```

In addition, the contract act as a proxy, and automatically revotes every 10,000 datapoints for up to 30 BPs, ranking them by total number of datapoints contributed since inception.

[https://www.alohaeos.com/vote/proxy/delphioracle](https://www.alohaeos.com/vote/proxy/delphioracle)

Users and dApps relying on DelphiOracle are invited to delegate their votes to it, and support contributing BPs.

## Compile and deploy oracle.cpp (using eosio.cdt v.1.2.x)

Clone repository

```
cd delphioracle
cd contract
eosio-cpp oracle.cpp -o oracle.wasm #need to incluse path to eosio.system.hpp file
cleos set code <eoscontract> oracle.wasm
cleos set abi <eoscontract> oracle.abi
```

## Push value to the contract

Qualified block producers can call the contract up to once every minute, to provide the current price of any asset pair.

**Note:**

*for EOS/USD (eosusd), price must be pushed as integer, using the last 4 digits to represent the value after the decimal separator (10,000th of a dollar precision)*

*for EOS/BTC (eosbtc), price must be pushed as integer, using the last 8 digits to represent the value after the decimal separator (100,000,000th of a bitcoin precision)*

Example: a value for EOS/USD of $5.85 pushed by block producer acryptotitan to delphioracle contract would look like this:

```
cleos push action delphioracle write '{"owner":"acryptotitan", "value":58500, "symbol":"eosusd"}' -p acryptotitan@active
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
cleos get table <eoscontract> eosusd datapoints --limit 1
```

Sample output:
```
{
  "rows": [{
      "id": "18446744073709551508",
      "owner": "acryptotitan",
      "value": 56800,
      "median": 56863,
      "timestamp": "1538937978500000"
    }
  ],
  "more": true
}
```