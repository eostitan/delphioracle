// EOSJS version 2 used
const { Api, JsonRpc, RpcError } = require('eosjs');
const fetch = require('node-fetch');
const { TextEncoder, TextDecoder } = require('util');
const { JsSignatureProvider } = require('eosjs/dist/eosjs-jssig');
const axios = require('axios');
const dotenv = require('dotenv');

dotenv.load();

var chain = process.env.CHAIN;
var cryptocompareKey = process.env.CRYPTOCOMPARE;
priceUrl = `https://min-api.cryptocompare.com/data/pricemulti?fsyms=WAXP,USDC,USDT&tsyms=BTC,USD,ETH,EOS&api_key=${cryptocompareKey}`;
usdpair = "waxpusd";
btcpair = "waxpbtc";
eospair = "waxpeos";
ethpair = "waxpeth";
usdcpair = "usdcusd";
usdtpair = "usdtusd";

const owner = process.env.ORACLE;
const oracleContract = process.env.CONTRACT;
const defaultPrivateKey = process.env.EOS_KEY;
const permission = process.env.ORACLE_PERMISSION;
const httpEndpoint =  process.env.EOS_PROTOCOL + "://" +  process.env.EOS_HOST + ":" + process.env.EOS_PORT;

console.log(owner,oracleContract,permission)
const rpc = new JsonRpc(httpEndpoint, { fetch });
const signatureProvider = new JsSignatureProvider([defaultPrivateKey]);
const api = new Api({ rpc, signatureProvider, textDecoder: new TextDecoder(), textEncoder: new TextEncoder() });


const eosmain = async (quotes2) => {
    try {
    const result = await api.transact({
        actions: [{
            account: oracleContract,
            name: 'write',
            authorization: [{
                actor: owner,
                permission: permission,
            }],
            data: {
                owner: owner,
                quotes: quotes2
            },
        }]
        }, {
            blocksBehind: 3,
            expireSeconds: 30,
        });
        console.dir(result);
    } catch (e) {
        console.log('\nCaught exception: ' + e);
        if (e instanceof RpcError)
          console.log(JSON.stringify(e.json, null, 2));
      }
  }

async function writequotes() {
  try {
    const waxpResponse = await axios.get(priceUrl);
    const [usdcResponse, usdtResponse] = await Promise.all([
      axios.get('https://api.coingecko.com/api/v3/simple/price?ids=usd-coin&vs_currencies=usd'),
      axios.get('https://api.coingecko.com/api/v3/simple/price?ids=tether&vs_currencies=usd')
    ]);

    const waxpData = waxpResponse.data;
    const usdcPrice = usdcResponse.data['usd-coin'].usd;
    const usdtPrice = usdtResponse.data.tether.usd;

    const quotes2 = [
      { "value": Math.round(waxpData.WAXP.BTC * 100000000), pair: btcpair },
      { "value": Math.round(waxpData.WAXP.USD * 10000), pair: usdpair },
      { "value": Math.round(waxpData.WAXP.ETH * 100000000), pair: ethpair },
      { "value": Math.round(waxpData.WAXP.EOS * 1000000), pair: eospair },
      { "value": Math.round(usdcPrice * 10000), pair: usdcpair },
      { "value": Math.round(usdtPrice * 10000), pair: usdtpair }
    ];

    console.log(quotes2);
    await eosmain(quotes2);
  } catch (error) {
    console.log('\nCaught exception: ' + error);
    if (error instanceof RpcError) {
      console.log(JSON.stringify(error.json, null, 2));
    }
  }
}

writequotes();