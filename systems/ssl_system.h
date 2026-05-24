#include "target_system.h"

class SshSystem : public TargetSystem {
private:
    void* m_session = nullptr; 

public:
    SshSystem(const std::string& host, const std::string& user) { /* Store credentials */ }
    
    bool initialize() override { /* Connect and authenticate */ return true; }
    void disconnect() override { /* Close session */ }
    std::string executeCommand(const std::string& command) override {
        return "ssh_output"; 
    }
    /*
    bool findOpenFoam() override { return true; }
    QStringList getTutorials(QString path) {}
    */
};