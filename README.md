![EOS TITAN](./eos_logo_white.jpg "EOS TITAN")

[https://eostitan.com](https://eostitan.com)

# DelphiOracle

The DelphiOracle dApp acts as a multi-party source of truth, designed to provide the price of the EOS/UDS pair to other smart contracts or to external users.

The dApp allows the currently elected block producers and other qualified oracles to push the price of EOS expressed in USD, up to every minute each.

When a new datapoint is pushed to the contract, it will perform an approxmative, continuous moving average calculation and store it with the entry.

Consumer contracts or external applications can retrieve the last price and use it for their needs.

This repository provides the code to the contract, as well as updating and fetching scripts written in node.js.