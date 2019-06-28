const Eos = require('eosjs');
const dotenv = require('dotenv');
const axios = require('axios');
const request = require('request');

const ccUrl = "https://min-api.cryptocompare.com/data/price?fsym=EOS&tsyms=BTC,USD,CNY";
//const btcUrl = "https://min-api.cryptocompare.com/data/price?fsym=EOS&tsyms=BTC";

dotenv.load();

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


	request.get(ccUrl, function (err, res, ccRes){
			
			console.log("USD:", JSON.parse(ccRes).USD);
			console.log("BTC:", JSON.parse(ccRes).BTC);
			console.log("CNY:", JSON.parse(ccRes).CNY);

			var quotes = [{"value": parseInt(Math.round(JSON.parse(btcRes).BTC * 100000000)), pair:"eosbtc"}, 
										{"value": parseInt(Math.round(JSON.parse(usdRes).USD * 10000)), pair:"eosusd"}, 
										{"value": parseInt(Math.round(JSON.parse(usdRes).CNY * 10000)), pair:"eoscny"}];

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

