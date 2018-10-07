const Eos = require('eosjs');
const dotenv = require('dotenv');

dotenv.load();

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

eos.contract('eostitantest')
	.then((contract) => {
		contract.write({
				owner:"acryptotitan",
				value:564
			},
			{
				scope: "eostitantest",
				authorization: ['acryptotitan'] 
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


}

setInterval(write, 60000);