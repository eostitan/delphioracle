// EOSJS version 2 used
const { Api, JsonRpc, RpcError } = require('eosjs');
const fetch = require('node-fetch'); 
const { TextEncoder, TextDecoder } = require('util');  
const { JsSignatureProvider } = require('eosjs/dist/eosjs-jssig'); 
const axios = require('axios');
const dotenv = require('dotenv');

dotenv.load();

var chain = process.env.CHAIN;
var priceUrl = "";
var usdpair = "";
var btcpair = "";

// Switch priceURL depending on EOSIO Chain provided via .env
switch (chain) {
  case "wax":
	priceUrl = "https://min-api.cryptocompare.com/data/price?fsym=WAXP&tsyms=BTC,USD";
	usdpair = "waxpusd";
	btcpair = "waxpbtc";
    break;
  case "eos":
	priceUrl = "https://min-api.cryptocompare.com/data/price?fsym=EOS&tsyms=BTC,USD";
	usdpair = "eosusd";
	btcpair = "eosbtc";
    break;
  default:
	var priceUrl = "https://min-api.cryptocompare.com/data/price?fsym=WAXP&tsyms=BTC,USD";
	
}

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

function writequotes(){
	axios
	.get(priceUrl)
	.then(response => {
		//Assign repsonse to quotes
		const quotes2 = [{"value": Math.round((response.data.BTC)* 100000000), pair: btcpair }, {"value": Math.round((response.data.USD)* 10000), pair: usdpair }]
		console.log(quotes2)
        //Call eos.contracts method 
        eosmain(quotes2)
	})
}

writequotes();