require('dotenv').load();
const colors = require("colors");
const Eos = require('eosjs');

const url = `${process.env.EOS_PROTOCOL}://${process.env.EOS_HOST}:${process.env.EOS_PORT}`;

const eos = Eos({
  httpEndpoint: url,
  chainId: process.env.EOS_CHAIN,
  verbose:false
});

///home/titan/jungle/eos/build/programs/cleos/cleos -u http://localhost:18888 get actions benchbenchbp -1 -5 -j

var previous_tx;

function getLastActions(){

	eos.getActions({
      "json": true,
      "account_name": "eosmechanics",
      "pos": -1,
      "offset": -10
    })
		.then(actions=>{
			
			//console.log("actions:", actions);

			if (actions.actions[0].action_trace.trx_id == previous_tx) return setTimeout(getLastActions, 1000);

			previous_tx = actions.actions[0].action_trace.trx_id;

			//console.log(actions.actions[0].action_trace.trx_id, block_results.producer);
			//console.log();

			eos.getTransaction(previous_tx)
				.then(tx=>{
					
					//console.log("tx:", tx);
					//setTimeout(getLastActions, 1000);

					setTimeout(function(){

						eos.getBlock(actions.actions[0].block_num)
							.then(block=>{
								console.log(previous_tx, " by ", block.producer, "took", tx.trx.receipt.cpu_usage_us);

								return setTimeout(getLastActions, 1000);

							})
							.catch(err=>{

								return setTimeout(getLastActions, 1000);

							});

					}, 1500);

				})

		})



}

getLastActions();
