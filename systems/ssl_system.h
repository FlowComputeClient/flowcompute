// Copyright 2026 FlowCompute LLC
//
// This file is part of FlowCompute.
//
// FlowCompute is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// FlowCompute is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with FlowCompute. If not, see <https://www.gnu.org/licenses/>.

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