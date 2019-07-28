/*

  oracle

  Author: Guillaume "Gnome" Babin-Tremblay - EOS Titan
  
  Website: https://eostitan.com
  Email: guillaume@eostitan.com

  Github: https://github.com/eostitan/delphioracle/
  
  Published under MIT License

TODO

OK - add multi instrument support
OK - open it to BPs rank 50 or better
OK - replace the average with median
OK - do the automated fee distribution based on contribution


*/

#include <eosio.system/eosio.system.hpp>
#include <eosiolib/symbol.hpp>
#include <eosiolib/chain.h>
#include <eosiolib/time.hpp>
#include <eosiolib/eosio.hpp>
#include <math.h>

using namespace eosio;


//Controlling account to be phased out 
//static const name titan_account = name("delphioracle");

//Min value set to 0.01$ , max value set to 10,000$
static const uint64_t val_min = 1;
static const uint64_t val_max = 100000000;

const uint64_t one_minute = 1000000 * 55; //give extra time for cron jobs

static const uint64_t standbys = 50; //allowed standby producers rank cutoff
static const uint64_t paid = 21; //maximum number of oracles getting paid from donations

CONTRACT oracle : public eosio::contract {
 public:
  oracle(name receiver, name code, datastream<const char*> ds) : eosio::contract(receiver, code, ds) {}

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

  //Holds the latest datapoints from qualified oracles
  TABLE datapoints {
    uint64_t id;
    name owner; 
    uint64_t value;
    uint64_t median;
    uint64_t timestamp;

    uint64_t primary_key() const {return id;}
    uint64_t by_timestamp() const {return timestamp;}
    uint64_t by_value() const {return value;}
    EOSLIB_SERIALIZE( datapoints, (id)(owner)(value)(median)(timestamp))
  };

  //Global config
  TABLE global {
    uint64_t id;
    uint64_t total_datapoints_count;
    
    uint64_t primary_key() const {return id;}

  };

  //Holds the count and time of last writes for qualified oracles
  TABLE stats {
    name owner; 
    uint64_t timestamp;
    uint64_t count;
    uint64_t last_claim;
    asset balance;

    uint64_t primary_key() const {return owner.value;}
    uint64_t by_count() const {return count;}

  };

  //Holds the list of pairs
  TABLE pairs {
    
    bool active = false;
    bool bounty_awarded = false;
    bool bounty_edited_by_custodians = false;

    name proposer;
    name aname;

    asset bounty_amount = asset(0, symbol("EOS",4));

    std::vector<name> approving_custodians;
    std::vector<name> approving_oracles;

    symbol base_symbol;
    uint64_t base_type;
    name base_contract;

    symbol quote_symbol;
    uint64_t quote_type;
    name quote_contract;
    
    uint64_t quoted_precision;

    uint64_t primary_key() const {return aname.value;}

  };

  //Quote
  struct quote {
    uint64_t value;
    name pair;

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

  TABLE producer_info {
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
      name  from;
      name  to;
      asset         quantity;
      std::string   memo;
  };

  //Multi index types definition
  typedef eosio::multi_index<name("global"), global> globaltable;

  typedef eosio::multi_index<name("stats"), stats,
      indexed_by<name("count"), const_mem_fun<stats, uint64_t, &stats::by_count>>> statstable;

  typedef eosio::multi_index<name("pairs"), pairs> pairstable;

  typedef eosio::multi_index<name("datapoints"), datapoints,
      indexed_by<name("value"), const_mem_fun<datapoints, uint64_t, &datapoints::by_value>>, 
      indexed_by<name("timestamp"), const_mem_fun<datapoints, uint64_t, &datapoints::by_timestamp>>> datapointstable;

typedef eosio::multi_index<name("producers"), producer_info,
      indexed_by<name("prototalvote"), const_mem_fun<producer_info, double, &producer_info::by_votes>>> producers_table;

  //Check if calling account is a qualified oracle
  bool check_oracle(const name owner){

    producers_table ptable(name("eosio"), name("eosio").value);

    auto p_idx = ptable.get_index<name("prototalvote")>();

    auto p_itr = p_idx.begin();

    uint64_t count = 0;

    while (p_itr != p_idx.end()) {
      print(p_itr->owner, "\n");
      if (p_itr->owner==owner) return true;
      p_itr++;
      count++;
      if (count>standbys) break;
    }

    return true;
  }

  //Ensure account cannot push data more often than every 60 seconds
  void check_last_push(const name owner, const name pair){

    statstable gstore(_self,_self.value);
    statstable store(_self, pair.value);

    auto itr = store.find(owner.value);
    if (itr != store.end()) {

      uint64_t ctime = current_time();
      auto last = store.get(owner.value);

      eosio_assert(last.timestamp + one_minute <= ctime, "can only call every 60 seconds");

      store.modify( itr, _self, [&]( auto& s ) {
        s.timestamp = ctime;
        s.count++;
      });

    } else {

      store.emplace(_self, [&](auto& s) {
        s.owner = owner;
        s.timestamp = current_time();
        s.count = 1;
        s.balance = asset(0, symbol("EOS",4));
        s.last_claim = 0;
      });

    }

    auto gitr = gstore.find(owner.value);
    if (gitr != gstore.end()) {

      uint64_t ctime = current_time();

      gstore.modify( gitr, _self, [&]( auto& s ) {
        s.timestamp = ctime;
        s.count++;
      });

    } else {

      gstore.emplace(_self, [&](auto& s) {
        s.owner = owner;
        s.timestamp = current_time();
        s.count = 1;
       s.balance = asset(0, symbol("EOS",4));
       s.last_claim = 0;
      });

    }

  }

  //Push oracle message on top of queue, pop old elements (older than one minute)
  void update_datapoints(const name owner, const uint64_t value, name pair){

    datapointstable dstore(_self, pair.value);
    uint64_t ctime = current_time();

    auto size = std::distance(dstore.begin(), dstore.end());

    uint64_t median = 0;
    uint64_t primary_key ;

    //Find median
    if (size>0){

      //Calculate new primary key by substracting one from the previous one
      auto latest = dstore.begin();
      primary_key = latest->id - 1;
       
       //Pop old points (older than one minute)
       while (latest != dstore.end()){
        if (latest->timestamp + one_minute < ctime)
          latest = dstore.erase(latest);
        else
          latest++;
       }

        //Insert next datapoint
        auto c_itr = dstore.emplace(_self, [&](auto& s) {
          s.id = primary_key;
          s.owner = owner;
          s.value = value;
          s.timestamp = ctime;
        });

        //Get index sorted by value
        auto value_sorted = dstore.get_index<name("value")>();
        uint64_t mid = (uint64_t)floor(std::distance(value_sorted.begin(), value_sorted.end())/2.0);
        auto itr = value_sorted.begin();
        for (int i=0; i<mid; i++){
          itr++;
        }
        
        median=itr->value;
        //set median
        dstore.modify(c_itr, _self, [&](auto& s) {
          s.median = median;
        });

      }
    else {

      //First data point starts at uint64 max
      primary_key = std::numeric_limits<unsigned long long>::max();
      median = value;

      //Push new point at the end of the queue
      dstore.emplace(_self, [&](auto& s) {
        s.id = primary_key;
        s.owner = owner;
        s.value = value;
        s.median = median;
        s.timestamp = current_time();
      });

    }

    globaltable gtable(_self,_self.value);

    gtable.modify(gtable.begin(), _self, [&](auto& s) {
      s.total_datapoints_count++;
    });

  }

  //Write datapoint
  
  ACTION write(const name owner, const std::vector<quote>& quotes) {
    
    require_auth(owner);
    
    int length = quotes.size();

   // print("length ", length);

    eosio_assert(length>0, "must supply non-empty array of quotes");
    eosio_assert(check_oracle(owner), "account is not an active producer or approved oracle");

    for (int i=0; i<length;i++){
     // print("quote ", i, " ", quotes[i].value, " ",  quotes[i].pair, "\n");
       eosio_assert(quotes[i].value >= val_min && quotes[i].value <= val_max, "value outside of allowed range");
    }

    for (int i=0; i<length;i++){    
      check_last_push(owner, quotes[i].pair);
      update_datapoints(owner, quotes[i].value, quotes[i].pair);
    }

    //TODO: check if symbol exists
    
  }

  //claim rewards
 
  ACTION claim(name owner) {
    
    require_auth(owner);

    statstable gstore(_self,_self.value);

    auto itr = gstore.find(owner.value);

    eosio_assert(itr != gstore.end(), "oracle not found");
    eosio_assert( itr->balance.amount > 0, "no rewards to claim" );

    asset payout = itr->balance;

    //if( existing->quantity.amount == quantity.amount ) {
    //   bt.erase( *existing );
    //} else {
    gstore.modify( *itr, _self, [&]( auto& a ) {
        a.balance = asset(0, symbol("EOS",4));
    });
    //}

    //if quantity symbol == EOS -> token_contract

   // SEND_INLINE_ACTION(token_contract, transfer, {name("eostitancore"),name("active")}, {name("eostitancore"), from, quantity, std::string("")} );
      
    action act(
      permission_level{_self, name("active")},
      name("eosio.token"), name("transfer"),
      std::make_tuple(_self, owner, payout, std::string(""))
    );
    act.send();

  }

  //temp configuration
  
  ACTION configure() {
    
    require_auth(_self);

    globaltable gtable(_self,_self.value);
    pairstable pairs(_self,_self.value);

    gtable.emplace(_self, [&](auto& o) {
      o.id = 1;
      o.total_datapoints_count = 0;
    });

    pairs.emplace(_self, [&](auto& o) {
      o.aname = name("eosusd");
      o.base_symbol = symbol("EOS",4);
      o.base_type = asset_type::eosio_token;
      o.base_contract = name("eosio.token");
      o.quote_symbol = symbol("USD",2);
      o.quote_type = asset_type::fiat;
      o.quote_contract = name("");
      o.quoted_precision = 4;
    });

    pairs.emplace(_self, [&](auto& o) {
      o.aname = name("eosbtc");
      o.base_symbol = symbol("EOS",4);
      o.base_type = asset_type::eosio_token;
      o.base_contract = name("eosio.token");
      o.quote_symbol = symbol("BTC",4);
      o.quote_type = asset_type::cryptocurrency;
      o.quote_contract = name("");
      o.quoted_precision = 6;
    });

    pairs.emplace(_self, [&](auto& o) {
      o.aname = name("iqeos");
      o.base_symbol = symbol("IQ",3);
      o.base_type = asset_type::eosio_token;
      o.base_contract = name("everipediaiq");
      o.quote_symbol = symbol("EOS",4);
      o.quote_type = asset_type::eosio_token;
      o.quote_contract = name("eosio.token");
      o.quoted_precision = 6;
    });

    pairs.emplace(_self, [&](auto& o) {
      o.aname = name("peoseos");
      o.base_symbol = symbol("PEOS",4);
      o.base_type = asset_type::eosio_token;
      o.base_contract = name("thepeostoken");
      o.quote_symbol = symbol("EOS",4);
      o.quote_type = asset_type::eosio_token;
      o.quote_contract = name("eosio.token");
      o.quoted_precision = 6;
    });

    pairs.emplace(_self, [&](auto& o) {
      o.aname = name("diceeos");
      o.base_symbol = symbol("DICE",4);
      o.base_type = asset_type::eosio_token;
      o.base_contract = name("betdicetoken");
      o.quote_symbol = symbol("EOS",4);
      o.quote_type = asset_type::eosio_token;
      o.quote_contract = name("eosio.token");
      o.quoted_precision = 6;
    });

    pairs.emplace(_self, [&](auto& o) {
      o.aname = name("tpteos");
      o.base_symbol = symbol("TPT",4);
      o.base_type = asset_type::eosio_token;
      o.base_contract = name("eosiotptoken");
      o.quote_symbol = symbol("EOS",4);
      o.quote_type = asset_type::eosio_token;
      o.quote_contract = name("eosio.token");
      o.quoted_precision = 6;
    });

    pairs.emplace(_self, [&](auto& o) {
      o.aname = name("vigeos");
      o.base_symbol = symbol("VIG",4);
      o.base_type = asset_type::eosio_token;
      o.base_contract = name("vig111111111");
      o.quote_symbol = symbol("EOS",4);
      o.quote_type = asset_type::eosio_token;
      o.quote_contract = name("eosio.token");
      o.quoted_precision = 6;
    });

  }

  //Clear all data
  ACTION clear(name pair) {
    require_auth(_self);

    globaltable gtable(_self,_self.value);
    statstable gstore(_self,_self.value);
    statstable lstore(_self, pair.value);
    datapointstable estore(_self,  pair.value);
    pairstable pairs(_self,_self.value);
    
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


  ACTION transfer(uint64_t sender, uint64_t receiver) {

    print("transfer notifier", "\n");

    auto transfer_data = unpack_action_data<oracle::st_transfer>();

    print("transfer ", name{transfer_data.from}, " ",  name{transfer_data.to}, " ", transfer_data.quantity, "\n");

    //if incoming transfer
    if (transfer_data.from != _self && transfer_data.to == _self){
      
      globaltable global(_self,_self.value);
      statstable gstore(_self,_self.value);

      uint64_t size = std::distance(gstore.begin(), gstore.end());

      uint64_t upperbound = std::min(size, paid);

      auto count_index = gstore.get_index<name("count")>();

      auto itr = count_index.begin();
      auto gitr = global.begin();

      uint64_t total_datapoints = 0; //gitr->total_datapoints_count;

      print("upperbound", upperbound, "\n");
      //print("itr->owner", itr->owner, "\n");

      //Move pointer to upperbound, counting total number of datapoints for oracles elligible for payout
      for (uint64_t i=1;i<=upperbound;i++){
        total_datapoints+=itr->count;
        

        if (i<upperbound ){
          itr++;
          print("increment 1", "\n");

        } 

      }

      print("total_datapoints", total_datapoints, "\n");

      uint64_t amount = transfer_data.quantity.amount;

      //Move pointer back to 0, calculating prorated contribution of oracle and allocating proportion of donation
      for (uint64_t i=upperbound;i>=1;i--){

        uint64_t datapoints = itr->count;

        double percent = ((double)datapoints / (double)total_datapoints) ;
        uint64_t uquota = (uint64_t)(percent * (double)transfer_data.quantity.amount) ;


        print("itr->owner", itr->owner, "\n");
        print("datapoints", datapoints, "\n");
        print("percent", percent, "\n");
        print("uquota", uquota, "\n");

        asset payout;

        //avoid leftover rounding by giving to top contributor
        if (i == 1){
          payout = asset(amount, symbol("EOS",4));
        }
        else {
          payout = asset(uquota, symbol("EOS",4));
        }

        amount-= uquota;
        
        print("payout", payout, "\n");

        gstore.modify(*itr, _self, [&]( auto& s ) {
          s.balance += payout;
        });
        
        if (i>1 ){
          itr--;
          print("decrement 1", "\n");

        } 
      }


    }

  }

};



extern "C" {
    void apply(uint64_t receiver, uint64_t code, uint64_t action) {
        if(code==receiver)
        {
            switch(action)
            {
                EOSIO_DISPATCH_HELPER(oracle, (write)(clear)(claim)(configure)(transfer))
            }
        }
        else if(code=="eosio.token"_n.value && action=="transfer"_n.value) {
            execute_action( name(receiver), name(code), &oracle::transfer);
        }
    }
}