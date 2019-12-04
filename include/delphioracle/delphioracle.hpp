/*

  delphioracle

  Authors: 
    Guillaume "Gnome" Babin-Tremblay - EOS Titan 
    Andrew "netuoso" Chaney - EOS Titan

  Website: https://eostitan.com
  Email: guillaume@eostitan.com

  Github: https://github.com/eostitan/delphioracle/

  Published under MIT License

*/

#include <eosio/eosio.hpp>
#include <eosio/asset.hpp>
#include <eosio/crypto.hpp>
#include <eosio/system.hpp>
#include <math.h>

using namespace eosio;

// Setup per-chain system constants
#ifdef WAX
    const std::string SYSTEM_SYMBOL = "WAX";
    const uint64_t SYSTEM_PRECISION = 8;
    static const asset one_larimer = asset(1000000, symbol(SYSTEM_SYMBOL, SYSTEM_PRECISION));
#else
    const std::string SYSTEM_SYMBOL = "EOS";
    const uint64_t SYSTEM_PRECISION = 4;
    static const asset one_larimer = asset(1, symbol(SYSTEM_SYMBOL, SYSTEM_PRECISION));
#endif

static const std::string system_str("system");

const checksum256 NULL_HASH;
const eosio::time_point NULL_TIME_POINT = eosio::time_point(eosio::microseconds(0));

CONTRACT delphioracle : public eosio::contract {
 public:
 	using contract::contract;

  //Types

  enum e_asset_type: uint16_t {
      fiat=1,
      cryptocurrency=2,
      erc20_token=3,
      eosio_token=4,
      equity=5,
      derivative=6,
      other=7
  };

  typedef uint16_t asset_type;

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

  struct pairinput {
    name name;
    symbol base_symbol;
    asset_type base_type;
    eosio::name base_contract;
    symbol quote_symbol;
    asset_type quote_type;
    eosio::name quote_contract;
    uint64_t quoted_precision;
  };


  //Quote
  struct quote {
    uint64_t value;
    name pair;
  };

  //eosmechanics::cpu
  struct event {
    uint64_t value;
    name instrument;
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
      name  from;
      name  to;
      asset         quantity;
      std::string   memo;
  };


  //Global config
  TABLE global {

    //variables
    uint64_t id;
    uint64_t total_datapoints_count;
    asset total_claimed = asset(0, symbol(SYSTEM_SYMBOL, SYSTEM_PRECISION));

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

  TABLE oglobal {
    uint64_t id;
    uint64_t total_datapoints_count;
    //asset total_claimed;

    uint64_t primary_key() const {return id;}
  };

  //Holds the last datapoints_count datapoints from qualified oracles
  TABLE datapoints {
    uint64_t id;
    name owner; 
    uint64_t value;
    uint64_t median;
    time_point timestamp;

    uint64_t primary_key() const {return id;}
    uint64_t by_timestamp() const {return timestamp.elapsed.to_seconds();}
    uint64_t by_value() const {return value;}

  };

  //Holds the last hashes from qualified oracles
  TABLE hashes {
    uint64_t id;
    name owner;
    checksum256 multiparty;
    checksum256 hash;
    std::string reveal;
    time_point timestamp;

    uint64_t primary_key() const {return id;}
    uint64_t by_timestamp() const {return timestamp.elapsed.to_seconds();}
    uint64_t by_owner() const {return owner.value;}
    checksum256 by_hash() const {return hash;}
  };

  //Holds the count and time of last writes for qualified oracles
  TABLE stats {
    name owner; 
    time_point timestamp;
    uint64_t count;
    time_point last_claim;
    asset balance;

    uint64_t primary_key() const {return owner.value;}
    uint64_t by_count() const {return -count;}

  };

  //Holds aggregated datapoints
  TABLE bars {
    uint64_t id;

    uint64_t high;
    uint64_t low;
    uint64_t median;
    time_point timestamp;

    uint64_t primary_key() const {return id;}
    uint64_t by_timestamp() const {return timestamp.elapsed.to_seconds();}

  };
/*
  //Holds bounties information
  TABLE bounties {

    name name;
    std::string description;
    asset bounty;

    uint64_t primary_key() const {return name.value;}

  };*/

  //Holds rewards information
  TABLE donations {

    uint64_t id;

    name donator;
    name pair;
    time_point timestamp;
    asset amount;

    uint64_t primary_key() const {return id;}
    uint64_t by_donator() const {return donator.value;}

  };

  //Holds users information
  TABLE users {

    name name;
    asset contribution;
    uint64_t score;
    time_point creation_timestamp;

    uint64_t primary_key() const {return name.value;}
    uint64_t by_score() const {return score;}

  };

  TABLE abusers {

    name name;
    uint64_t votes;

    uint64_t primary_key() const {return name.value;}
    uint64_t by_votes() const {return votes;}

  };

  //Holds custodians information
  TABLE custodians {

    name name;

    uint64_t primary_key() const {return name.value;}

  };

  //Holds the list of pairs
  TABLE pairs {

    bool active = false;
    bool bounty_awarded = false;
    bool bounty_edited_by_custodians = false;

    name proposer;
    name name;

    asset bounty_amount = asset(0, symbol(SYSTEM_SYMBOL, SYSTEM_PRECISION));

    std::vector<eosio::name> approving_custodians;
    std::vector<eosio::name> approving_oracles;

    symbol base_symbol;
    asset_type base_type;
    eosio::name base_contract;

    symbol quote_symbol;
    asset_type quote_type;
    eosio::name quote_contract;
    
    uint64_t quoted_precision;

    uint64_t primary_key() const {return name.value;}

  };

  //Holds the list of pairs
  TABLE networks {

    name name;

    uint64_t primary_key() const {return name.value;}

  };

   TABLE voter_info {
      name                owner;     /// the voter
      name                proxy;     /// the proxy set by the voter, if any
      std::vector<name>   producers; /// the producers approved by this voter if no proxy set
      int64_t             staked = 0;

      /**
       *  Every time a vote is cast we must first "undo" the last vote weight, before casting the
       *  new vote weight.  Vote weight is calculated as:
       *
       *  stated.amount * 2 ^ ( weeks_since_launch/weeks_per_year)
       */
      double              last_vote_weight = 0; /// the vote weight cast the last time the vote was updated

      /**
       * Total vote weight delegated to this voter.
       */
      double              proxied_vote_weight= 0; /// the total vote weight delegated to this voter as a proxy
      bool                is_proxy = 0; /// whether the voter is a proxy for others


      uint32_t            flags1 = 0;
      uint32_t            reserved2 = 0;
      eosio::asset        reserved3;

      uint64_t primary_key()const { return owner.value; }

      enum class flags1_fields : uint32_t {
         ram_managed = 1,
         net_managed = 2,
         cpu_managed = 4
      };

      // explicit serialization macro is not necessary, used here only to improve compilation time
      EOSLIB_SERIALIZE( voter_info, (owner)(proxy)(producers)(staked)(last_vote_weight)(proxied_vote_weight)(is_proxy)(flags1)(reserved2)(reserved3) )
   };

  //Multi index types definition
  typedef eosio::multi_index<"global"_n, global> globaltable;
  typedef eosio::multi_index<"global"_n, oglobal> oglobaltable;

  typedef eosio::multi_index<"custodians"_n, custodians> custodianstable;

  typedef eosio::multi_index<"stats"_n, stats,
      indexed_by<"count"_n, const_mem_fun<stats, uint64_t, &stats::by_count>>> statstable;

  typedef eosio::multi_index<"pairs"_n, pairs> pairstable;
  typedef eosio::multi_index<"npairs"_n, pairs> npairstable;

  typedef eosio::multi_index<"datapoints"_n, datapoints,
      indexed_by<"value"_n, const_mem_fun<datapoints, uint64_t, &datapoints::by_value>>, 
      indexed_by<"timestamp"_n, const_mem_fun<datapoints, uint64_t, &datapoints::by_timestamp>>> datapointstable;

  typedef eosio::multi_index<"hashes"_n, hashes,
      indexed_by<"timestamp"_n, const_mem_fun<hashes, uint64_t, &hashes::by_timestamp>>,
      indexed_by<"owner"_n, const_mem_fun<hashes, uint64_t, &hashes::by_owner>>,
      indexed_by<"hash"_n, const_mem_fun<hashes, checksum256, &hashes::by_hash>>> hashestable;

  typedef eosio::multi_index<"voters"_n, voter_info,
      indexed_by<"voter"_n, const_mem_fun<voter_info, uint64_t, &voter_info::primary_key>>> voters_table;

  typedef eosio::multi_index<"producers"_n, producer_info,
      indexed_by<"prototalvote"_n, const_mem_fun<producer_info, double, &producer_info::by_votes>>> producers_table;

  typedef eosio::multi_index<"donations"_n, donations,
      indexed_by<"donator"_n, const_mem_fun<donations, uint64_t, &donations::by_donator>>> donationstable;

  typedef eosio::multi_index<"users"_n, users,
      indexed_by<"score"_n, const_mem_fun<users, uint64_t, &users::by_score>>> userstable;

  typedef eosio::multi_index<"abusers"_n, abusers,
      indexed_by<"votes"_n, const_mem_fun<abusers, uint64_t, &abusers::by_votes>>> abuserstable;

  //typedef eosio::multi_index<"bounties"_n, bounties> bountiestable;

  std::string to_hex( const char* d, uint32_t s ) 
  {
      std::string r;
      const char* to_hex="0123456789abcdef";
      uint8_t* c = (uint8_t*)d;
      for( uint32_t i = 0; i < s; ++i )
          (r += to_hex[(c[i]>>4)]) += to_hex[(c[i] &0x0f)];
      return r;
  }

  //Write datapoint
  ACTION write(const name owner, const std::vector<quote>& quotes);
  ACTION writehash(const name owner, const checksum256 hash, const std::string reveal);
  ACTION forfeithash(const name owner);
  ACTION claim(name owner);
  ACTION configure(globalinput g);
  ACTION newbounty(name proposer, pairinput pair);
  ACTION cancelbounty(name name, std::string reason);
  ACTION votebounty(name owner, name bounty);
  ACTION unvotebounty(name owner, name bounty);
  ACTION editbounty(name name, pairinput pair);
  ACTION editpair(pairs pair);
  ACTION deletepair(name name);
  ACTION addcustodian(name name);
  ACTION delcustodian(name name);
  ACTION reguser(name owner);
  ACTION clear(name pair);
  ACTION updateusers();
  ACTION voteabuser(name owner, name abuser);
  ACTION migratedata();

  [[eosio::on_notify("eosio.token::transfer")]]
  void transfer(uint64_t sender, uint64_t receiver) {

    print("transfer notifier", "\n");

    auto transfer_data = unpack_action_data<delphioracle::st_transfer>();

    print("transfer ", name{transfer_data.from}, " ",  name{transfer_data.to}, " ", transfer_data.quantity, "\n");

    //if incoming transfer
    if (transfer_data.from != _self && transfer_data.to == _self){

      //check if memo contains the name of an existing pair

      pairstable pairs(_self, _self.value);
      //bountiestable bounties(_self, _self.value);

      //auto name_index = pairs.get_index<"name"_n>();

      // print("name(transfer_data.memo)", name(transfer_data.memo), "\n");
      // print("transfer_data.memo", transfer_data.memo, "\n");

      if (transfer_data.memo == system_str ) return; //transfer to system account

      auto itr = pairs.find(name(transfer_data.memo).value);

      //auto bitr = bounties.find(name(transfer_data.memo).value;

      if (itr != pairs.end() && itr->bounty_awarded == true ) process_donation(transfer_data.from, itr->name, transfer_data.quantity);
      else if (itr != pairs.end() && itr->bounty_awarded == false) process_bounty(transfer_data.from, itr->name, transfer_data.quantity);
      else process_donation(transfer_data.from, _self, transfer_data.quantity);

    }

  }

  using write_action = action_wrapper<"write"_n, &delphioracle::write>;
  using writehash_action = action_wrapper<"writehash"_n, &delphioracle::writehash>;
  using claim_action = action_wrapper<"claim"_n, &delphioracle::claim>;
  using configure_action = action_wrapper<"configure"_n, &delphioracle::configure>;
  using newbounty_action = action_wrapper<"newbounty"_n, &delphioracle::newbounty>;
  using cancelbounty_action = action_wrapper<"cancelbounty"_n, &delphioracle::cancelbounty>;
  using votebounty_action = action_wrapper<"votebounty"_n, &delphioracle::votebounty>;
  using unvotebounty_action = action_wrapper<"unvotebounty"_n, &delphioracle::unvotebounty>;
  using editbounty_action = action_wrapper<"editbounty"_n, &delphioracle::editbounty>;
  using editpair_action = action_wrapper<"editpair"_n, &delphioracle::editpair>;
  using deletepair_action = action_wrapper<"deletepair"_n, &delphioracle::deletepair>;
  using addcustodian_action = action_wrapper<"addcustodian"_n, &delphioracle::addcustodian>;
  using delcustodian_action = action_wrapper<"delcustodian"_n, &delphioracle::delcustodian>;
  using reguser_action = action_wrapper<"reguser"_n, &delphioracle::reguser>;
  using clear_action = action_wrapper<"clear"_n, &delphioracle::clear>;
  using voteabuser_action = action_wrapper<"voteabuser"_n, &delphioracle::voteabuser>;
  using updateusers_action = action_wrapper<"updateusers"_n, &delphioracle::updateusers>;
  using migratedata_action = action_wrapper<"migratedata"_n, &delphioracle::migratedata>;
  using transfer_action = action_wrapper<name("transfer"), &delphioracle::transfer>;

private:

  static uint128_t composite_key(const uint64_t &x, const uint64_t &y) {
      return (uint128_t{x} << 64) | y;
  }

  //Check if calling account is a qualified oracle
  bool check_oracle(const name owner){

    globaltable gtable(_self, _self.value);
    producers_table ptable("eosio"_n, name("eosio").value);

    auto gitr = gtable.begin();

    auto p_idx = ptable.get_index<"prototalvote"_n>();

    auto p_itr = p_idx.begin();

    uint64_t count = 0;

    while (p_itr != p_idx.end()) {
      if (p_itr->owner==owner) return true;
      p_itr++;
      count++;
      if (count>gitr->minimum_rank) break;
    }

    return false;
  }

  checksum256 get_multiparty_hash(const name owner, const std::string reveal){

    hashestable htable(_self, _self.value);

    auto itr = htable.rbegin();

    while (itr != htable.rend()){
      if(itr->owner != owner && itr->reveal!=""){
        

        std::string prev_multiparty = to_hex((char*)itr->multiparty.extract_as_byte_array().data(), sizeof(itr->multiparty));

        print("reveal:", reveal, "\n");
        print("prev_multiparty:", prev_multiparty, "\n");

        std::string to_hash(prev_multiparty);

        to_hash+= reveal;

        print("to_hash:", to_hash, "\n");

        checksum256 h = sha256(to_hash.c_str(), to_hash.length());

        print("h:", h, "\n");

        return h;
      }
      itr++;
    }
    
    return NULL_HASH;

  }

  //Check if calling account is can vote on bounties
  bool check_approver(const name owner){

    globaltable gtable(_self, _self.value);
    statstable stats(_self, _self.value);

    auto gitr = gtable.begin();
    auto itr = stats.find(owner.value);

    if (gitr->approver_threshold == 0) return true;
    else if (itr != stats.end() && itr->count >= gitr->approver_threshold) return true;
    else return false;

  }

  bool check_user(const name owner){

    userstable utable(_self, _self.value);

    auto user = utable.find(owner.value);

    if (user != utable.end()) return true;
    else return false;

  }

  //Ensure account cannot push data more often than every 60 seconds
  void check_last_push(const name owner, const name pair){

    globaltable gtable(_self, _self.value);
    statstable gstore(_self, _self.value);
    statstable store(_self, pair.value);

    auto gitr = gtable.begin();
    auto itr = store.find(owner.value);

    if (itr != store.end()) {

      time_point ctime = current_time_point();
      auto last = store.get(owner.value);

      time_point next_push = eosio::time_point(last.timestamp.elapsed + eosio::microseconds(gitr->write_cooldown));

      check(ctime>=next_push, "can only call every 60 seconds");

      store.modify( itr, _self, [&]( auto& s ) {
        s.timestamp = ctime;
        s.count++;
      });

    } else {

      store.emplace(_self, [&](auto& s) {
        s.owner = owner;
        s.timestamp = current_time_point();
        s.count = 1;
        s.balance = asset(0, symbol(SYSTEM_SYMBOL, SYSTEM_PRECISION));
        s.last_claim = NULL_TIME_POINT;
      });

    }

    auto gsitr = gstore.find(owner.value);
    if (gsitr != gstore.end()) {

      time_point ctime = current_time_point();

      gstore.modify( gsitr, _self, [&]( auto& s ) {
        s.timestamp = ctime;
        s.count++;
      });

    } else {

      gstore.emplace(_self, [&](auto& s) {
        s.owner = owner;
        s.timestamp = current_time_point();
        s.count = 1;
        s.balance = asset(0, symbol(SYSTEM_SYMBOL, SYSTEM_PRECISION));
        s.last_claim = NULL_TIME_POINT;
      });

    }

  }

  void update_votes(){

    print("voting for bps:", "\n");

    std::vector<eosio::name> bps;

    statstable gstore(_self, _self.value);

    auto sorted_idx = gstore.get_index<"count"_n>();
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
      permission_level{_self, "active"_n},
      "eosio"_n, "voteproducer"_n,
      std::make_tuple(_self, ""_n, bps)
    );
    act.send();
 
  }

  //Push oracle message on top of queue, pop oldest element if queue size is larger than datapoints_count
  void update_datapoints(const name owner, const uint64_t value, pairstable::const_iterator pair_itr){

    globaltable gtable(_self, _self.value);
    datapointstable dstore(_self, pair_itr->name.value);

    uint64_t median = 0;
    //uint64_t primary_key ;

    //Calculate new primary key by substracting one from the previous one
 /*   auto latest = dstore.begin();
    primary_key = latest->id - 1;*/

    auto t_idx = dstore.get_index<"timestamp"_n>();

    auto oldest = t_idx.begin();

/*

  auto idx = hashtable.get_index<"producer"_n>();
    auto itr = idx.find(producer);
    if (itr != idx.end()) {
      idx.modify(itr, producer, [&](auto& prod_hash) {
        prod_hash.hash = hash;
      });
    } else {*/


    //Pop oldest point
    //t_idx.erase(oldest);

    //Insert next datapoint
/*    auto c_itr = dstore.emplace(_self, [&](auto& s) {
      s.id = primary_key;
      s.owner = owner;
      s.value = value;
      s.timestamp = current_time_point().sec_since_epoch();
    });*/


    t_idx.modify(oldest, _self, [&](auto& s) {
     // s.id = primary_key;
      s.owner = owner;
      s.value = value;
      s.timestamp = current_time_point();
    });

    //Get index sorted by value
    auto value_sorted = dstore.get_index<"value"_n>();

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
    t_idx.modify(oldest, _self, [&](auto& s) {
      s.median = median;
    });

    gtable.modify(gtable.begin(), _self, [&](auto& s) {
      s.total_datapoints_count++;
    });

    auto gitr =  gtable.begin();

    print("gtable.begin()->total_datapoints_count:", gitr->total_datapoints_count,  "\n");

    if (gtable.begin()->total_datapoints_count % gitr->vote_interval == 0){
      update_votes();
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





  void create_user(name owner) {

    userstable users(_self, _self.value);

    auto itr = users.find(owner.value);

    if( itr == users.end() ) {
      users.emplace(_self, [&](auto& o) {
        o.name = owner;
        o.score = 0;
        o.creation_timestamp = current_time_point();
      });
    }

  }

  void process_donation(name from, name scope, asset quantity){

    globaltable gtable(_self, _self.value);
    statstable cstore(_self, scope.value);
    donationstable donations(_self, from.value);
    userstable users(_self, _self.value);

    auto uitr = users.find(from.value);

    if( uitr == users.end() ) create_user( from );

    uitr = users.find(from.value);

    users.modify(*uitr, _self, [&]( auto& o) {
      o.score += quantity.amount;
    });

    // store donation in table
    donations.emplace(_self, [&](auto& o) {
      o.id = donations.available_primary_key();
      o.donator = from;
      o.pair = scope;
      o.timestamp = current_time_point();
      o.amount = quantity;
    });

    auto gitr = gtable.begin();

    uint64_t size = std::distance(cstore.begin(), cstore.end());

    uint64_t upperbound = std::min(size, gitr->paid); //max number of oracles being paid

    auto count_index = cstore.get_index<"count"_n>(); //get list of oracles ranked by number of datapoints contributed for this scope (descending)

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
        payout = asset(amount, symbol(SYSTEM_SYMBOL, SYSTEM_PRECISION));
      }
      else {
        payout = asset(uquota, symbol(SYSTEM_SYMBOL, SYSTEM_PRECISION));
      }

      amount-= uquota;

      print("payout", payout, "\n");

      if (scope == _self) {

        //global donation to the contract, split between top oracles across all pairs
        cstore.modify(*itr, _self, [&]( auto& s ) {
          s.balance += payout;
        });

      }
      else {

        //donation to a specific pair, split between top oracles of only that pair
        statstable gstore(_self, _self.value);

        auto optr = gstore.find(itr->owner.value);

        gstore.modify(*optr, _self, [&]( auto& s ) {
          s.balance += payout;
        });

      }

      if (i>1 ){
        itr--;
        print("decrement 1", "\n");

      } 
    }


  }

  void process_bounty(name from, name pair, asset quantity){

    pairstable pairs(_self, _self.value);

    auto pitr = pairs.find(pair.value);

    pairs.modify(*pitr, _self, [&]( auto& s ) {
      s.bounty_amount += quantity;
    });

  }

};
