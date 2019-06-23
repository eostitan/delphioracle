
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

  uint64_t get_last_price(account_name pair){

    datapointstable dstore(N(delphioracle), pair);

    eosio_assert(dstore.begin() != dstore.end(), "no datapoints");

    auto newest = dstore.begin();

    return newest->value;

  }

  void read(account_name pair){
    
    uint64_t lastprice = get_last_price(pair);

    print("last price:", lastprice, "\n");

  }

};

//EOSIO_ABI(DelphiOracle, (write)(clear)(configure)(transfer))

EOSIO_ABI( Reader, (read))