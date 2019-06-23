
#include <eosio.system/eosio.system.hpp>
#include <eosiolib/symbol.hpp>
#include <eosiolib/chain.h>
#include <eosiolib/time.hpp>
#include <eosiolib/eosio.hpp>
#include <math.h>

using namespace eosio;

class Reader : public eosio::contract {
 public:
  Reader(account_name self) : eosio::contract(self) {}

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

  typedef eosio::multi_index<N(datapoints), datapoints,
      indexed_by<N(value), const_mem_fun<datapoints, uint64_t, &datapoints::by_value>>, 
      indexed_by<N(timestamp), const_mem_fun<datapoints, uint64_t, &datapoints::by_timestamp>>> datapointstable;

  void read(){
    
/*
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
    });*/

  }

}

//EOSIO_ABI(DelphiOracle, (write)(clear)(configure)(transfer))

EOSIO_ABI( Reader, (read))