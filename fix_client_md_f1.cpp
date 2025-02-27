#include <quickfix/Application.h>
#include <quickfix/FileStore.h>
#include <quickfix/SocketInitiator.h>
#include <quickfix/Log.h>
#include <quickfix/SessionSettings.h>
#include <quickfix/Field.h>

#include <iostream>
#include <memory>
#include <string>

class MyApplication : public FIX::Application {
public:

    void printMessageFields(const FIX::Message &message)
    {
        FIX::DataDictionary dataDictionary("/usr/local/share/quickfix/FIX44.xml");
        for (FIX::FieldMap::const_iterator it = message.begin(); it != message.end(); ++it)
        {
            std::string fieldName;
            dataDictionary.getFieldName(it->getTag(), fieldName);
            std::cout << fieldName << "->" << it->getFixString() << std::endl;
        }
    }

    void onCreate(const FIX::SessionID& sessionID) override {
        std::cout << "Session created: " << sessionID << std::endl;
    }

    void onLogon(const FIX::SessionID& sessionID) override {
        std::cout << "Logon: " << sessionID << std::endl;

        FIX::Message marketDataRequest;
        marketDataRequest.getHeader().setField(FIX::FIELD::MsgType, FIX::MsgType_MarketDataRequest);

        marketDataRequest.setField(FIX::FIELD::MDReqID, "ltp"+std::to_string(time(NULL))+"ltp");
        marketDataRequest.setField(FIX::FIELD::SubscriptionRequestType, "0");  // Snapshot
        marketDataRequest.setField(FIX::FIELD::MarketDepth, "0");  // Full Market Depth
        marketDataRequest.setField(FIX::FIELD::MDUpdateType, "0");  // Full Refresh

        FIX::Group mdEntryTypes(267, FIX::FIELD::MDEntryType);  // 267=NoMDEntryTypes
        mdEntryTypes.setField(FIX::FIELD::MDEntryType, "0");  // Bid
        marketDataRequest.addGroup(mdEntryTypes);
        
        mdEntryTypes.setField(FIX::FIELD::MDEntryType, "1");  // Offer
        marketDataRequest.addGroup(mdEntryTypes);
        
        mdEntryTypes.setField(FIX::FIELD::MDEntryType, "2");  // Trade
        marketDataRequest.addGroup(mdEntryTypes);

        mdEntryTypes.setField(FIX::FIELD::MDEntryType, "4");  // Opening Price
        marketDataRequest.addGroup(mdEntryTypes);

        mdEntryTypes.setField(FIX::FIELD::MDEntryType, "5");  // Closing Price
        marketDataRequest.addGroup(mdEntryTypes);

        mdEntryTypes.setField(FIX::FIELD::MDEntryType, "7");  // Trading Session High Price
        marketDataRequest.addGroup(mdEntryTypes);

        mdEntryTypes.setField(FIX::FIELD::MDEntryType, "8");  // Trading Session Low Price
        marketDataRequest.addGroup(mdEntryTypes);

        mdEntryTypes.setField(FIX::FIELD::MDEntryType, "B");  // Trading Volume
        marketDataRequest.addGroup(mdEntryTypes);

        FIX::Group noRelatedSymGroup(146, FIX::FIELD::Symbol);  // 146=NoRelatedSym
        noRelatedSymGroup.setField(FIX::FIELD::Symbol, "BTCUSD");
        marketDataRequest.addGroup(noRelatedSymGroup);

        FIX::Session::sendToTarget(marketDataRequest, sessionID);
    }

    void onLogout(const FIX::SessionID& sessionID) override {
        std::cout << "Logout: " << sessionID << std::endl;
    }
    void toAdmin(FIX::Message& message, const FIX::SessionID& sessionID) override {
        if (message.getHeader().getField(FIX::FIELD::MsgType) == FIX::MsgType_Logon) {
            message.setField(FIX::Username("lts24md"));
            message.setField(FIX::Password("OlglriQKONLG4V5VzM4e"));
        }
        std::cout << "Sending admin message: " << message << std::endl;
    }

    void toApp(FIX::Message& message, const FIX::SessionID& sessionID) override {
        std::cout << "Sending application message: " << message << std::endl;
        printMessageFields(message);
    }

    void fromAdmin(const FIX::Message& message, const FIX::SessionID& sessionID) override {
        std::cout << "Received admin message: " << message << std::endl;
    }

    void fromApp(const FIX::Message& message, const FIX::SessionID& sessionID) override {
        std::cout << "Received application message: " << message << std::endl;
        printMessageFields(message);
        // 8=FIX.4.49=14435=W34=1249=DBS52=20241105-10:01:02.95156=LTS24MD55=BTCUSD262=uniqueID268=3269=2270=70000271=0.1273=01:22:55269=0346=0269=1346=010=133
        
        try {
            FIX::StringField msgType(FIX::FIELD::MsgType);
            message.getHeader().getField(msgType);

            std::string msgTypeValue = msgType.getString();

            if (msgTypeValue == FIX::MsgType_MarketDataSnapshotFullRefresh) 
            {
                std::cout << "Received MarketDataSnapshotFullRefresh message: " << message << std::endl;

                FIX::Group marketDataGroup(268, FIX::FIELD::MDEntryType);  // (268=NoMDEntries, 269=MDEntryType)
                int index = 1;

                while (true) {
                    try {
                        message.getGroup(index, marketDataGroup);
                    } catch (const FIX::FieldNotFound&) {
                        break;
                    }

                    FIX::StringField entryTypeField(FIX::FIELD::MDEntryType);
                    marketDataGroup.getField(entryTypeField);
                    std::string entryType = entryTypeField.getString();

                    double price = 0.0;
                    double volume = 0.0;

                    if (marketDataGroup.isSetField(FIX::FIELD::MDEntryPx)) {
                        FIX::DoubleField priceField(FIX::FIELD::MDEntryPx);
                        marketDataGroup.getField(priceField);
                        price = priceField.getValue();
                    }

                    if (marketDataGroup.isSetField(FIX::FIELD::MDEntrySize)) {
                        FIX::DoubleField volumeField(FIX::FIELD::MDEntrySize);
                        marketDataGroup.getField(volumeField);
                        volume = volumeField.getValue();
                    }

                    std::cout << "MDEntryType: " << entryType 
                            << ", Price: " << price 
                            << ", Volume: " << volume << std::endl;

                    index++;
                }
            }
            else 
            {
                std::cout << "Received non-market data message: " << message << std::endl;
            }
        } catch (const FIX::FieldNotFound& e) {
            std::cerr << "Field not found: " << e.what() << std::endl;
        } catch (const std::exception& e) {
            std::cerr << "Error in fromApp: " << e.what() << std::endl;
        }
    }
};

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <config_file.cfg>" << std::endl;
        return 1;
    }

    std::string configFile = argv[1];

    try {
        FIX::SessionSettings settings(configFile);
        MyApplication application;

        FIX::FileStoreFactory storeFactory(settings);
        FIX::ScreenLogFactory logFactory(settings);
        FIX::SocketInitiator initiator(application, storeFactory, settings, logFactory);

        initiator.start();

        std::cout << "Press Ctrl-C to exit." << std::endl;
        while (true) {
            FIX::process_sleep(1);
        }
        
        initiator.stop();
    } catch (std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
