const Eos = require('eosjs');
const dotenv = require('dotenv');
const axios = require('axios');

const url = "https://min-api.cryptocompare.com/data/price?fsym=EOS&tsyms=USD&e=coinbase";
//https://api.newdex.io/v1/price?symbol=everipediaiq-iq-eos
//https://api.newdex.io/v1/price?symbol=thepeostoken-peos-eos
//https://api.newdex.io/v1/price?symbol=betdicetoken-dice-eos
//https://api.newdex.io/v1/price?symbol=eosiotptoken-tpt-eos
//https://min-api.cryptocompare.com/data/price?fsym=EOS&tsyms=USD&e=Kraken

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

	axios.get(`${url}`)
		.then(results=>{

			if (results.data && results.data.USD){

				console.log(" results.data.USD",  results.data.USD);

				eos.contract(oracleContract)
					.then((contract) => {
						contract.write({
								owner: owner,
								quotes:[{value: parseInt(Math.round((results.data.USD+.5) * 1000000)), pair: "eosusd"}]
							},
							{
								scope: oracleContract,
								authorization: [`${owner}@${process.env.ORACLE_PERMISSION || 'active'}`] 
							})
							.then(results=>{
								console.log("results:", results);
								setTimeout(write, interval);
							})
							.catch(error=>{
								console.log("error:", error);
								setTimeout(write, interval);
							});

					})
					.catch(error=>{
						console.log("error:", error);
						setTimeout(write, interval);
					});

			}
			else setTimeout(write, interval);

		})
		.catch(error=>{
			console.log("error:", error);
			setTimeout(write, interval);
		});

}


write();

//setInterval(write, 60000);


