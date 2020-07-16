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

const btcUrl = "https://min-api.cryptocompare.com/data/price?fsym=BTC&tsyms=USD,CAD";
const btccnyUrl = "https://blockchain.info/ticker";

console.log(usdpair, btcpair, priceUrl);


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
		request.get(btcUrl, function (err, res, btcRes){
			request.get(btccnyUrl, function (err, res, btccnyRes){
					console.log("BTCUSD:", JSON.parse(btcRes).USD);
					console.log("BTCUSD:", JSON.parse(btcRes).CAD);
					console.log("BTCCNY:", JSON.parse(btccnyRes).CNY.last);


					/* var quotes = [{"value": parseInt(Math.round(JSON.parse(eosRes).BTC * 100000000)), pair:"eosbtc"}, 
									{"value": parseInt(Math.round(JSON.parse(eosRes).USD * 10000)), pair:"eosusd"}, 
									{"value": parseInt(Math.round(JSON.parse(btcRes).USD * 10000)), pair:"btcusd"}, 
									{"value": parseInt(Math.round(JSON.parse(btccnyRes).CNY.last * 10000)), pair:"btccny"}];
					*/
                    
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
		});
	});

}

write();

