#include <quickfix/Application.h>
#include <quickfix/FileStore.h>
#include <quickfix/SocketInitiator.h>
#include <quickfix/Log.h>
#include <quickfix/SessionSettings.h>
#include <quickfix/fix44/NewOrderSingle.h>
#include "quickfix/fix44/ExecutionReport.h"
#include "quickfix/fix44/OrderCancelRequest.h"

#include <iostream>
#include <memory>
#include <string>
class MyApplication : public FIX::Application {
public:
    void onCreate(const FIX::SessionID& sessionID) override {
        std::cout << "Session created: " << sessionID << std::endl;
    }

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

    void onLogon(const FIX::SessionID& sessionID) override {
        std::cout << "Logon: " << sessionID << std::endl;

        FIX44::NewOrderSingle order;
        order.set(FIX::ClOrdID("ltp"+std::to_string(time(NULL))+"ltp"));
        order.set(FIX::Price(90000));
        order.set(FIX::OrderQty(0.0002));
        order.set(FIX::Symbol("BTCUSD"));
        order.set(FIX::Side(FIX::Side_BUY));
        order.set(FIX::OrdType(FIX::OrdType_LIMIT));
        // order.set(FIX::TimeInForce(FIX::TimeInForce_IMMEDIATE_OR_CANCEL));
        order.set(FIX::TimeInForce(FIX::TimeInForce_GOOD_TILL_CANCEL));
        order.set(FIX::Account("4ab47a03-9625-4430-8a3d-e2d35ee7d0b4"));
        order.set(FIX::TransactTime(time(NULL)));
        FIX::Session::sendToTarget(order, sessionID);
    }

    void onLogout(const FIX::SessionID& sessionID) override {
        std::cout << "Logout: " << sessionID << std::endl;
    }

    void toAdmin(FIX::Message& message, const FIX::SessionID& sessionID) override {
        if (message.getHeader().getField(FIX::FIELD::MsgType) == FIX::MsgType_Logon) {
            message.setField(FIX::Username("4ab47a03-9625-4430-8a3d-e2d35ee7d0b4"));
            message.setField(FIX::Password("Oa3CZFRyWZLp54hpGA2D"));
            message.setField(FIX::ResetSeqNumFlag(true)); // Reset sequence number 141=Y
            message.getHeader().setField(FIX::MsgSeqNum(1)); // 34=1
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
        std::string msgType = message.getHeader().getField(FIX::FIELD::MsgType);
        if (msgType == FIX::MsgType_ExecutionReport)
        {
            FIX44::ExecutionReport executionReport(message);
            std::cout << "Received ExecutionReport: " << executionReport << std::endl;

            if (executionReport.getField(FIX::FIELD::OrdStatus) == std::string(&FIX::OrdStatus_NEW, 1))
            {
                FIX44::OrderCancelRequest cancelRequest;
                cancelRequest.set(FIX::ClOrdID(executionReport.getField(FIX::FIELD::ClOrdID)));
                cancelRequest.set(FIX::Symbol("BTCUSD"));
                cancelRequest.set(FIX::Side(FIX::Side_BUY));
                cancelRequest.set(FIX::Account("4ab47a03-9625-4430-8a3d-e2d35ee7d0b4"));
                cancelRequest.set(FIX::TransactTime(time(NULL)));

                // cancelRequest.set(FIX::OrigClOrdID(executionReport.getField(FIX::FIELD::OrigClOrdID)));
                cancelRequest.set(FIX::OrigClOrdID(executionReport.getField(FIX::FIELD::ClOrdID)));
                cancelRequest.set(FIX::OrderID(executionReport.getField(FIX::FIELD::OrderID)));
                cancelRequest.set(FIX::OrderQty(std::stod(executionReport.getField(FIX::FIELD::OrderQty))));
                
                std::cout << "Sending OrderCancelRequest: " << cancelRequest << std::endl;
                FIX::Session::sendToTarget(cancelRequest, sessionID);
            }            
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