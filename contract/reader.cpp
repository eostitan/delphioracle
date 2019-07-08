
#include <eosio.system/eosio.system.hpp>
#include <eosiolib/symbol.hpp>
#include <eosiolib/chain.h>
#include <eosiolib/time.hpp>
#include <eosiolib/eosio.hpp>
#include <math.h>

using namespace eosio;

const uint64_t one_min = 60 * 1000000;

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

  uint64_t get_last_price(account_name pair){

    datapointstable dstore(N(delphioracle), pair);

    eosio_assert(dstore.begin() != dstore.end(), "no datapoints");

    auto newest = dstore.begin();

    return newest->value;

  }

  uint64_t get_one_min_median(account_name pair){

    datapointstable dstore(N(delphioracle), pair);

    eosio_assert(dstore.begin() != dstore.end(), "no datapoints");

    uint64_t ctime = current_time();
    uint64_t one_min_ago = ctime - one_min;

    auto t_idx = dstore.get_index<N(timestamp)>();

    auto b_itr = t_idx.lower_bound(one_min_ago);

    uint64_t size = std::distance(b_itr, t_idx.end());
    uint64_t mid = (uint64_t)floor(size/2.0);

    for (int i=0; i<mid; i++){
      b_itr++;
    }
    
    return b_itr->value;
    
  }

  void read(account_name pair){
    
    uint64_t one_min_median = get_one_min_median(pair);

    print("one min median:", one_min_median, "\n");

  }

};

//EOSIO_ABI(DelphiOracle, (write)(clear)(configure)(transfer))

EOSIO_ABI( Reader, (read))