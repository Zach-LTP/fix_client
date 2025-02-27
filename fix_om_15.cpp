#include <quickfix/Application.h>
#include <quickfix/FileStore.h>
#include <quickfix/SocketInitiator.h>
#include <quickfix/Log.h>
#include <quickfix/SessionSettings.h>
#include <quickfix/fix44/NewOrderSingle.h>
#include "quickfix/fix44/ExecutionReport.h"
#include "quickfix/fix44/OrderCancelReplaceRequest.h"
#include "quickfix/fix44/OrderCancelRequest.h"

#include <iostream>
#include <memory>
#include <string>
class MyApplication : public FIX::Application {
public:

    bool first_access = false;

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
        order.set(FIX::Price(100000));
        order.set(FIX::OrderQty(1));
        order.set(FIX::Symbol("BTCUSD"));
        order.set(FIX::Side(FIX::Side_BUY));
        order.set(FIX::OrdType(FIX::OrdType_LIMIT));
        order.set(FIX::TimeInForce(FIX::TimeInForce_GOOD_TILL_CANCEL));
        // order.set(FIX::TimeInForce(FIX::TimeInForce_GOOD_TILL_CANCEL));
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

    void logoutSession(const FIX::SessionID& sessionID) {
        FIX::Session* session = FIX::Session::lookupSession(sessionID);
        if (session && session->isLoggedOn()) {
            session->logout("User requested logout");
            std::cout << "Logout request sent for session: " << sessionID << std::endl;
        } else {
            std::cout << "Session not found or not logged on: " << sessionID << std::endl;
        }
    }

    void fromApp(const FIX::Message& message, const FIX::SessionID& sessionID) override {
        std::cout << "Received application message: " << message << std::endl;
        printMessageFields(message);
        std::string msgType = message.getHeader().getField(FIX::FIELD::MsgType);
        if (msgType == FIX::MsgType_ExecutionReport)
        {
            FIX44::ExecutionReport executionReport(message);
            std::cout << "Received ExecutionReport: " << executionReport << std::endl;

            FIX::OrdStatus ordStatus;
            executionReport.get(ordStatus);

           if (ordStatus == FIX::OrdStatus_NEW) {
                std::cout << "New order acknowledged." << std::endl;
                /*
                FIX::process_sleep(1);
                logoutSession(sessionID);
                */
            } else if (ordStatus == FIX::OrdStatus_FILLED) {
                std::cout << "Order fully filled." << std::endl;
                // 处理完全成交的订单
            } else if (ordStatus == FIX::OrdStatus_PARTIALLY_FILLED) {
                std::cout << "Order partially filled." << std::endl;
                // 处理部分成交的订单
                if (!first_access)
                {
                    FIX44::OrderCancelReplaceRequest replaceRequest;
                    replaceRequest.set(FIX::ClOrdID(executionReport.getField(FIX::FIELD::ClOrdID)));
                    replaceRequest.set(FIX::Symbol("BTCUSD"));
                    replaceRequest.set(FIX::Side(FIX::Side_BUY));
                    replaceRequest.set(FIX::Account("4ab47a03-9625-4430-8a3d-e2d35ee7d0b4"));
                    replaceRequest.set(FIX::TransactTime(time(NULL)));

                    // replaceRequest.set(FIX::OrigClOrdID(executionReport.getField(FIX::FIELD::OrigClOrdID)));
                    replaceRequest.set(FIX::OrigClOrdID(executionReport.getField(FIX::FIELD::ClOrdID)));
                    replaceRequest.set(FIX::OrderID(executionReport.getField(FIX::FIELD::OrderID)));
                    replaceRequest.set(FIX::Price(90000));
                    replaceRequest.set(FIX::OrderQty(1.2));
                    replaceRequest.set(FIX::OrdType(FIX::OrdType_LIMIT));
                
                    std::cout << "Sending OrderReplaceRequest: " << replaceRequest << std::endl;
                    FIX::Session::sendToTarget(replaceRequest, sessionID);

                    first_access = true;
                    /*
                    FIX::process_sleep(1);
                    logoutSession(sessionID);
                    */
                }
                else
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
                    //cancelRequest.set(FIX::Price(std::stod(executionReport.getField(FIX::FIELD::Price))));
                
                    std::cout << "Sending OrderCancelRequest: " << cancelRequest << std::endl;
                    FIX::Session::sendToTarget(cancelRequest, sessionID);
                } 
            } else if (ordStatus == FIX::OrdStatus_REJECTED) {
                std::cout << "Order rejected: " << executionReport.getField(FIX::FIELD::Text) << std::endl;
            } else if (ordStatus == FIX::OrdStatus_CANCELED) {
                std::cout << "Order canceled." << std::endl;
            } else if (ordStatus == FIX::OrdStatus_PENDING_CANCEL) {
                std::cout << "Order pending cancel." << std::endl;
            } else if (ordStatus == FIX::OrdStatus_PENDING_REPLACE) {
                std::cout << "Order pending replace." << std::endl;
            } else if (ordStatus == FIX::OrdStatus_DONE_FOR_DAY) {
                std::cout << "Order done for day." << std::endl;
            } else if (ordStatus == FIX::OrdStatus_STOPPED) {
                std::cout << "Order stopped." << std::endl;
            } else if (ordStatus == FIX::OrdStatus_EXPIRED) {
                std::cout << "Order expired." << std::endl;
            } else if (ordStatus == FIX::OrdStatus_CALCULATED) {
                std::cout << "Order calculated." << std::endl;
            } else if (ordStatus == FIX::OrdStatus_ACCEPTED_FOR_BIDDING) {
                std::cout << "Order accepted for bidding." << std::endl;
            } else if (ordStatus == FIX::OrdStatus_PENDING_NEW) {
                std::cout << "Order pending new." << std::endl;
            } else if (ordStatus == FIX::OrdStatus_PENDING_CANCEL) {
                std::cout << "Order pending cancel." << std::endl;
            } else if (ordStatus == FIX::OrdStatus_PENDING_REPLACE) {
                std::cout << "Order pending replace." << std::endl;
            } else if (ordStatus == FIX::OrdStatus_REPLACED) {
                std::cout << "Order replaced." << std::endl;
            } else if (ordStatus == FIX::OrdStatus_SUSPENDED) {
                std::cout << "Order suspended." << std::endl;
            } else if (ordStatus == FIX::OrdStatus_PENDING_NEW) {
                std::cout << "Order pending new." << std::endl;
            } else if (ordStatus == FIX::OrdStatus_PENDING_CANCEL) {
                std::cout << "Order pending cancel." << std::endl;
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