#include "quickfix/Application.h"
#include "quickfix/MessageCracker.h"
#include "quickfix/Session.h"
#include "quickfix/SessionSettings.h"
#include "quickfix/FileStore.h"
#include "quickfix/SocketInitiator.h"
#include "quickfix/FileLog.h"
#include "quickfix/fix44/NewOrderSingle.h"
#include "trading_engine.h"
#include <iostream>
#include <cstdlib>
#include <string>

class FixApp : public FIX::Application, public FIX::MessageCracker {
public:
    void onCreate(const FIX::SessionID&) override {}

    void onLogon(const FIX::SessionID&) override {
        std::cout << "FIX session logged on" << std::endl;
    }

    void onLogout(const FIX::SessionID&) override {
        std::cout << "FIX session logged out" << std::endl;
    }

    void toAdmin(FIX::Message&, const FIX::SessionID&) override {}

    void fromAdmin(const FIX::Message&, const FIX::SessionID&) override {}

    void toApp(FIX::Message&, const FIX::SessionID&) override {}

    void fromApp(const FIX::Message& message, const FIX::SessionID& sessionID) override {
        crack(message, sessionID);
    }

    // Process NewOrderSingle FIX messages.
    void onMessage(const FIX44::NewOrderSingle& message, const FIX::SessionID& sessionID) override {
        FIX::ClOrdID clOrdID;
        FIX::Symbol symbol;
        FIX::Side side;
        FIX::Price price;
        FIX::OrderQty qty;
        message.get(clOrdID);
        message.get(symbol);
        message.get(side);
        message.get(price);
        message.get(qty);

        // Convert FIX side to our order side: 'B' for Buy, 'S' for Sell.
        char sideChar = (side == FIX::Side_BUY) ? 'B' : 'S';
        // Convert ClOrdID (string) to int for order id.
        int id = std::atoi(clOrdID.getValue().c_str());
        // Use the extracted symbol.
        std::string symStr = symbol.getValue();
        // Call our C++ wrapper with all five arguments.
        cpp_add_order(id, symStr, price, qty, sideChar);
        std::cout << "Processed FIX NewOrderSingle for symbol " << symStr << ": order id " << id << std::endl;
    }
};

int main(int argc, char** argv) {
    try {
        if (argc < 2) {
            std::cerr << "Usage: simulator_fix <config_file>" << std::endl;
            return 1;
        }
        FIX::SessionSettings settings(argv[1]);
        FixApp application;
        FIX::FileStoreFactory storeFactory(settings);
        FIX::FileLogFactory logFactory(settings);
        FIX::SocketInitiator initiator(application, storeFactory, settings, logFactory);
        initiator.start();
        std::cout << "FIX Initiator started. Press <enter> to quit." << std::endl;
        std::cin.get();
        initiator.stop();
    } catch (std::exception &e) {
        std::cerr << "Exception: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}

