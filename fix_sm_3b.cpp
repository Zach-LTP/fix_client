#include <quickfix/Application.h>
#include <quickfix/FileStore.h>
#include <quickfix/SocketInitiator.h>
#include <quickfix/Log.h>
#include <quickfix/SessionSettings.h>
#include <quickfix/fix44/NewOrderSingle.h>
#include <quickfix/fix44/ExecutionReport.h>
#include <quickfix/fix44/OrderCancelRequest.h>

#include <iostream>
#include <memory>
#include <string>

class MyApplication : public FIX::Application {
public:
    void onCreate(const FIX::SessionID& sessionID) override {
        std::cout << "Session created: " << sessionID << std::endl;
    }

    void onLogon(const FIX::SessionID& sessionID) override {
        std::cout << "Logon: " << sessionID << std::endl;
    }

    void onLogout(const FIX::SessionID& sessionID) override {
        std::cout << "Logout: " << sessionID << std::endl;
    }

    void toAdmin(FIX::Message& message, const FIX::SessionID& sessionID) override {
        // std::cout << "Sending admin message: " << message << std::endl;
        if (message.getHeader().getField(FIX::FIELD::MsgType) == FIX::MsgType_Logon) {
            message.setField(FIX::Username("4ab47a03-9625-4430-8a3d-e2d35ee7d0b4"));
            message.setField(FIX::Password("Oa3CZFRyWZLp54hpGA2D"));
            message.setField(FIX::ResetSeqNumFlag(false)); // 设置 141=N
            message.getHeader().setField(FIX::MsgSeqNum(1)); // 设置 34=1
        }
    }

    void toApp(FIX::Message& message, const FIX::SessionID& sessionID) override {
        //std::cout << "Sending application message: " << message << std::endl;
        //printMessageFields(message);
    }

    void fromAdmin(const FIX::Message& message, const FIX::SessionID& sessionID) override {
        //std::cout << "Received admin message: " << message << std::endl;
    }

    void fromApp(const FIX::Message& message, const FIX::SessionID& sessionID) override {
        //std::cout << "Received application message: " << message << std::endl;
        //printMessageFields(message);
        std::string msgType = message.getHeader().getField(FIX::FIELD::MsgType);
        if (msgType == FIX::MsgType_ExecutionReport) {
            FIX44::ExecutionReport executionReport(message);
            std::cout << "Received ExecutionReport: " << executionReport << std::endl;

            FIX::OrdStatus ordStatus;
            executionReport.get(ordStatus);

           if (ordStatus == FIX::OrdStatus_NEW) {
                std::cout << "New order acknowledged." << std::endl;
            } else if (ordStatus == FIX::OrdStatus_FILLED) {
                std::cout << "Order fully filled." << std::endl;
                // 处理完全成交的订单
            } else if (ordStatus == FIX::OrdStatus_REJECTED) {
                std::cout << "Order rejected: " << executionReport.getField(FIX::FIELD::Text) << std::endl;
            }
        }
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