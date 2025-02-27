#include <quickfix/Application.h>
#include <quickfix/FileStore.h>
#include <quickfix/SocketInitiator.h>
#include <quickfix/Log.h>
#include <quickfix/SessionSettings.h>

#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>

class MyApplication : public FIX::Application {
public:
    void onCreate(const FIX::SessionID& sessionID) override {
        std::cout << "Session created: " << sessionID << std::endl;
    }

    void onLogon(const FIX::SessionID& sessionID) override {
        std::cout << "Logon: " << sessionID << std::endl;

        // 构建 MarketDataRequest 消息
        FIX::Message marketDataRequest;
        marketDataRequest.getHeader().setField(FIX::FIELD::MsgType, FIX::MsgType_MarketDataRequest);

        // 设置请求字段
        marketDataRequest.setField(FIX::FIELD::MDReqID, "uniqueID");
        marketDataRequest.setField(FIX::FIELD::SubscriptionRequestType, "1");  // Snapshot + Updates (Subscribe)
        marketDataRequest.setField(FIX::FIELD::MarketDepth, "1");  // Top of Book
        marketDataRequest.setField(FIX::FIELD::MDUpdateType, "1");  // Incremental Refresh

        // 设置订阅的条目类型
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

        mdEntryTypes.setField(FIX::FIELD::MDEntryType, "B");  // Trade Volume
        marketDataRequest.addGroup(mdEntryTypes);

        // 设置订阅的合约
        FIX::Group relatedSym(146, FIX::FIELD::Symbol);  // 146=NoRelatedSym
        relatedSym.setField(FIX::FIELD::Symbol, "BTCUSD");
        marketDataRequest.addGroup(relatedSym);

        // 发送 MarketDataRequest 消息
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
    }

    void toApp(FIX::Message& message, const FIX::SessionID& sessionID) override {}

    void fromAdmin(const FIX::Message& message, const FIX::SessionID& sessionID) override {}

    void fromApp(const FIX::Message& message, const FIX::SessionID& sessionID) override {
        std::cout << "Received application message: " << message << std::endl;

        try {
            FIX::StringField msgType(FIX::FIELD::MsgType);
            message.getHeader().getField(msgType);

            std::string msgTypeValue = msgType.getString();

            if (msgTypeValue == FIX::MsgType_MarketDataSnapshotFullRefresh || msgTypeValue == FIX::MsgType_MarketDataIncrementalRefresh) {
                std::cout << "Received " << (msgTypeValue == FIX::MsgType_MarketDataSnapshotFullRefresh ? "MarketDataSnapshotFullRefresh" : "MarketDataIncrementalRefresh") << " message: " << message << std::endl;

                FIX::Group marketDataGroup(268, FIX::FIELD::MDEntryType);  // (268=NoMDEntries, 269=MDEntryType)
                int index = 1;

                std::unordered_map<std::string, std::string> entryTypeMap = {
                    {"0", "Bid"},
                    {"1", "Offer"},
                    {"2", "Trade"},
                    {"4", "Opening Price"},
                    {"5", "Closing Price"},
                    {"7", "Trading Session High Price"},
                    {"8", "Trading Session Low Price"},
                    {"B", "Trade Volume"}
                };

                while (true) {
                    try {
                        message.getGroup(index, marketDataGroup);
                    } catch (const FIX::FieldNotFound&) {
                        break;
                    }

                    FIX::StringField entryTypeField(FIX::FIELD::MDEntryType);
                    marketDataGroup.getField(entryTypeField);
                    std::string entryType = entryTypeField.getString();
                    std::string entryTypeDesc = entryTypeMap[entryType];

                    double price = 0.0;
                    double volume = 0.0;
                    std::string symbol = "";
                    std::string timestamp = "";
                    std::string channel = "";

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

                    if (message.isSetField(FIX::FIELD::Symbol)) {
                        FIX::StringField symbolField(FIX::FIELD::Symbol);
                        message.getField(symbolField);
                        symbol = symbolField.getString();
                    }

                    if (marketDataGroup.isSetField(FIX::FIELD::MDEntryDate)) {
                        FIX::UtcDateOnlyField dateField(FIX::FIELD::MDEntryDate);
                        marketDataGroup.getField(dateField);
                        timestamp = dateField.getString();
                    }

                    if (marketDataGroup.isSetField(FIX::FIELD::MDEntryTime)) {
                        FIX::UtcTimeOnlyField timeField(FIX::FIELD::MDEntryTime);
                        marketDataGroup.getField(timeField);
                        timestamp += " " + timeField.getString();
                    }

                    if (marketDataGroup.isSetField(FIX::FIELD::MDEntryID)) {
                        FIX::StringField channelField(FIX::FIELD::MDEntryID);
                        marketDataGroup.getField(channelField);
                        channel = channelField.getString();
                    }

                    if (msgTypeValue == FIX::MsgType_MarketDataIncrementalRefresh) {
                        FIX::StringField updateActionField(FIX::FIELD::MDUpdateAction);
                        marketDataGroup.getField(updateActionField);
                        std::string updateAction = updateActionField.getString();

                        std::cout << "Update Action: " << updateAction << std::endl;
                    }

                    std::cout << "Timestamp=[" << timestamp << "], Channel=[" << channel << "] Entry " << index << ": Symbol=" << symbol << ", Type=" << entryType << " (" << entryTypeDesc << "), Price=" << price << ", Volume=" << volume << std::endl;

                    index++;
                }
            }

            // 字段编号到字段说明的映射
            std::unordered_map<int, std::string> fieldDescriptions = {
                {8, "BeginString"},
                {9, "BodyLength"},
                {35, "MsgType"},
                {34, "MsgSeqNum"},
                {49, "SenderCompID"},
                {52, "SendingTime"},
                {56, "TargetCompID"},
                {55, "Symbol"},
                {262, "MDReqID"},
                {268, "NoMDEntries"},
                {10, "CheckSum"},
                {281, "MDReqRejReason"}
                // 你可以根据需要添加更多的字段说明
            };

            // 解析并输出所有字段
            std::cout << "All fields in the message:" << std::endl;
            const FIX::FieldMap& headerFields = message.getHeader();
            for (FIX::FieldMap::const_iterator it = headerFields.begin(); it != headerFields.end(); ++it) {
                int tag = it->getTag();
                std::string description = fieldDescriptions.count(tag) ? fieldDescriptions[tag] : "Unknown";
                std::cout << "Header Field " << tag << " (" << description << "): " << it->getString() << std::endl;
            }

            const FIX::FieldMap& bodyFields = message;
            for (FIX::FieldMap::const_iterator it = bodyFields.begin(); it != bodyFields.end(); ++it) {
                int tag = it->getTag();
                std::string description = fieldDescriptions.count(tag) ? fieldDescriptions[tag] : "Unknown";
                std::cout << "Body Field " << tag << " (" << description << "): " << it->getString() << std::endl;
            }

            const FIX::FieldMap& trailerFields = message.getTrailer();
            for (FIX::FieldMap::const_iterator it = trailerFields.begin(); it != trailerFields.end(); ++it) {
                int tag = it->getTag();
                std::string description = fieldDescriptions.count(tag) ? fieldDescriptions[tag] : "Unknown";
                std::cout << "Trailer Field " << tag << " (" << description << "): " << it->getString() << std::endl;
            }

        } catch (const FIX::FieldNotFound& e) {
            std::cerr << "Field not found: " << e.what() << std::endl;
        } catch (const FIX::IncorrectDataFormat& e) {
            std::cerr << "Incorrect data format: " << e.what() << std::endl;
        } catch (const FIX::IncorrectTagValue& e) {
            std::cerr << "Incorrect tag value: " << e.what() << std::endl;
        } catch (const FIX::RejectLogon& e) {
            std::cerr << "Logon rejected: " << e.what() << std::endl;
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