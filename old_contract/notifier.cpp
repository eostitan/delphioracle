#include <eosiolib/eosio.hpp>

using namespace eosio;

class Notifier : public eosio::contract {
 public:
  Notifier(account_name self) : eosio::contract(self) {}

  void write(uint64_t sender, uint64_t receiver) {

    //print("delphioracle write handler, do something here", "\n");

  }

};

#define EOSIO_ABI_EX( TYPE, MEMBERS ) \
extern "C" { \
   void apply( uint64_t receiver, uint64_t code, uint64_t action ) { \
      auto self = receiver; \
      if( code == self || code == N(delphioracle)) { \
         if( action == N(write)){ \
          eosio_assert( code == N(delphioracle), "Handles write only"); \
         } \
         TYPE thiscontract( self ); \
         switch( action ) { \
            EOSIO_API( TYPE, MEMBERS ) \
         } \
         /* does not allow destructor of thiscontract to run: eosio_exit(0); */ \
      } \
   } \
}

EOSIO_ABI_EX( Notifier, (write))