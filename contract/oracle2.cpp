/*

  DelphiOracle

  Author: Guillaume "Gnome" Babin-Tremblay - EOS Titan
  
  Website: https://eostitan.com
  Email: guillaume@eostitan.com

  Github: https://github.com/eostitan/delphioracle/
  
  Published under MIT License

*/

#include <eosiolib/eosio.hpp>
#include <eosiolib/fixedpoint.hpp>
#include <eosiolib/chain.h>

using namespace eosio;

//Controlling account
static const account_name titan_account = N(titanclearer);

//Approved oracles
//static const account_name oracles[] = {N(titanclearer), N(eostitantest), N(mydemolisher), N(acryptotitan), N(delphioracle), N(eosmetaliock), N(eosdacbptest) };

//Number of datapoints to hold
static const uint64_t datapoints_count = 21;

//Min value set to 0.01$ , max value set to 10,000$
static const uint64_t val_min = 100;
static const uint64_t val_max = 100000000;

const uint64_t one_minute = 1000000 * 60;

class DelphiOracle : public eosio::contract {
 public:
  DelphiOracle(account_name self) : eosio::contract(self){}

  //Types

  //Holds the last datapoints_count datapoints from qualified oracles
  struct [[eosio::table]] eosusd {
    uint64_t id;
    account_name owner; 
    uint64_t value;
    uint64_t accumulator;
    uint64_t average;
    uint64_t timestamp;

    uint64_t primary_key() const {return id;}
    uint64_t by_timestamp() const {return timestamp;}
    uint64_t by_value() const {return value;}

    EOSLIB_SERIALIZE( eosusd, (id)(owner)(value)(accumulator)(average)(timestamp))

  };

  //Holds the time of last eosusd writes for qualified oracles
  struct [[eosio::table]] eosusdlast {
    account_name owner; 
    uint64_t timestamp;

    account_name primary_key() const {return owner;}

  };

  //Holds the list of oracles
  struct [[eosio::table]] oracles {
    account_name owner;

    account_name primary_key() const {return owner;}

  };

  //Multi index types definition
  typedef eosio::multi_index<N(eosusdlast), eosusdlast> lastusdtable;
  typedef eosio::multi_index<N(oracles), oracles> oraclestable;
  typedef eosio::multi_index<N(eosusd), eosusd, 
      indexed_by<N(timestamp), const_mem_fun<eosusd, uint64_t, &eosusd::by_timestamp>>,
      indexed_by<N(value), const_mem_fun<eosusd, uint64_t, &eosusd::by_value>>> usdtable;

  //Check if calling account is a qualified oracle
  bool check_oracle(const account_name owner){

    oraclestable oracles(get_self(), get_self());

    auto itr = oracles.begin();
    while (itr != oracles.end()) {
        if (itr->owner == owner) return true;
        else itr++;
    }


/*    //Account is oracle if on qualified oracle list
    for(account_name oracle : oracles){
      if (oracle == owner) return true;
    }*/

    account_name producers[21] = { 0 };
    uint32_t bytes_populated = get_active_producers(producers, sizeof(account_name)*21);
    
    //Account is oracle if account is an active producer
    for(int i = 0; i < 21; i=i+1 ) {
      if (producers[i] == owner){
        return true;
      }
    }

    return false;

  }

  //Ensure account cannot push data more often than every 60 seconds
  void check_last_push(const account_name owner){

    lastusdtable store(get_self(), get_self());

    auto itr = store.find(owner);
    if (itr != store.end()) {

      uint64_t ctime = current_time();
      auto last = store.get(owner);

      eosio_assert(last.timestamp + one_minute <= ctime, "can only call every 60 seconds");

      store.modify( itr, get_self(), [&]( auto& s ) {
        s.timestamp = ctime;
      });

    } else {

      store.emplace(get_self(), [&](auto& s) {
        s.owner = owner;
        s.timestamp = current_time();
      });

    }

  }

  //Push oracle message on top of queue, pop oldest element if queue size is larger than X
  void update_eosusd_oracle(const account_name owner, const uint64_t value){

    usdtable usdstore(get_self(), get_self());

    auto size = std::distance(usdstore.begin(), usdstore.end());

    uint64_t avg;
    uint64_t accumulated;
    uint64_t primary_key ;

    //Calculate approximative rolling average
    if (size>0){

      auto latest = usdstore.begin();
      auto oldest = usdstore.end();
      oldest--;

      uint64_t p_accumulated = latest->accumulator;

      primary_key = latest->id - 1;

      accumulated = latest->accumulator+value;

      //Pop oldest point
      if (size+1>datapoints_count){

        accumulated-=oldest->value;
        usdstore.erase(oldest);

      }

      avg = (p_accumulated + value) / (size + 1);


    }
    else {
      primary_key = std::numeric_limits<unsigned long long>::max();
      accumulated = value;
      avg = value;
    }

    //Push new point at the end of the queue
    usdstore.emplace(get_self(), [&](auto& s) {
      s.id = primary_key;
      s.owner = owner;
      s.value = value;
      s.accumulator = accumulated;
      s.average = avg;
      s.timestamp = current_time();
    });


  }

  //Write datapoint
  [[eosio::action]]
  void write(const account_name owner, const uint64_t value) {
    
    require_auth(owner);

    eosio_assert(value >= val_min && value <= val_max, "value outside of allowed range");

    eosio_assert(check_oracle(owner), "account is not an active producer or approved oracle");
    check_last_push(owner);
    update_eosusd_oracle(owner, value);
    
  }

  //Update oracles list
  [[eosio::action]]
  void setoracles(const std::vector<account_name>& oracleslist) {
    
    require_auth(titan_account);

    oraclestable oracles(get_self(), get_self());

    while (oracles.begin() != oracles.end()) {
        auto itr = oracles.end();
        itr--;
        oracles.erase(itr);
    }

    for(const account_name& oracle : oracleslist){
      oracles.emplace(get_self(), [&](auto& o) {
        o.owner = oracle;
      });
    }

  }

  //Clear all data
  [[eosio::action]]
  void clear() {
    require_auth(titan_account);
    lastusdtable lstore(get_self(), get_self());
    usdtable estore(get_self(), get_self());
    oraclestable oracles(get_self(), get_self());
    
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
    
    while (oracles.begin() != oracles.end()) {
        auto itr = oracles.end();
        itr--;
        oracles.erase(itr);
    }

  }

};

EOSIO_ABI(DelphiOracle, (write)(setoracles)(clear))
