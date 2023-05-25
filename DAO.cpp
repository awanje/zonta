#include <eosio/eosio.hpp>
#include <eosio/asset.hpp>

using namespace eosio;

class [[eosio::contract]] dao_token : public contract {
public:
    using contract::contract;

    [[eosio::on_notify("*::transfer")]]
    void on_transfer(name from, name to, asset quantity, std::string memo) {
        if (to != get_self() || from == get_self()) {
            return;
        }

        eosio_assert(quantity.symbol.is_valid(), "Invalid symbol");
        eosio_assert(quantity.symbol == symbol("DAO", 4), "Invalid symbol");

       
        balances_table balances(get_self(), from.value);
        auto it = balances.find(quantity.symbol.code().raw());
        eosio_assert(it != balances.end(), "Sender does not have a balance for the specified symbol");
        eosio_assert(it->balance >= quantity, "Insufficient balance");

      
        balances.modify(it, get_self(), [&](auto& row) {
            row.balance -= quantity;
        });

       
        update_balance(to, quantity);

        transfer_event(from, to, quantity);

        on_token_transfer(from, to, quantity);
    }

private:
    struct [[eosio::table]] account {
        asset balance;

        uint64_t primary_key() const { return balance.symbol.code().raw(); }
    };

    typedef eosio::multi_index<"balances"_n, account> balances_table;

    void update_balance(name account, asset quantity) {
        balances_table balances(get_self(), account.value);
        auto it = balances.find(quantity.symbol.code().raw());

        if (it == balances.end()) {
            balances.emplace(get_self(), [&](auto& row) {
                row.balance = quantity;
            });
        } else {
            balances.modify(it, get_self(), [&](auto& row) {
                row.balance += quantity;
            });
        }
    }

  
    void transfer_event(name from, name to, asset quantity) {
        require_recipient(from);
        require_recipient(to);

        if (is_account(from)) {
            eosio::action(
                eosio::permission_level{get_self(), "active"_n},
                "eosio.token"_n,
                "transfer"_n,
                std::make_tuple(get_self(), from, quantity, std::string("Token transfer"))
            ).send();
        }

        if (is_account(to)) {
            eosio::action(
                eosio::permission_level{get_self(), "active"_n},
                "eosio.token"_n,
                "transfer"_n,
                std::make_tuple(get_self(), to, quantity, std::string("Token transfer"))
            ).send();
        }
    }

    void on_token_transfer(name from, name to, asset quantity) {
        
        eosio::print("Transfer from: ", from, ", Transfer to: ", to, ", Quantity: ", quantity);
        eosio::print(", Voting Power: ", calculate_voting_power(quantity));
    }

    uint64_t calculate_voting_power(asset quantity) {
        return quantity.amount * 10;
    }
};
