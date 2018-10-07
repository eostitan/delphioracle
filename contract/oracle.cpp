/*

  DelphiOracle

  Author: Guillaume "Gnome" Babin-Tremblay - EOS Titan
  
  Website: https://eostitan.com
  Email: guillaume@eostitan.com

  Published under MIT License

*/

#include <eosiolib/eosio.hpp>
#include <eosiolib/print.hpp>
#include <eosiolib/fixedpoint.hpp>
#include <eosiolib/chain.h>

using namespace eosio;

//Controlling account
static const account_name titan_account = N(titanclearer);

//Approved oracles
static const account_name oracles[] = {N(titanclearer), N(eostitantest), N(mydemolisher), N(acryptotitan)};

//Number of datapoints to hold
static const uint64_t X = 9;

const uint64_t one_minute = 1000000 * 60;

class DelphiOracle : public eosio::contract {
 public:
  DelphiOracle(account_name self) : eosio::contract(self){}

  //Types

  //Holds the last X datapoints from qualified oracles
  struct [[eosio::table]] eosusdstore {
    uint64_t id;
    account_name owner; 
    uint64_t value;
    uint64_t accumulator;
    uint64_t average;
    uint64_t timestamp;

    uint64_t primary_key() const {return id;}
    uint64_t by_timestamp() const {return timestamp;}

    EOSLIB_SERIALIZE( eosusdstore, (id)(owner)(value)(accumulator)(average)(timestamp))

  };

  //Holds the time of last eosusd writes for qualified oracles
  struct [[eosio::table]] eosusdlast {
    account_name owner; 
    uint64_t timestamp;

    account_name primary_key() const {return owner;}

  };

  //Multi index types definition
  typedef eosio::multi_index<N(eosusdlast), eosusdlast> lastusdtable;
  typedef eosio::multi_index<N(eosusdstore), eosusdstore, indexed_by<N(timestamp), const_mem_fun<eosusdstore, uint64_t, &eosusdstore::by_timestamp>>> usdtable;

  //Check if calling account is a qualified oracle
  bool check_oracle(const account_name producers[] , const account_name owner){

    //Account is oracle if on qualified oracle list
    for(account_name oracle : oracles){
      if (oracle == owner) return true;
    }

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

    //print("last not found, inserting", "\n");
      store.emplace(get_self(), [&](auto& s) {
        s.owner = owner;
        s.timestamp = current_time();
      });

    }

  }

  //Push oracle message on top of queue, pop oldest element if queue size is larger than X
  void update_eosusd_oracle(const account_name owner, const uint64_t value){

    //print("update_eosusd_oracle", "\n");

    usdtable usdstore(get_self(), get_self());

    auto size = std::distance(usdstore.begin(), usdstore.end());

    print("size ", size, "\n");
    //print("value ", value, "\n");

    uint64_t avg;
    uint64_t accumulated;

    //Calculate approximative rolling average
    if (size>0){


      auto last = usdstore.end();
      last--;

      //print("id: ", last->id, "\n");
      //print("owner: ", last->owner, "\n");
      //print("value: ", last->value, "\n");
      //print("average: ", last->average, "\n");
      //print("last timestamp: ", last->timestamp, "\n");

      avg = (last->accumulator + value) / (size + 1);

      accumulated = last->accumulator+value;

    }
    else {
      accumulated = value;
      avg = value;
    }

    //Pop oldest point
    if (size>X-1){
      auto first = usdstore.begin();
      usdstore.erase(first);

      accumulated-=first->value;

      print("pop", "\n");
    }

    //Push new point at the end of the queue
    usdstore.emplace(get_self(), [&](auto& s) {
      s.id = usdstore.available_primary_key();
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

    account_name producers[21] = { 0 };
    uint32_t bytes_populated = get_active_producers(producers, sizeof(account_name)*21);

    if (check_oracle(producers, owner)){
      //print("account is oracle ", name{owner}, "\n");
      check_last_push(owner);
      update_eosusd_oracle(owner, value);
      //print("done.");
    }

  }

  //Clear all data
  [[eosio::action]]
  void clear() {
    require_auth(titan_account);
    lastusdtable lstore(get_self(), get_self());
    usdtable estore(get_self(), get_self());
    
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
    
    //print("cleared lastusdtable and usdtable", "\n");

  }

};

EOSIO_ABI(DelphiOracle, (write)(clear))
