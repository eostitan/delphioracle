/*

  delphioracle

  Authors: Guillaume "Gnome" Babin-Tremblay - EOS Titan, Andrew "netuoso" Chaney - EOS Titan
  
  Website: https://eostitan.com
  Email: guillaume@eostitan.com

  Github: https://github.com/eostitan/delphioracle/
  
  Published under MIT License

*/

#include <delphioracle.hpp>

//Write datapoint
ACTION delphioracle::write(const name owner, const std::vector<quote>& quotes) {
  
  require_auth(owner);
  
  int length = quotes.size();

  print("length ", length);

  check(length>0, "must supply non-empty array of quotes");
  check(check_oracle(owner), "account is not a qualified oracle");

  statstable stable(_self, _self.value);
  pairstable pairs(_self, _self.value);

  auto oitr = stable.find(owner.value);

  

  //auto name_idx = pairs.find<"name"_n>();
  //auto itr = sorted_idx.begin();


  for (int i=0; i<length;i++){
    print("quote ", i, " ", quotes[i].value, " ",  quotes[i].pair, "\n");
    
    auto itr = pairs.find(quotes[i].pair.value);

    check(itr!=pairs.end() && itr->active == true, "pair not allowed");

    check_last_push(owner, quotes[i].pair);

    if (itr->bounty_amount>=one_larimer && oitr != stable.end()){

      //global donation to the contract, split between top oracles across all pairs
      stable.modify(*oitr, _self, [&]( auto& s ) {
        s.balance += one_larimer;
      });

      //gl  obal donation to the contract, split between top oracles across all pairs
      pairs.modify(*itr, _self, [&]( auto& s ) {
        s.bounty_amount -= one_larimer;
      });

    }
    else if (itr->bounty_awarded==false && itr->bounty_amount<one_larimer){

      //global donation to the contract, split between top oracles across all pairs
      pairs.modify(*itr, _self, [&]( auto& s ) {
        s.bounty_awarded = true;
      });

    }

    update_datapoints(owner, quotes[i].value, itr);

  }

  //for (int i=0; i<length;i++){
    //print("quote ", i, " ", quotes[i].value, " ",  quotes[i].pair, "\n");
    //check(quotes[i].value >= val_min && quotes[i].value <= val_max, "value outside of allowed range");
  //}

/*    for (int i=0; i<length;i++){    
  }
*/
  print("done \n");
  //TODO: check if symbol exists
  //require_recipient("eosusdcom111"_n);
  
}

ACTION delphioracle::writehash(const name owner, const checksum256 hash, const std::string reveal) {

  require_auth(owner);

  check(check_oracle(owner), "user is not a qualified oracle");

  globaltable gtable(_self, _self.value);
  statstable gstore(_self, _self.value);
  hashestable hstore(_self, _self.value);

  auto gitr = gtable.begin();
  // ensure user hasnt submitted an identical hash
  //auto h_idx = hstore.get_index<"hash"_n>();
  //auto hitr = h_idx.find(hash);
 // check(hitr == h_idx.end(), "oracle has previously submitted identical hash");

  auto o_idx = hstore.get_index<"owner"_n>();
  auto h_idx = hstore.get_index<"timestamp"_n>();

  checksum256 multiparty = NULL_HASH;

  auto previous_hash = o_idx.find(owner.value);
  if( previous_hash != o_idx.end() ) {

    time_point ctime = current_time_point();
    print("previous_hash->timestamp", previous_hash->timestamp.elapsed.to_seconds(), "\n");
    print("gitr->write_cooldown", gitr->write_cooldown, "\n");
    print("ctime", ctime.elapsed.to_seconds(), "\n");
    
    time_point next_push = eosio::time_point(previous_hash->timestamp.elapsed + eosio::microseconds(gitr->write_cooldown));
    
    check(ctime>=next_push, "can only call every 60 seconds");

    checksum256 hashed = sha256(reveal.c_str(), reveal.length());

    // increment count for user if their hash verified
    check(hashed == previous_hash->hash, "hash mismatch");

    multiparty = get_multiparty_hash(owner, reveal);
    
    //if(hashed == previous_hash->hash) {
    auto gsitr = gstore.find(owner.value);

    if (gsitr != gstore.end()) {
      gstore.modify( gsitr, owner, [&]( auto& s ) {
        s.timestamp = current_time_point();
        s.count++;
      });
    } else {
      gstore.emplace(owner, [&](auto& s) {
        s.owner = owner;
        s.timestamp = current_time_point();
        s.count = 1;
        s.balance = asset(0, symbol("EOS", 4));
        s.last_claim = NULL_TIME_POINT;
      });
    }

     o_idx.erase(previous_hash);

  }
  else {
    check(reveal=="", "reveal must be empty string on first writehash call");
  }

  // store users hash in table for future verification
  hstore.emplace(owner, [&](auto& o) {
    o.id = hstore.available_primary_key();
    o.owner = owner;
    o.multiparty = multiparty;
    o.hash = hash;
    o.reveal = reveal;
    o.timestamp = current_time_point();
  });

}


ACTION delphioracle::forfeithash(name owner) {

  require_auth(owner);

  hashestable hstore(_self, _self.value);

  auto o_idx = hstore.get_index<"owner"_n>();

  auto previous_hash = o_idx.find(owner.value);

  if( previous_hash != o_idx.end() ) o_idx.erase(previous_hash);

}

//claim rewards
ACTION delphioracle::claim(name owner) {
  
  require_auth(owner);

  globaltable gtable(_self, _self.value);
  statstable sstore(_self, _self.value);

  auto itr = sstore.find(owner.value);
  auto gitr = gtable.begin();

  check(itr != sstore.end(), "oracle not found");
  check( itr->balance.amount > 0, "no rewards to claim" );

  asset payout = itr->balance;

  //if( existing->quantity.amount == quantity.amount ) {
  //   bt.erase( *existing );
  //} else {
  sstore.modify( *itr, _self, [&]( auto& a ) {
      a.balance = asset(0, symbol("EOS", 4));
      a.last_claim = current_time_point();
  });

  gtable.modify( *gitr, _self, [&]( auto& a ) {
      a.total_claimed += payout;
  });

  //}

  //if quantity symbol == EOS -> token_contract

 // SEND_INLINE_ACTION(token_contract, transfer, {"eostitancore"_,"active"_n}, {"eostitancore"_, from, quantity, std::string("")} );
    
  action act(
    permission_level{_self, "active"_n},
    "eosio.token"_n, "transfer"_n,
    std::make_tuple(_self, owner, payout, std::string(""))
  );
  act.send();

}

//temp configuration
ACTION delphioracle::configure(globalinput g) {
  
  require_auth(_self);

  globaltable gtable(_self, _self.value);
  pairstable pairs(_self, _self.value);

  auto gitr = gtable.begin();
  auto pitr = pairs.begin();

  if (gitr == gtable.end()){

    gtable.emplace(_self, [&](auto& o) {
      o.id = 1;
      o.total_datapoints_count = 0;
      o.total_claimed = asset(0, symbol("EOS", 4));
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

    gtable.modify(*gitr, _self, [&]( auto& o ) {
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

      pairs.emplace(_self, [&](auto& o) {
        o.active = true;
        o.bounty_awarded = true;
        o.bounty_edited_by_custodians = true;
        o.proposer = _self;
        o.name = "eosusd"_n;
        o.bounty_amount = asset(0, symbol("EOS", 4));
        o.base_symbol =  symbol("EOS", 4);
        o.base_type = e_asset_type::eosio_token;
        o.base_contract = "eosio.token"_n;
        o.quote_symbol = symbol("USD", 2);
        o.quote_type = e_asset_type::fiat;
        o.quote_contract = ""_n;
        o.quoted_precision = 4;
      });

      datapointstable dstore(_self, name("eosusd"_n).value);

      //First data point starts at uint64 max
      uint64_t primary_key = 0;
     
      for (uint16_t i=0; i < 21; i++){

        //Insert next datapoint
        auto c_itr = dstore.emplace(_self, [&](auto& s) {
          s.id = primary_key;
          s.value = 0;
          s.timestamp = NULL_TIME_POINT;
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
ACTION delphioracle::newbounty(name proposer, pairinput pair) {

  require_auth(proposer);

  //Add request, proposer pays the RAM for the request + data structure for datapoints & bars. 

  pairstable pairs(_self, _self.value);
  datapointstable dstore(_self, pair.name.value);

  auto itr = pairs.find(pair.name.value);

  check(pair.name != "system"_n, "Cannot create a pair named system");
  check(itr == pairs.end(), "A pair with this name already exists.");

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
      s.timestamp = NULL_TIME_POINT;
    });

    primary_key++;

  }

}

//cancel a bounty
ACTION delphioracle::cancelbounty(name name, std::string reason) {
  
  pairstable pairs(_self, _self.value);
  datapointstable dstore(_self, name.value);

  auto itr = pairs.find(name.value);

  check(itr != pairs.end(), "bounty doesn't exist");

  print("itr->proposer", itr->proposer, "\n");

  check(has_auth(_self) || has_auth(itr->proposer), "missing required authority of contract or proposer");
  check(itr->active == false, "cannot cancel live pair");

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
ACTION delphioracle::votebounty(name owner, name bounty) {

  require_auth(owner); 

  pairstable pairs(_self, _self.value);

  auto pitr = pairs.find(bounty.value);

  check(!pitr->active, "pair is already active.");
  check(pitr != pairs.end(), "bounty not found.");

  custodianstable custodians(_self, _self.value);

  auto itr = custodians.find(owner.value);

  bool vote_approved = false;

  std::string err_msg = "";

  //print("itr->name", itr->name, "\n");

  if (itr != custodians.end()){
    //voter is custodian
    print("custodian found \n");

    std::vector<eosio::name> cv = pitr->approving_custodians;

    auto citr = find(cv.begin(), cv.end(), owner);

    //check(citr == cv.end(), "custodian already voting for bounty");

    if (citr == cv.end()){

      cv.push_back(owner);

      pairs.modify(*pitr, _self, [&]( auto& s ) {
        s.approving_custodians = cv;
      });

      print("custodian added vote \n");

      vote_approved=true;
      
    }
    else err_msg = "custodian already voting for bounty";

  }

  print("checking oracle qualification... \n");

  if (check_approver(owner)) {

    std::vector<eosio::name> ov = pitr->approving_oracles;

    auto oitr = find(ov.begin(), ov.end(), owner);

    if (oitr == ov.end()){

      ov.push_back(owner);

      pairs.modify(*pitr, _self, [&]( auto& s ) {
        s.approving_oracles = ov;
      });

      print("oracle added vote \n");

      vote_approved=true;

    }
    else err_msg = "oracle already voting for bounty";

  }
  else err_msg = "owner not a qualified oracle";

  check(vote_approved, err_msg.c_str());

  globaltable gtable(_self, _self.value);

  auto gitr = gtable.begin();

  uint64_t approving_custodians_count = std::distance(pitr->approving_custodians.begin(), pitr->approving_custodians.end());
  uint64_t approving_oracles_count = std::distance(pitr->approving_oracles.begin(), pitr->approving_oracles.end());

  if (approving_custodians_count>=gitr->approving_custodians_threshold && approving_oracles_count>=gitr->approving_oracles_threshold){
      print("activate bounty", "\n");

      pairs.modify(*pitr, _self, [&]( auto& s ) {
        s.active = true;
      });

  }

}

//vote bounty
ACTION delphioracle::unvotebounty(name owner, name bounty) {

  require_auth(owner); 

  pairstable pairs(_self, _self.value);

  auto pitr = pairs.find(bounty.value);

  check(!pitr->active, "pair is already active.");
  check(pitr != pairs.end(), "bounty not found.");

  custodianstable custodians(_self, _self.value);

  auto itr = custodians.find(owner.value);

  print("itr->name", itr->name, "\n");

  if (itr != custodians.end()){
    //voter is custodian
    print("custodian found \n");

    std::vector<eosio::name> cv = pitr->approving_custodians;

    auto citr = find(cv.begin(), cv.end(), owner);

    check(citr != cv.end(), "custodian is not voting for bounty");

    cv.erase(citr);

    pairs.modify(*pitr, _self, [&]( auto& s ) {
      s.approving_oracles = cv;
    });

    print("custodian removed vote \n");
    
  }
  else {

    print("checking oracle qualification... \n");

    //check(check_approver(owner), "owner not a qualified oracle"); // not necessary

    std::vector<eosio::name> ov = pitr->approving_oracles;

    auto oitr = find(ov.begin(), ov.end(), owner);

    check(oitr != ov.end(), "not an oracle or oracle is not voting for bounty");

    ov.erase(oitr);

    pairs.modify(*pitr, _self, [&]( auto& s ) {
      s.approving_oracles = ov;
    });

    print("oracle removed vote \n");

  }

}

//edit a bounty's information
ACTION delphioracle::editbounty(name name, pairinput pair) {

  eosio::name proposer;

  check(has_auth(_self) || has_auth(proposer), "missing required authority of contract or proposer");

  //check if bounty_edited_by_custodians == false || has_auth(_self), otherwise throw exception. Custodians have final say
  //edit bounty data

  //if edited by custodians, set flag bounty_edited_by_custodians to true

}

//edit pair 
ACTION delphioracle::editpair(pairs pair) {
  
  require_auth(_self);
  
  //edit pair description

}

//delete pair 
ACTION delphioracle::deletepair(name name) {
  
  require_auth(_self); //controlled by msig over active key
  
  //delete pair, post reason to chain for reference

}

//add custodian
ACTION delphioracle::addcustodian(name name) {

  require_auth(_self); 

  custodianstable custodians(_self, _self.value);

  custodians.emplace(_self, [&](auto& s) {
    s.name = name;
  });


}

//remove custodian
ACTION delphioracle::delcustodian(name name) {

  require_auth(_self); 

  custodianstable custodians(_self, _self.value);

  auto itr = custodians.find(name.value);

  check(itr != custodians.end(), "account not a custodian");

  custodians.erase(itr);

}

//registers a user
ACTION delphioracle::reguser(name owner) {

  require_auth(owner);

  if( !check_user(owner) ) create_user( owner );

}

//updates all users voting scores
//run at some random interval daily
ACTION delphioracle::updateusers() {

  require_auth( _self );

  userstable users(_self, _self.value);
  voters_table vtable("eosio"_n, name("eosio").value);

  for(auto itr = users.begin(); itr != users.end(); ++itr) {

    // add proxy score
    auto v_itr = vtable.find(itr->name.value);
    auto score = itr->score;

    if( v_itr != vtable.end() && v_itr->proxy == _self) {
      score += v_itr->staked;
    }

    users.modify(*itr, _self, [&]( auto& o ) {
      o.score = score;
    });

  }

}

//Clear all data
ACTION delphioracle::clear(name pair) {

  require_auth(_self);

  globaltable gtable(_self, _self.value);
  statstable gstore(_self, _self.value);
  statstable lstore(_self, pair.value);
  datapointstable estore(_self,  pair.value);
  pairstable pairs(_self, _self.value);
  custodianstable ctable(_self, _self.value);
  hashestable htable(_self, _self.value);
  
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
  
  while (htable.begin() != htable.end()) {
      auto itr = htable.end();
      itr--;
      htable.erase(itr);
  }

}

ACTION delphioracle::voteabuser(const name owner, const name abuser) {
  
  require_auth(owner);
  check(check_oracle(abuser), "abuser is not a qualified oracle");

  donationstable donations(_self, owner.value);
  voters_table vtable("eosio"_n, name("eosio").value);

  // donations
  auto d_idx = donations.get_index<"donator"_n>();
  auto d_itr = d_idx.find(owner.value);

  auto total_donated = 0;

  while (d_itr->donator == owner && d_itr != d_idx.end()) {
    total_donated += d_itr->amount.amount;
    d_itr++;
  }

  auto v_itr = vtable.find(owner.value);

  // proxy voting
  auto total_proxied = 0;

  if( v_itr != vtable.end() && v_itr->proxy == _self) {
    total_proxied += v_itr->staked;
  }

  // TODO: verify user object exists and user has some voting score
  check(total_donated > 0 || total_proxied > 0, "user must donate or proxy vote to delphioracle to vote for abusers");

  print("user: ", owner, " is voting for abuser: ", abuser, " with total stake: ", total_donated + total_proxied);

  // store data for abuse vote

}

ACTION delphioracle::migratedata() {

  require_auth(_self);

  statstable stats(name("delphibackup"), name("delphibackup").value);
  statstable _stats(_self, _self.value);

  check(_stats.begin() == _stats.end(), "stats info already exists; call clear first");

  auto gitr = stats.begin();
  while (gitr != stats.end()) {
    _stats.emplace(_self, [&](auto& s){
      s.owner = gitr->owner;
      s.timestamp = gitr->timestamp;
      s.count = gitr->count;
      s.last_claim = gitr->last_claim;
      s.balance = gitr->balance;
    });
    gitr++;
  }

  npairstable pairs(name("delphibackup"), name("delphibackup").value);
  pairstable _pairs(_self, _self.value);
  auto pitr = pairs.begin();
  while (pitr != pairs.end()) {
    _pairs.emplace(_self, [&](auto& p){
      p.active = pitr->active;
      p.bounty_awarded = pitr->bounty_awarded;
      p.bounty_edited_by_custodians = pitr->bounty_edited_by_custodians;
      p.proposer = pitr->proposer;
      p.name = pitr->name;
      p.bounty_amount = pitr->bounty_amount;
      p.base_symbol =  pitr->base_symbol;
      p.base_type = pitr->base_type;
      p.base_contract = pitr->base_contract;
      p.quote_symbol = pitr->quote_symbol;
      p.quote_type = pitr->quote_type;
      p.quote_contract = pitr->quote_contract;
      p.quoted_precision = pitr->quoted_precision;
    });

    statstable pstats(name("delphibackup"), pitr->name.value);
    statstable _pstats(_self, pitr->name.value);
    auto sitr = pstats.begin();
    while (sitr != pstats.end()) {
      _pstats.emplace(_self, [&](auto& s){
        s.owner = sitr->owner;
        s.timestamp = sitr->timestamp;
        s.count = sitr->count;
        s.last_claim = sitr->last_claim;
        s.balance = sitr->balance;
      });
      sitr++;
    }

    datapointstable datapoints(name("delphibackup"), pitr->name.value);
    datapointstable _datapoints(_self, pitr->name.value);
    auto ditr = datapoints.begin();
    while (ditr != datapoints.end()) {
      _datapoints.emplace(_self, [&](auto& d){
        d.id = ditr->id;
        d.owner = ditr->owner;
        d.value = ditr->value;
        d.median = ditr->median;
        d.timestamp = ditr->timestamp;
      });
      ditr++;
    }

    pitr++;
  }

}
