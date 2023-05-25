#include <eosio/eosio.hpp>
#include <eosio/asset.hpp>

using namespace eosio;

class [[eosio::contract("zontacoin")]] zontacoin : public contract {
  public:
      zontacoin(name receiver, name code, datastream<const char*> ds) : contract(receiver, code, ds) {}

      [[eosio::action]]
      void presale(name buyer, asset amount) {
          require_auth(buyer);

                eosio_assert(is_presale_active(), "Presale has ended");

                    eosio_assert(amount.symbol == ETH_SYMBOL, "Invalid presale currency");
          eosio_assert(amount.amount == PRE_SALE_PRICE.amount, "Invalid presale amount");

                   eosio_assert(get_max_purchase_amount(buyer) >= amount, "Exceeds maximum purchase amount");

                   issue_coins(buyer, amount);
      }

      [[eosio::action]]
      void transfer(name from, name to, asset quantity, std::string memo) {
          require_auth(from);

          
          eosio_assert(quantity.symbol == ZTN_SYMBOL, "Invalid token symbol");

          
          auto from_acct = get_account(from);
          eosio_assert(from_acct.balance >= quantity, "Insufficient balance");

          sub_balance(from_acct, quantity);
          add_balance(to, quantity, from);
      }

  private:
      static constexpr symbol ZTN_SYMBOL = symbol("ZTN", 4);
      static constexpr symbol ETH_SYMBOL = symbol("ETH", 4);
      static constexpr asset PRE_SALE_PRICE = asset(20000, ETH_SYMBOL);
      static constexpr uint64_t PRE_SALE_SUPPLY = 5000000000;
      static constexpr uint64_t MAX_PURCHASE_AMOUNT = 1000000; 

            struct [[eosio::table]] totalsupply {
          uint64_t id;
          asset supply;

          uint64_t primary_key() const { return id; }
      };
      typedef eosio::multi_index<"totalsupply"_n, totalsupply> totalsupply_index;

            struct [[eosio::table]] account {
          name owner;
          asset balance;
          uint64_t presale_balance;
          time_point_sec vesting_start_time;
          uint64_t vesting_period;

          uint64_t primary_key() const { return owner.value; }
      };
      typedef eosio::multi_index<"accounts"_n, account> account_index;

      bool is_presale_active() {
     time_point_sec current_time = current_time_point();
    time_point_sec start_time = time_point_sec(eosio::seconds(1651344000)); 
    time_point_sec end_time = time_point_sec(eosio::seconds(1704063999));

    if (current_time < start_time || current_time > end_time) {
        return false;
    }
                    totalsupply_index totalsupplies(get_self(), get_self().value);
          auto itr = totalsupplies.find(ZTN_SYMBOL.raw());
          return itr == totalsupplies.end() || itr->supply.amount < PRE_SALE_SUPPLY;
      }

      asset get_max_purchase_amount(name buyer) {
          account_index accounts(get_self(), ZTN_SYMBOL.raw());
          auto itr = accounts.find(buyer.value);
          if (itr != accounts.end())


uint64_t max_purchase = MAX_PURCHASE_AMOUNT - itr->presale_balance;
          return asset(max_purchase, ZTN_SYMBOL);
      }

      account get_account(name owner) {
          account_index accounts(get_self(), ZTN_SYMBOL.raw());
          auto itr = accounts.find(owner.value);
          eosio_assert(itr != accounts.end(), "Account does not exist");
          return *itr;
      }

      void issue_coins(name to, asset quantity) {
          totalsupply_index totalsupplies(get_self(), get_self().value);
          auto itr = totalsupplies.find(ZTN_SYMBOL.raw());
          if (itr == totalsupplies.end()) {
              totalsupplies.emplace(get_self(), [&](auto& ts) {
                  ts.id = ZTN_SYMBOL.raw();
                  ts.supply = quantity;
              });
          } else {
              totalsupplies.modify(itr, same_payer, [&](auto& ts) {
                  ts.supply += quantity;
              });
          }

            account_index accounts(get_self(), ZTN_SYMBOL.raw());
          auto itr2 = accounts.find(to.value);
          if (itr2 == accounts.end()) {
              accounts.emplace(get_self(), [&](auto& a) {
                  a.owner = to;
                  a.balance = quantity;
                  a.presale_balance = quantity.amount;
              });
          } else {
              accounts.modify(itr2, same_payer, [&](auto& a) {
                  a.balance += quantity;
                  a.presale_balance += quantity.amount;
              });
          }
      }

      void sub_balance(account& owner, asset value) {
          account_index accounts(get_self(), ZTN_SYMBOL.raw());
          auto itr = accounts.find(owner.owner.value);
          eosio_assert(itr != accounts.end(), "Account does not exist");

          accounts.modify(itr, same_payer, [&](auto& a) {
              a.balance -= value;
          });
      }

      void add_balance(name owner, asset value, name ram_payer) {
          account_index accounts(get_self(), ZTN_SYMBOL.raw());
          auto itr = accounts.find(owner.value);
          if (itr == accounts.end()) {
              accounts.emplace(ram_payer, [&](auto& a) {
                  a.owner = owner;
                  a.balance = value;
                  a.presale_balance = 0;
                  a.vesting_start_time = time_point_sec(current_time_point());
                  a.vesting_period = 365 * 24 * 60 * 60; // 1 year vesting period
              });
          } else {
              accounts.modify(itr, same_payer, [&](auto& a) {
                  a.balance += value;
              });
          }
      }
};
