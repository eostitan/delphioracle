/*

  DelphiOracle

  Author: Guillaume "Gnome" Babin-Tremblay - EOS Titan
  
  Website: https://eostitan.com
  Email: guillaume@eostitan.com

  Github: https://github.com/eostitan/delphioracle/
  
  Published under MIT License

TODO

OK - Add bounty system
OK - Add auto-voting system
OK - Parse memo field of transfers and allocate the transfer to bounty or to distribution
Add timeframes (in seconds)
  60
Add more configuration flexibility


*/

#include <eosio.system/eosio.system.hpp>
#include <eosiolib/symbol.hpp>
#include <eosiolib/chain.h>
#include <eosiolib/time.hpp>
#include <eosiolib/eosio.hpp>
#include <math.h>

using namespace eosio;

/*//Number of datapoints to hold per asset
static const uint64_t datapoints_count = 21; //don't change for now

//Number of bars to hold per asset
static const uint64_t bars_count = 30;

//revote every vote_interval datapoints
static const uint64_t vote_interval = 1;

//Min value set to 0.01$ , max value set to 10,000$
static const uint64_t val_min = 100;
static const uint64_t val_max = 100000000;

const uint64_t one_minute = 1000000 * 55; //give extra time for cron jobs

static const uint64_t standbys = 105; //allowed standby producers rank cutoff
static const uint64_t paid = 21; //maximum number of oracles getting paid from donations
*/


static const std::string system_str("system");

static const asset one_larimer = asset(1, S(4, EOS));

class DelphiOracle : public eosio::contract {
 public:
  DelphiOracle(account_name self) : eosio::contract(self) {}

  //Types

  enum asset_type: uint16_t {
      fiat=1, 
      cryptocurrency=2, 
      erc20_token=3, 
      eosio_token=4, 
      equity=5, 
      derivative=6, 
      other=7
  };

  enum instrument_type: uint16_t {
      pair=1, 
      value=2
  };

  struct globalinput {
    uint64_t datapoints_per_instrument;
    uint64_t bars_per_instrument;
    uint64_t vote_interval;
    uint64_t write_cooldown;
    uint64_t approver_threshold;
    uint64_t approving_oracles_threshold;
    uint64_t approving_custodians_threshold;
    uint64_t minimum_rank;
    uint64_t paid;
    uint64_t min_bounty_delay;
    uint64_t new_bounty_delay;
  };

  //Global config
  struct [[eosio::table]] global {

    //variables
    uint64_t id;
    uint64_t total_datapoints_count;
    asset total_claimed = asset(0, S(4, EOS));

    //constants
    uint64_t datapoints_per_instrument = 21;
    uint64_t bars_per_instrument = 30;
    uint64_t vote_interval = 10000;
    uint64_t write_cooldown = 1000000 * 55;
    uint64_t approver_threshold = 1;
    uint64_t approving_oracles_threshold = 1;
    uint64_t approving_custodians_threshold =1;
    uint64_t minimum_rank = 105;
    uint64_t paid = 21;
    uint64_t min_bounty_delay = 604800;
    uint64_t new_bounty_delay = 259200;

    uint64_t primary_key() const {return id;}

  };

  //Holds the last datapoints_count datapoints from qualified oracles
  struct [[eosio::table]] datapoints {
    uint64_t id;
    account_name owner; 
    uint64_t value;
    uint64_t median;
    uint64_t timestamp;

    uint64_t primary_key() const {return id;}
    uint64_t by_timestamp() const {return timestamp;}
    uint64_t by_value() const {return value;}

  };

  //Holds the count and time of last writes for qualified oracles
  struct [[eosio::table]] stats {
    account_name owner; 
    uint64_t timestamp;
    uint64_t count;
    uint64_t last_claim;
    asset balance;

    account_name primary_key() const {return owner;}
    uint64_t by_count() const {return -count;}

  };

  //Holds aggregated datapoints
  struct [[eosio::table]] bars {
    uint64_t id;

    uint64_t high;
    uint64_t low;
    uint64_t median;
    uint64_t timestamp;

    uint64_t primary_key() const {return id;}
    uint64_t by_timestamp() const {return timestamp;}

  };
/*
  //Holds bounties information
  struct [[eosio::table]] bounties {

    account_name name;
    std::string description;
    asset bounty;

    uint64_t primary_key() const {return name;}

  };*/

  //Holds rewards information
  struct [[eosio::table]] donations {

    uint64_t id;

    account_name donator;
    account_name pair;
    //instrument_type type;
    uint64_t timestamp;
    asset amount;

    uint64_t primary_key() const {return id;}

  };

  //Holds users information
  struct [[eosio::table]] users {

    account_name name;
    asset contribution;
    uint64_t score;
    uint64_t creation_timestamp;

    uint64_t primary_key() const {return name;}

  };

  //Holds custodians information
  struct [[eosio::table]] custodians {

    account_name name;

    uint64_t primary_key() const {return name;}

  };

  struct pairinput {
    account_name name;
    symbol_type base_symbol;
    asset_type base_type;
    account_name base_contract;
    symbol_type quote_symbol;
    asset_type quote_type;
    account_name quote_contract;
    uint64_t quoted_precision;
  };

  //Holds the list of pairs
  struct [[eosio::table]] pairs {
    //uint64_t id;

    bool active = false;
    bool bounty_awarded = false;
    bool bounty_edited_by_custodians = false;

    account_name proposer;
    account_name name;

    asset bounty_amount = asset(0, S(4, EOS));

    std::vector<account_name> approving_custodians;
    std::vector<account_name> approving_oracles;

    symbol_type base_symbol;
    asset_type base_type;
    account_name base_contract;

    symbol_type quote_symbol;
    asset_type quote_type;
    account_name quote_contract;
    
    uint64_t quoted_precision;

    uint64_t primary_key() const {return name;}
    //account_name by_name() const {return name;}

  };

  //Holds the list of pairs
  struct [[eosio::table]] networks {
    //uint64_t id;

    account_name name;

    uint64_t primary_key() const {return name;}
    //account_name by_name() const {return name;}

  };

  //Quote
  struct quote {
    uint64_t value;
    account_name pair;
  };

  //eosmechanics::cpu
  struct event {
    uint64_t value;
    account_name instrument;
  };

/*   struct blockchain_parameters {
      uint64_t free_ram()const { return max_ram_size - total_ram_bytes_reserved; }

      uint64_t             max_ram_size = 64ll*1024 * 1024 * 1024;
      uint64_t             total_ram_bytes_reserved = 0;
      int64_t              total_ram_stake = 0;

      block_timestamp      last_producer_schedule_update;
      time_point           last_pervote_bucket_fill;
      int64_t              pervote_bucket = 0;
      int64_t              perblock_bucket = 0;
      uint32_t             total_unpaid_blocks = 0; /// all blocks which have been produced but not paid
      int64_t              total_activated_stake = 0;
      time_point           thresh_activated_stake_time;
      uint16_t             last_producer_schedule_size = 0;
      double               total_producer_vote_weight = 0; /// the sum of all producer votes
      block_timestamp      last_name_close;

      uint64_t primary_key()const { return 1;      }  
   };
*/

  struct producer_info {
    name                  owner;
    double                total_votes = 0;
    eosio::public_key     producer_key; /// a packed public key object
    bool                  is_active = true;
    std::string           url;
    uint32_t              unpaid_blocks = 0;
    time_point            last_claim_time;
    uint16_t              location = 0;

    uint64_t primary_key()const { return owner.value;                             }
    double   by_votes()const    { return is_active ? -total_votes : total_votes;  }
    bool     active()const      { return is_active;                               }
    //void     deactivate()       { producer_key = public_key(); is_active = false; }

  };

  struct st_transfer {
      account_name  from;
      account_name  to;
      asset         quantity;
      std::string   memo;
  };

  //Multi index types definition
  typedef eosio::multi_index<N(global), global> globaltable;

  typedef eosio::multi_index<N(custodians), custodians> custodianstable;

  typedef eosio::multi_index<N(stats), stats,
      indexed_by<N(count), const_mem_fun<stats, uint64_t, &stats::by_count>>> statstable;

  typedef eosio::multi_index<N(pairs), pairs> pairstable;

  typedef eosio::multi_index<N(datapoints), datapoints,
      indexed_by<N(value), const_mem_fun<datapoints, uint64_t, &datapoints::by_value>>, 
      indexed_by<N(timestamp), const_mem_fun<datapoints, uint64_t, &datapoints::by_timestamp>>> datapointstable;

  typedef eosio::multi_index<N(producers), producer_info,
      indexed_by<N(prototalvote), const_mem_fun<producer_info, double, &producer_info::by_votes>>> producers_table;

  //typedef eosio::multi_index<N(bounties), bounties> bountiestable;

  //Check if calling account is a qualified oracle
  bool check_oracle(const account_name owner){

    globaltable gtable(get_self(), get_self());
    producers_table ptable(N(eosio), N(eosio));

    auto gitr = gtable.begin();

    auto p_idx = ptable.get_index<N(prototalvote)>();

    auto p_itr = p_idx.begin();

    uint64_t count = 0;

    while (p_itr != p_idx.end()) {
      print(p_itr->owner, "\n");
      if (p_itr->owner==owner) return true;
      p_itr++;
      count++;
      if (count>gitr->minimum_rank) break;
    }

    return false;
  }

  //Check if calling account is can vote on bounties
  bool check_approver(const account_name owner){

    globaltable gtable(get_self(), get_self());
    statstable stats(get_self(), get_self());

    auto gitr = gtable.begin();
    auto itr = stats.find(owner);

    if (itr != stats.end() && itr->count >= gitr->approver_threshold) return true;
    else return false;

  }

  //Ensure account cannot push data more often than every 60 seconds
  void check_last_push(const account_name owner, const account_name pair){

    globaltable gtable(get_self(), get_self());
    statstable gstore(get_self(), get_self());
    statstable store(get_self(), pair);

    auto gitr = gtable.begin();
    auto itr = store.find(owner);

    if (itr != store.end()) {

      uint64_t ctime = current_time();
      auto last = store.get(owner);

      eosio_assert(last.timestamp + gitr->write_cooldown <= ctime, "can only call every 60 seconds");

      store.modify( itr, get_self(), [&]( auto& s ) {
        s.timestamp = ctime;
        s.count++;
      });

    } else {

      store.emplace(get_self(), [&](auto& s) {
        s.owner = owner;
        s.timestamp = current_time();
        s.count = 1;
        s.balance = asset(0, S(4, EOS));
        s.last_claim = 0;
      });

    }

    auto gsitr = gstore.find(owner);
    if (gsitr != gstore.end()) {

      uint64_t ctime = current_time();

      gstore.modify( gsitr, get_self(), [&]( auto& s ) {
        s.timestamp = ctime;
        s.count++;
      });

    } else {

      gstore.emplace(get_self(), [&](auto& s) {
        s.owner = owner;
        s.timestamp = current_time();
        s.count = 1;
       s.balance = asset(0, S(4, EOS));
       s.last_claim = 0;
      });

    }

  }

  void update_votes(){

    print("voting for bps:", "\n");

    std::vector<account_name> bps;

    statstable gstore(get_self(), get_self());

    auto sorted_idx = gstore.get_index<N(count)>();
    auto itr = sorted_idx.begin();

    uint64_t count = 0;

    while(itr != sorted_idx.end() && count<30){
      print(itr->owner, "\n");
      bps.push_back(itr->owner);

      itr++;
      count++;

    }

    sort(bps.begin(), bps.end());

    action act(
      permission_level{_self, N(active)},
      N(eosio), N(voteproducer),
      std::make_tuple(_self, N(""), bps)
    );
    act.send();
 
  }

  //Push oracle message on top of queue, pop oldest element if queue size is larger than datapoints_count
  void update_datapoints(const account_name owner, const uint64_t value, pairstable::const_iterator pair_itr){

    globaltable gtable(get_self(), get_self());
    datapointstable dstore(get_self(), pair_itr->name);

    uint64_t median = 0;
    //uint64_t primary_key ;

    //Calculate new primary key by substracting one from the previous one
 /*   auto latest = dstore.begin();
    primary_key = latest->id - 1;*/

    auto t_idx = dstore.get_index<N(timestamp)>();

    auto oldest = t_idx.begin();
    
/*

  auto idx = hashtable.get_index<N(producer)>();
    auto itr = idx.find(producer);
    if (itr != idx.end()) {
      idx.modify(itr, producer, [&](auto& prod_hash) {
        prod_hash.hash = hash;
      });
    } else {*/


    //Pop oldest point
    //t_idx.erase(oldest);

    //Insert next datapoint
/*    auto c_itr = dstore.emplace(get_self(), [&](auto& s) {
      s.id = primary_key;
      s.owner = owner;
      s.value = value;
      s.timestamp = current_time();
    });*/


    t_idx.modify(oldest, get_self(), [&](auto& s) {
     // s.id = primary_key;
      s.owner = owner;
      s.value = value;
      s.timestamp = current_time();
    });

    //Get index sorted by value
    auto value_sorted = dstore.get_index<N(value)>();

    //skip first 10 values
    auto itr = value_sorted.begin();
    itr++;
    itr++;
    itr++;
    itr++;
    itr++;
    itr++;
    itr++;
    itr++;
    itr++;

    median=itr->value;

    //set median
    t_idx.modify(oldest, get_self(), [&](auto& s) {
      s.median = median;
    });

    gtable.modify(gtable.begin(), get_self(), [&](auto& s) {
      s.total_datapoints_count++;
    });

    auto gitr =  gtable.begin();

    print("gtable.begin()->total_datapoints_count:", gitr->total_datapoints_count,  "\n");

    if (gtable.begin()->total_datapoints_count % gitr->vote_interval == 0){
      update_votes();
    }

  }

  //Write datapoint
  [[eosio::action]]
  void write(const account_name owner, const std::vector<quote>& quotes) {
    
    require_auth(owner);
    
    int length = quotes.size();

    print("length ", length);

    eosio_assert(length>0, "must supply non-empty array of quotes");
    eosio_assert(check_oracle(owner), "account is not an a qualified oracle");

    statstable stable(get_self(), get_self());
    pairstable pairs(get_self(), get_self());

    auto oitr = stable.find(owner);

    

    //auto name_idx = pairs.find<N(name)>();
    //auto itr = sorted_idx.begin();


    for (int i=0; i<length;i++){
      print("quote ", i, " ", quotes[i].value, " ",  quotes[i].pair, "\n");
      
      auto itr = pairs.find(quotes[i].pair);

      eosio_assert(itr!=pairs.end() && itr->active == true, "pair not allowed");

      check_last_push(owner, quotes[i].pair);

      if (itr->bounty_amount>=one_larimer && oitr != stable.end()){

        //global donation to the contract, split between top oracles across all pairs
        stable.modify(*oitr, get_self(), [&]( auto& s ) {
          s.balance += one_larimer;
        });

        //global donation to the contract, split between top oracles across all pairs
        pairs.modify(*itr, get_self(), [&]( auto& s ) {
          s.bounty_amount -= one_larimer;
        });

      }
      else if (itr->bounty_awarded==false && itr->bounty_amount<one_larimer){

        //global donation to the contract, split between top oracles across all pairs
        pairs.modify(*itr, get_self(), [&]( auto& s ) {
          s.bounty_awarded = true;
        });

      }

      update_datapoints(owner, quotes[i].value, itr);

    }

    //for (int i=0; i<length;i++){
      //print("quote ", i, " ", quotes[i].value, " ",  quotes[i].pair, "\n");
      //eosio_assert(quotes[i].value >= val_min && quotes[i].value <= val_max, "value outside of allowed range");
    //}

/*    for (int i=0; i<length;i++){    
    }
*/
    print("done \n");
    //TODO: check if symbol exists
    //require_recipient(N(eosusdcom111));
    
  }

  //claim rewards
  [[eosio::action]]
  void claim(account_name owner) {
    
    require_auth(owner);

    globaltable gtable(get_self(), get_self());
    statstable sstore(get_self(), get_self());

    auto itr = sstore.find(owner);
    auto gitr = gtable.begin();

    eosio_assert(itr != sstore.end(), "oracle not found");
    eosio_assert( itr->balance.amount > 0, "no rewards to claim" );

    asset payout = itr->balance;

    //if( existing->quantity.amount == quantity.amount ) {
    //   bt.erase( *existing );
    //} else {
    sstore.modify( *itr, get_self(), [&]( auto& a ) {
        a.balance = asset(0, S(4, EOS));
        a.last_claim = current_time();
    });

    gtable.modify( *gitr, get_self(), [&]( auto& a ) {
        a.total_claimed += payout;
    });

    //}

    //if quantity symbol == EOS -> token_contract

   // SEND_INLINE_ACTION(token_contract, transfer, {N(eostitancore),N(active)}, {N(eostitancore), from, quantity, std::string("")} );
      
    action act(
      permission_level{_self, N(active)},
      N(eosio.token), N(transfer),
      std::make_tuple(_self, owner, payout, std::string(""))
    );
    act.send();

  }

  //temp configuration
  [[eosio::action]]
  void configure(globalinput g) {
    
    require_auth(_self);

    globaltable gtable(get_self(), get_self());
    pairstable pairs(get_self(), get_self());

    auto gitr = gtable.begin();
    auto pitr = pairs.begin();

    if (gitr == gtable.end()){

      gtable.emplace(get_self(), [&](auto& o) {
        o.id = 1;
        o.total_datapoints_count = 0;
        o.total_claimed = asset(0, S(4, EOS));
        o.datapoints_per_instrument = g.datapoints_per_instrument;
        o.bars_per_instrument = g.bars_per_instrument;
        o.vote_interval = g.vote_interval;
        o.write_cooldown = g.write_cooldown;
        o.approver_threshold = g.approver_threshold;
        o.approving_oracles_threshold = g.approving_oracles_threshold;
        o.approving_custodians_threshold = g.approving_custodians_threshold;
        o.minimum_rank = g.minimum_rank;
        o.paid = g.paid;
        o.min_bounty_delay = g.min_bounty_delay;
        o.new_bounty_delay = g.new_bounty_delay;
      });

    }
    else {

      gtable.modify(*gitr, get_self(), [&]( auto& o ) {
        o.datapoints_per_instrument = g.datapoints_per_instrument;
        o.bars_per_instrument = g.bars_per_instrument;
        o.vote_interval = g.vote_interval;
        o.write_cooldown = g.write_cooldown;
        o.approver_threshold = g.approver_threshold;
        o.approving_oracles_threshold = g.approving_oracles_threshold;
        o.approving_custodians_threshold = g.approving_custodians_threshold;
        o.minimum_rank = g.minimum_rank;
        o.paid = g.paid;
        o.min_bounty_delay = g.min_bounty_delay;
        o.new_bounty_delay = g.new_bounty_delay;
      });

    }

    if (pitr == pairs.end()){

        pairs.emplace(get_self(), [&](auto& o) {
          o.active = true;
          o.bounty_awarded = true;
          o.bounty_edited_by_custodians = true;
          o.proposer = N(delphioracle);
          o.name = N(eosusd);
          o.bounty_amount = asset(0, S(4, EOS));
          o.base_symbol =  S(4, EOS);
          o.base_type = asset_type::eosio_token;
          o.base_contract = N(eosio.token);
          o.quote_symbol = S(2, USD);
          o.quote_type = asset_type::fiat;
          o.quote_contract = N("");
          o.quoted_precision = 4;
        });

        datapointstable dstore(get_self(), N(eosusd));

        //First data point starts at uint64 max
        uint64_t primary_key = 0;
       
        for (uint16_t i=0; i < 21; i++){

          //Insert next datapoint
          auto c_itr = dstore.emplace(get_self(), [&](auto& s) {
            s.id = primary_key;
            s.value = 0;
            s.timestamp = 0;
          });

          primary_key++;

        }

    }

  }

  //Delphi Oracle - Bounty logic

  //Anyone can propose a bounty to add a new pair. This is the only way to add new pairs.
  //  By proposing a bounty, the proposer pays upfront for all the RAM requirements of the pair (expensive enough to discourage spammy proposals)

  //Once the bounty has been created, anyone can contribute to the bounty by sending a transfer with the bounty name in the memo

  //Custodians of the contract or the bounty proposer can cancel the bounty. This refunds RAM to the proposer, as well as all donations made to the bounty 
  //  to original payer accounts. 

  //Custodians of the contract can edit the bounty's name and description (curation and standardization process)

  //Any BP that has contributed a certain amount of datapoints (TBD) to the contract is automatically added as an authorized account to approve a bounty

  //Once a BP approves the bounty, a timer (1 week?) starts

  //X more BPs and Y custodians (1?) must then approve the bounty to activate it

  //The pair is not activated until the timer expires AND X BPs and Y custodians approved

  //No more than 1 pair can be activated per X period of time (72 hours?)

  //The bounty is then paid at a rate of X larimers per datapoint to BPs contributing to it until it runs out.


  //create a new pair request bounty
  [[eosio::action]]
  void newbounty(account_name proposer, pairinput pair) {

    require_auth(proposer);

    //Add request, proposer pays the RAM for the request + data structure for datapoints & bars. 

    pairstable pairs(get_self(), get_self());
    datapointstable dstore(get_self(), pair.name);

    auto itr = pairs.find(pair.name);

    eosio_assert(pair.name != N(system), "Cannot create a pair named system");
    eosio_assert(itr == pairs.end(), "A pair with this name already exists.");

    pairs.emplace(proposer, [&](auto& s) {
      s.proposer = proposer;
      s.name = pair.name;
      s.base_symbol = pair.base_symbol;
      s.base_type = pair.base_type;
      s.base_contract = pair.base_contract;
      s.quote_symbol = pair.quote_symbol;
      s.quote_type = pair.quote_type;
      s.quote_contract = pair.quote_contract;
      s.quoted_precision = pair.quoted_precision;
    });

    //First data point starts at uint64 max
    uint64_t primary_key = 0;
   
    for (uint16_t i=0; i < 21; i++){

      //Insert next datapoint
      auto c_itr = dstore.emplace(proposer, [&](auto& s) {
        s.id = primary_key;
        s.value = 0;
        s.timestamp = 0;
      });

      primary_key++;

    }

  }

  //cancel a bounty
  [[eosio::action]]
  void cancelbounty(account_name name, std::string reason) {
    
    pairstable pairs(get_self(), get_self());
    datapointstable dstore(get_self(),  name);

    auto itr = pairs.find(name);

    eosio_assert(itr != pairs.end(), "bounty doesn't exist");

    print("itr->proposer", itr->proposer, "\n");

    eosio_assert(has_auth(_self) || has_auth(itr->proposer), "missing required authority of contract or proposer");
    eosio_assert(itr->active == false, "cannot cancel live pair");

    //Cancel bounty, post reason to chain.

    pairs.erase(itr);

    while (dstore.begin() != dstore.end()) {
        auto ditr = dstore.end();
        ditr--;
        dstore.erase(ditr);
    }

    //TODO: Refund accumulated bounty to balance of user

  }

  //vote bounty
  [[eosio::action]]
  void votebounty(account_name owner, account_name bounty) {

    require_auth(owner); 

    pairstable pairs(get_self(), get_self());

    auto pitr = pairs.find(bounty);

    eosio_assert(!pitr->active, "pair is already active.");
    eosio_assert(pitr != pairs.end(), "bounty not found.");

    custodianstable custodians(get_self(), get_self());

    auto itr = custodians.find(owner);

    bool vote_approved = false;

    std::string err_msg = "";

    //print("itr->name", itr->name, "\n");

    if (itr != custodians.end()){
      //voter is custodian
      print("custodian found \n");

      std::vector<account_name> cv = pitr->approving_custodians;

      auto citr = find(cv.begin(), cv.end(), owner);

      //eosio_assert(citr == cv.end(), "custodian already voting for bounty");

      if (citr == cv.end()){

        cv.push_back(owner);

        pairs.modify(*pitr, get_self(), [&]( auto& s ) {
          s.approving_custodians = cv;
        });

        print("custodian added vote \n");

        vote_approved=true;
        
      }
      else err_msg = "custodian already voting for bounty";

    }

    print("checking oracle qualification... \n");

    if (check_approver(owner)) {

      std::vector<account_name> ov = pitr->approving_oracles;

      auto oitr = find(ov.begin(), ov.end(), owner);

      if (oitr == ov.end()){

        ov.push_back(owner);

        pairs.modify(*pitr, get_self(), [&]( auto& s ) {
          s.approving_oracles = ov;
        });

        print("oracle added vote \n");

        vote_approved=true;

      }
      else err_msg = "oracle already voting for bounty";

    }
    else err_msg = "owner not a qualified oracle";

    eosio_assert(vote_approved, err_msg.c_str());

    globaltable gtable(get_self(), get_self());

    auto gitr = gtable.begin();

    uint64_t approving_custodians_count = std::distance(pitr->approving_custodians.begin(), pitr->approving_custodians.end());
    uint64_t approving_oracles_count = std::distance(pitr->approving_oracles.begin(), pitr->approving_oracles.end());

    if (approving_custodians_count>=gitr->approving_custodians_threshold && approving_oracles_count>=gitr->approving_oracles_threshold){
        print("activate bounty", "\n");

        pairs.modify(*pitr, get_self(), [&]( auto& s ) {
          s.active = true;
        });

    }

  }

  //vote bounty
  //[[eosio::action]]
  void unvotebounty(account_name owner, account_name bounty) {

    require_auth(owner); 

    pairstable pairs(get_self(), get_self());

    auto pitr = pairs.find(bounty);

    eosio_assert(!pitr->active, "pair is already active.");
    eosio_assert(pitr != pairs.end(), "bounty not found.");

    custodianstable custodians(get_self(), get_self());

    auto itr = custodians.find(owner);

    print("itr->name", itr->name, "\n");

    if (itr != custodians.end()){
      //voter is custodian
      print("custodian found \n");

      std::vector<account_name> cv = pitr->approving_custodians;

      auto citr = find(cv.begin(), cv.end(), owner);

      eosio_assert(citr != cv.end(), "custodian is not voting for bounty");

      cv.erase(citr);

      pairs.modify(*pitr, get_self(), [&]( auto& s ) {
        s.approving_oracles = cv;
      });

      print("custodian removed vote \n");
      
    }
    else {

      print("checking oracle qualification... \n");

      //eosio_assert(check_approver(owner), "owner not a qualified oracle"); // not necessary

      std::vector<account_name> ov = pitr->approving_oracles;

      auto oitr = find(ov.begin(), ov.end(), owner);

      eosio_assert(oitr != ov.end(), "not an oracle or oracle is not voting for bounty");

      ov.erase(oitr);

      pairs.modify(*pitr, get_self(), [&]( auto& s ) {
        s.approving_oracles = ov;
      });

      print("oracle removed vote \n");

    }

  }

  //edit a bounty's information
  //[[eosio::action]]
  void editbounty(account_name name, pairinput pair) {

    account_name proposer;

    eosio_assert(has_auth(_self) || has_auth(proposer), "missing required authority of contract or proposer");

    //check if bounty_edited_by_custodians == false || has_auth(_self), otherwise throw exception. Custodians have final say
    //edit bounty data

    //if edited by custodians, set flag bounty_edited_by_custodians to true

  }

  //edit pair 
  //[[eosio::action]]
  void editpair(pairs pair) {
    
    require_auth(_self);
    
    //edit pair description

  }

  //delete pair 
  //[[eosio::action]]
  void deletepair(account_name name) {
    
    require_auth(_self); //controlled by msig over active key
    
    //delete pair, post reason to chain for reference

  }

  //add custodian
  //[[eosio::action]]
  void addcustodian(account_name name) {

    require_auth(_self); 

    custodianstable custodians(get_self(), get_self());

    custodians.emplace(get_self(), [&](auto& s) {
      s.name = name;
    });


  }

  //remove custodian
  //[[eosio::action]]
  void delcustodian(account_name name) {

    require_auth(_self); 

    custodianstable custodians(get_self(), get_self());

    auto itr = custodians.find(name);

    eosio_assert(itr != custodians.end(), "account not a custodian");

    custodians.erase(itr);

  }

  //registers a user
  //[[eosio::action]]
  void reguser(account_name owner) {

    require_auth(_self); 

  }

  //Clear all data
  [[eosio::action]]
  void clear(account_name pair) {
    require_auth(_self);

    globaltable gtable(get_self(), get_self());
    statstable gstore(get_self(), get_self());
    statstable lstore(get_self(), pair);
    datapointstable estore(get_self(),  pair);
    pairstable pairs(get_self(), get_self());
    custodianstable ctable(get_self(), get_self());
    
    while (ctable.begin() != ctable.end()) {
        auto itr = ctable.end();
        itr--;
        ctable.erase(itr);
    }

    while (gtable.begin() != gtable.end()) {
        auto itr = gtable.end();
        itr--;
        gtable.erase(itr);
    }

    while (gstore.begin() != gstore.end()) {
        auto itr = gstore.end();
        itr--;
        gstore.erase(itr);
    }

    while (lstore.begin() != lstore.end()) {
        auto itr = lstore.end();
        itr--;
        lstore.erase(itr);    
    }
    
    while (estore.begin() != estore.end()) {
        auto itr = estore.end();
        itr--;
        estore.erase(itr);
    }
    
    while (pairs.begin() != pairs.end()) {
        auto itr = pairs.end();
        itr--;
        pairs.erase(itr);
    }


  }

  void process_donation(account_name from, account_name scope, asset quantity){

    globaltable gtable(get_self(), get_self());
    statstable cstore(get_self(), scope);

    auto gitr = gtable.begin();

    uint64_t size = std::distance(cstore.begin(), cstore.end());

    uint64_t upperbound = std::min(size, gitr->paid); //max number of oracles being paid

    auto count_index = cstore.get_index<N(count)>(); //get list of oracles ranked by number of datapoints contributed for this scope (descending)

    auto itr = count_index.begin();

    uint64_t total_datapoints = 0; //gitr->total_datapoints_count;

    print("upperbound", upperbound, "\n");

    //Move pointer to upperbound, counting total number of datapoints for oracles elligible for payout
    for (uint64_t i=1;i<=upperbound;i++){
      total_datapoints+=itr->count;
      
      if (i<upperbound ){
        itr++;
        print("increment 1", "\n");

      } 

    }

    print("total_datapoints", total_datapoints, "\n"); //total datapoints for the eligible contributors

    uint64_t amount = quantity.amount;

    //Move pointer back to 0, calculating prorated contribution of oracle and allocating proportion of donation
    for (uint64_t i=upperbound;i>=1;i--){

      uint64_t datapoints = itr->count;

      double percent = ((double)datapoints / (double)total_datapoints) ;
      uint64_t uquota = (uint64_t)(percent * (double)quantity.amount) ;

      print("itr->owner", itr->owner, "\n");
      print("datapoints", datapoints, "\n");
      print("percent", percent, "\n");
      print("uquota", uquota, "\n");

      asset payout;

      //avoid rounding issues by giving leftovers to top contributor
      if (i == 1){
        payout = asset(amount, S(4, EOS));
      }
      else {
        payout = asset(uquota, S(4, EOS));
      }

      amount-= uquota;
      
      print("payout", payout, "\n");

      if (scope == get_self()) {

        //global donation to the contract, split between top oracles across all pairs
        cstore.modify(*itr, get_self(), [&]( auto& s ) {
          s.balance += payout;
        });

      }
      else {

        //donation to a specific pair, split between top oracles of only that pair
        statstable gstore(get_self(), get_self());

        auto optr = gstore.find(itr->owner);

        gstore.modify(*optr, get_self(), [&]( auto& s ) {
          s.balance += payout;
        });

      }
      
      if (i>1 ){
        itr--;
        print("decrement 1", "\n");

      } 
    }


  }

  void process_bounty(account_name from, account_name pair, asset quantity){

    pairstable pairs(get_self(), get_self());

    auto pitr = pairs.find(pair);

    pairs.modify(*pitr, get_self(), [&]( auto& s ) {
      s.bounty_amount += quantity;
    });

  }

  void transfer(uint64_t sender, uint64_t receiver) {

    print("transfer notifier", "\n");

    auto transfer_data = unpack_action_data<DelphiOracle::st_transfer>();

    print("transfer ", name{transfer_data.from}, " ",  name{transfer_data.to}, " ", transfer_data.quantity, "\n");

    //if incoming transfer
    if (transfer_data.from != _self && transfer_data.to == _self){
      
      //check if memo contains the name of an existing pair

      pairstable pairs(get_self(), get_self());
      //bountiestable bounties(get_self(), get_self());

      //auto name_index = pairs.get_index<N(name)>();

      print("string_to_name(transfer_data.memo.c_str())", string_to_name(transfer_data.memo.c_str()), "\n");
      print("transfer_data.memo.c_str()", transfer_data.memo.c_str(), "\n");


      if (transfer_data.memo.c_str() == system_str.c_str() ) return; //transfer to system account


      auto itr = pairs.find(string_to_name(transfer_data.memo.c_str()));

      //auto bitr = bounties.find(string_to_name(transfer_data.memo.c_str()));

      if (itr != pairs.end() && itr->bounty_awarded == true ) process_donation(name{transfer_data.from}, itr->name, transfer_data.quantity);
      else if (itr != pairs.end() && itr->bounty_awarded == false) process_bounty(name{transfer_data.from}, itr->name, transfer_data.quantity);
      else process_donation(name{transfer_data.from}, get_self(), transfer_data.quantity);

    }

  }

};

#define EOSIO_ABI_EX( TYPE, MEMBERS ) \
extern "C" { \
   void apply( uint64_t receiver, uint64_t code, uint64_t action ) { \
      auto self = receiver; \
      if( code == self || code == N(eosio.token)) { \
         if( action == N(transfer)){ \
          eosio_assert( code == N(eosio.token), "Must transfer EOS"); \
         } \
         TYPE thiscontract( self ); \
         switch( action ) { \
            EOSIO_API( TYPE, MEMBERS ) \
         } \
         /* does not allow destructor of thiscontract to run: eosio_exit(0); */ \
      } \
   } \
}

//EOSIO_ABI(DelphiOracle, (write)(clear)(configure)(transfer))

EOSIO_ABI_EX( DelphiOracle, (write)(clear)(newbounty)(cancelbounty)(addcustodian)(delcustodian)(votebounty)(unvotebounty)(claim)(configure)(transfer))
