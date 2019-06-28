/*

  DelphiOracle

  Author: Guillaume "Gnome" Babin-Tremblay - EOS Titan
  
  Website: https://eostitan.com
  Email: guillaume@eostitan.com

  Github: https://github.com/eostitan/delphioracle/
  
  Published under MIT License

TODO

Add bounty system
OK - Add auto-voting system
Parse memo field of transfers and allocate the transfer to bounty or to distribution
Add timeframes (in seconds)
  1 min   -> 60
  60 min  -> 3600
  daily   -> 86400
Add user - payer logic for additional pairs

*/

#include <eosio.system/eosio.system.hpp>
#include <eosiolib/symbol.hpp>
#include <eosiolib/chain.h>
#include <eosiolib/time.hpp>
#include <eosiolib/eosio.hpp>
#include <math.h>

using namespace eosio;

//Number of datapoints to hold per asset
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

class DelphiOracle : public eosio::contract {
 public:
  DelphiOracle(account_name self) : eosio::contract(self) {}

  //Types

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

  //Holds bounties information
  struct [[eosio::table]] bounties {

    std::string description;
    account_name name;
    asset bounty;

    uint64_t primary_key() const {return name;}

  };

  //Global config
  struct [[eosio::table]] global {
    uint64_t id;
    uint64_t total_datapoints_count;
    //asset total_claimed;
    
    uint64_t primary_key() const {return id;}

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

  //Holds the list of pairs
  struct [[eosio::table]] pairs {
    uint64_t id;
    asset bounty;

    account_name name;

    symbol_type base_symbol;
    symbol_type quote_symbol;
    uint64_t precision;

    account_name owner;

    uint64_t primary_key() const {return id;}
    account_name by_name() const {return name;}

  };

  //Quote
  struct quote {
    uint64_t value;
    account_name pair;
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

  typedef eosio::multi_index<N(stats), stats,
      indexed_by<N(count), const_mem_fun<stats, uint64_t, &stats::by_count>>> statstable;

  typedef eosio::multi_index<N(pairs), pairs, 
      indexed_by<N(name), const_mem_fun<pairs, uint64_t, &pairs::by_name>>> pairstable;
  typedef eosio::multi_index<N(datapoints), datapoints,
      indexed_by<N(value), const_mem_fun<datapoints, uint64_t, &datapoints::by_value>>, 
      indexed_by<N(timestamp), const_mem_fun<datapoints, uint64_t, &datapoints::by_timestamp>>> datapointstable;

  typedef eosio::multi_index<N(producers), producer_info,
      indexed_by<N(prototalvote), const_mem_fun<producer_info, double, &producer_info::by_votes>>> producers_table;

  typedef eosio::multi_index<N(bounties), bounties> bountiestable;

  //Check if calling account is a qualified oracle
  bool check_oracle(const account_name owner){

    producers_table ptable(N(eosio), N(eosio));

    auto p_idx = ptable.get_index<N(prototalvote)>();

    auto p_itr = p_idx.begin();

    uint64_t count = 0;

    while (p_itr != p_idx.end()) {
      print(p_itr->owner, "\n");
      if (p_itr->owner==owner) return true;
      p_itr++;
      count++;
      if (count>standbys) break;
    }

    return false;
  }

  //Ensure account cannot push data more often than every 60 seconds
  void check_last_push(const account_name owner, const account_name pair){

    statstable gstore(get_self(), get_self());
    statstable store(get_self(), pair);

    auto itr = store.find(owner);
    if (itr != store.end()) {

      uint64_t ctime = current_time();
      auto last = store.get(owner);

      eosio_assert(last.timestamp + one_minute <= ctime, "can only call every 60 seconds");

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

    auto gitr = gstore.find(owner);
    if (gitr != gstore.end()) {

      uint64_t ctime = current_time();

      gstore.modify( gitr, get_self(), [&]( auto& s ) {
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
  void update_datapoints(const account_name owner, const uint64_t value, account_name pair){

    datapointstable dstore(get_self(), pair);

    auto size = std::distance(dstore.begin(), dstore.end());

    uint64_t median = 0;
    uint64_t primary_key ;

    //Find median
    if (size>0){

      //Calculate new primary key by substracting one from the previous one
      auto latest = dstore.begin();
      primary_key = latest->id - 1;

      //If new size is greater than the max number of datapoints count
      if (size+1>datapoints_count){

        auto oldest = dstore.end();
        oldest--;

        //Pop oldest point
        dstore.erase(oldest);

        //Insert next datapoint
        auto c_itr = dstore.emplace(get_self(), [&](auto& s) {
          s.id = primary_key;
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
        dstore.modify(c_itr, get_self(), [&](auto& s) {
          s.median = median;
        });

      }
      else {

        //No median is calculated until the expected number of datapoints have been received
        median = value;

        //Push new point at the end of the queue
        dstore.emplace(get_self(), [&](auto& s) {
          s.id = primary_key;
          s.owner = owner;
          s.value = value;
          s.median = median;
          s.timestamp = current_time();
        });

      }

    }
    else {

      //First data point starts at uint64 max
      primary_key = std::numeric_limits<unsigned long long>::max();
      median = value;

      //Push new point at the end of the queue
      dstore.emplace(get_self(), [&](auto& s) {
        s.id = primary_key;
        s.owner = owner;
        s.value = value;
        s.median = median;
        s.timestamp = current_time();
      });

    }

    globaltable gtable(get_self(), get_self());

    gtable.modify(gtable.begin(), get_self(), [&](auto& s) {
      s.total_datapoints_count++;
    });

    print("gtable.begin()->total_datapoints_count:", gtable.begin()->total_datapoints_count,  "\n");

    if (gtable.begin()->total_datapoints_count % vote_interval == 0){
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
    eosio_assert(check_oracle(owner), "account is not an active producer or approved oracle");

    for (int i=0; i<length;i++){
      print("quote ", i, " ", quotes[i].value, " ",  quotes[i].pair, "\n");
      eosio_assert(quotes[i].value >= val_min && quotes[i].value <= val_max, "value outside of allowed range");
    }

    for (int i=0; i<length;i++){    
      check_last_push(owner, quotes[i].pair);
      update_datapoints(owner, quotes[i].value, quotes[i].pair);
    }

    print("done \n");
    //TODO: check if symbol exists
    //require_recipient(N(eosusdcom111));
    
  }

  //claim rewards
  [[eosio::action]]
  void claim(account_name owner) {
    
    require_auth(owner);

   // globaltable gtable(get_self(), get_self());
    statstable sstore(get_self(), get_self());

    auto itr = sstore.find(owner);
    //auto gitr = gtable.begin();

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

/*    gtable.modify( *gitr, get_self(), [&]( auto& a ) {
        a.total_claimed += payout;
    });
*/
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
  void configure() {
    
    require_auth(_self);

    globaltable gtable(get_self(), get_self());
    pairstable pairs(get_self(), get_self());

    gtable.emplace(get_self(), [&](auto& o) {
      o.id = 1;
      o.total_datapoints_count = 0;
    });

    pairs.emplace(get_self(), [&](auto& o) {
      o.id = 1;
      o.name = N(eosusd);
    });

    pairs.emplace(get_self(), [&](auto& o) {
      o.id = 2;
      o.name = N(eosbtc);
    });

  }

  //create a new pair request bounty
  //[[eosio::action]]
  void new_bounty(account_name proposer, account_name name, std::string description) {

    require_auth(proposer);

    //add request, proposer pays the RAM for the request + data structure for datapoints & bars. Anyone can then transfer EOS to the smart contract with the name
    //of the bounty in the memo to have the amount added to the bounty.

  }

  //cancel a bounty
  //[[eosio::action]]
  void cancel_bounty(account_name name, std::string reason) {

    //eosio_assert(has_auth(_self) || has_auth(proposer), "missing required authority of contract or proposer");

    //cancel bounty, post reason to chain. Refund accumulated bounty or split between oracles?

  }

  //approve_bounty
  //[[eosio::action]]
  void approve_bounty(account_name name, account_name new_name, symbol_type base_symbol, symbol_type quote_symbol, std::string description) {

    require_auth(_self); //controlled by msig over active key

    //add pair from existing bounty request, move bounty amount to deferred bounty payout, delete bounty request

  }

  //edit a bounty's information
  //[[eosio::action]]
  void edit_bounty_info(account_name name, std::string description) {

    account_name proposer;

    require_auth(proposer);

    //edit bounty description

  }

  //edit pair 
  //[[eosio::action]]
  void edit_pair(account_name name, std::string description) {
    
    require_auth(_self); //controlled by msig over active key
    
    //edit pair description

  }

  //delete pair 
  //[[eosio::action]]
  void delete_pair(account_name name, std::string reason) {
    
    require_auth(_self); //controlled by msig over active key
    
    //delete pair, post reason to chain for reference

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

  void process_donation(account_name scope, asset quantity){

    statstable cstore(get_self(), scope);

    uint64_t size = std::distance(cstore.begin(), cstore.end());

    uint64_t upperbound = std::min(size, paid);

    auto count_index = cstore.get_index<N(count)>();

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

    print("total_datapoints", total_datapoints, "\n");

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

      //avoid leftover rounding by giving to top contributor
      if (i == 1){
        payout = asset(amount, S(4, EOS));
      }
      else {
        payout = asset(uquota, S(4, EOS));
      }

      amount-= uquota;
      
      print("payout", payout, "\n");


      if (scope == get_self()) {

        cstore.modify(*itr, get_self(), [&]( auto& s ) {
          s.balance += payout;
        });

      }
      else {

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

  void process_bounty(account_name pair, asset quantity){

  }

  void transfer(uint64_t sender, uint64_t receiver) {

    print("transfer notifier", "\n");

    auto transfer_data = unpack_action_data<DelphiOracle::st_transfer>();

    print("transfer ", name{transfer_data.from}, " ",  name{transfer_data.to}, " ", transfer_data.quantity, "\n");

    //if incoming transfer
    if (transfer_data.from != _self && transfer_data.to == _self){
      
      //check if memo contains the name of an existing pair

      pairstable pairs(get_self(), get_self());
      bountiestable bounties(get_self(), get_self());

      auto name_index = pairs.get_index<N(name)>();

      print("string_to_name(transfer_data.memo.c_str())", string_to_name(transfer_data.memo.c_str()), "\n");
      print("transfer_data.memo.c_str()", transfer_data.memo.c_str(), "\n");

      auto itr = name_index.find(string_to_name(transfer_data.memo.c_str()));
      auto bitr = bounties.find(string_to_name(transfer_data.memo.c_str()));

      if (itr != name_index.end()) process_donation(itr->name, transfer_data.quantity);
      else if (itr != name_index.end()) process_bounty(itr->name, transfer_data.quantity);
      else process_donation(get_self(), transfer_data.quantity);

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

EOSIO_ABI_EX( DelphiOracle, (write)(clear)(claim)(configure)(transfer))
