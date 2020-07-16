const Eos = require('eosjs');
const dotenv = require('dotenv');
//const axios = require('axios');
const request = require('request');

dotenv.load();

var chain = process.env.CHAIN;
console.log(chain)
var priceUrl = "";
var usdpair = "";
var btcpair = "";
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

const interval = process.env.FREQ;
const owner = process.env.ORACLE;
const oracleContract = process.env.CONTRACT;

const eos = Eos({ 
  httpEndpoint: process.env.EOS_PROTOCOL + "://" +  process.env.EOS_HOST + ":" + process.env.EOS_PORT,
  keyProvider: process.env.EOS_KEY,
  chainId: process.env.EOS_CHAIN,
  verbose:false,
  logger: {
    log: null,
    error: null
  }
});

function write(){


	request.get(priceUrl, function (err, res, priceRes){
		var quotes = [{"value": parseInt(Math.round(JSON.parse(priceRes).USD * 10000)), pair: usdpair }, {"value": parseInt(Math.round(JSON.parse(priceRes).BTC * 100000000)), pair: btcpair }];
		console.log("quotes:", quotes);

		eos.contract(oracleContract)
			.then((contract) => {
				contract.write({
						owner: owner,
						quotes: quotes
					},
					{
						scope: oracleContract,
						authorization: [`${owner}@${process.env.ORACLE_PERMISSION || 'active'}`] 
					})
					.then(results=>{
						console.log("results:", results);
					})
					.catch(error=>{
						console.log("error:", error);
					});

			})
			.catch(error=>{
				console.log("error:", error);
			});
	});
}

write();

