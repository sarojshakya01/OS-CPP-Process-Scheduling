#include <iostream>
#include <fstream>
#include <string>
#include <queue>

using std::cin;
using std::cout;
using std::endl;
using std::string;
using std::ifstream;
using std::queue;

struct Processes {
    int processId;
    int processTime;
    int completionTime;
    int currentState;
    int device;
    queue<string> event;
    queue<int> eventTime;
};
class Operations {
public:
    enum processState { NEWPROCESS, RUNNING, READY, BLOCKED, TERMINATED };
    enum device { NEW, CORE, DISK, USER};
    queue<Processes> processes;
    queue<Processes> readyQueue;
    queue<Processes> diskQueue;
    queue<Processes> userQueue;
    void input();
    void checkinput(queue<Processes> processes);
    void arrivals();
    void coreRequest();
    void coreRelease();
    void diskRequest();
    void diskRelease();
    void userRequest();
    void userRelease();
    void terminatedProcess();
    void print(queue<Processes> proc);
};

int main(int argc, const char * argv[]) {
    Operations o;
    o.input();
    o.checkinput(o.processes);
    o.arrivals();
    return 0;
}

void Operations::arrivals() {
    while (!processes.empty()) {
        cout << "-- ARRIVAL event for process " << processes.front().processId << " at time " << processes.front().processTime << " ms" << endl;
        cout << "-- Process " << processes.front().processId << " starts at time " << processes.front().processTime << " ms" << endl;
        cout << "-- Process " << processes.front().processId << " requests a core at time " << processes.front().processTime << " ms for " << processes.front().eventTime.front() << " ms" << endl;
        if (processes.front().processId == 0) {
            int completionTemp = processes.front().completionTime + processes.front().eventTime.front();
            cout << "-- Process " << processes.front().processId << " will release a core at time " << completionTemp << " ms" << endl << endl;
            processes.front().currentState = RUNNING;
            readyQueue.push(processes.front());
        }
        else {
            cout << "-- Process " << processes.front().processId << " must wait for a core " << endl;
            cout << "-- Ready queue now contains " << readyQueue.size() << " process(es) waiting for a core" << endl << endl;
            if (processes.front().processId == 1) {
                processes.front().currentState = READY;
            }
            else {
                processes.front().currentState = BLOCKED;
            }
            readyQueue.push(processes.front());
        }
        processes.pop();
    }
    processes = readyQueue;
    processes.front().device = CORE;
    coreRelease();
}

void Operations::coreRequest(){
    queue<Processes> temp = readyQueue;
    if (!temp.empty()) {
        cout << "-- Process " << readyQueue.front().processId << " must wait for a core" << endl;
        temp.pop();
        cout << "-- Ready Queue now contains " << readyQueue.size() << " process(es) waiting for a core" << endl << endl;
    }
    temp = readyQueue;
    while (!temp.empty()) {
        if (temp.front().currentState == READY) {
            readyQueue.front().currentState = RUNNING;
        }
        else {
            temp.front().currentState = BLOCKED;
        }
        temp.pop();
    }
    coreRelease();
}

void Operations::coreRelease(){
    queue<Processes> temp = processes;
    queue<Processes> rq;
    int counter = 0;
    while (!temp.empty()) {
        while (!readyQueue.empty()) {
            if (readyQueue.front().currentState == RUNNING) {
                temp.front().completionTime += temp.front().eventTime.front();
                cout << "-- CORE completion event for process " << temp.front().processId << " at time " << temp.front().completionTime << endl;
                int accTime = temp.front().processTime + temp.front().eventTime.front();
                readyQueue.pop();
                //readyQueue.front().currentState = RUNNING;
                temp.front().event.pop();
                temp.front().eventTime.pop();
                readyQueue.front().completionTime = readyQueue.front().eventTime.front() + accTime;
                cout << "-- Process " << readyQueue.front().processId << " will release a core at time " << readyQueue.front().completionTime << " ms" << endl;
                processes = temp;
            }
            else if (readyQueue.front().currentState == READY) {
                readyQueue.front().currentState = RUNNING;
                rq.push(readyQueue.front());
                readyQueue.pop();
            }
            else if (readyQueue.front().currentState == BLOCKED) {
                if (counter == 0) {
                    readyQueue.front().currentState = READY;
                    rq.push(readyQueue.front());
                    ++counter;
                }
                else {
                    rq.push(readyQueue.front());
                }
                readyQueue.pop();
            }
        }
        temp.pop();
    }
    readyQueue = rq;
    temp = processes;
    while (!temp.empty()) {
        if (temp.front().event.front() == "DISK") {
            temp.front().currentState = READY;
            diskQueue.push(temp.front());
            processes.front().device = DISK;
            diskRequest();
            temp.pop();
        }
        else if (temp.front().event.front() == "USER") {
            temp.front().currentState = READY;
            userQueue.push(temp.front());
            processes.front().device = USER;
            userRequest();
            temp.pop();
        }
        else {
            temp.pop();
        }
    }
    terminatedProcess();
}

void Operations::diskRequest(){
    queue<Processes> temp = diskQueue;
    while (!temp.empty()) {
        if (temp.front().currentState == READY) {
            cout << "-- Process " << temp.front().processId << " requests disk access at time " << temp.front().completionTime << " ms for " << temp.front().eventTime.front() << " ms" << endl;
            diskQueue.front().currentState = RUNNING;
            temp.front().completionTime += temp.front().eventTime.front();
            cout << "-- Process " << temp.front().processId << " will release the disk at time " << temp.front().completionTime << " ms" << endl << endl;
        }
        temp.pop();
    }
    diskRelease();
}

void Operations::diskRelease(){
    queue<Processes> temp = processes;
    queue<Processes> dq;
    int counter = 0;
    while (!temp.empty()) {
        while (!diskQueue.empty()) {
            if (diskQueue.front().currentState == RUNNING) {
                temp.front().completionTime += temp.front().eventTime.front();
                cout << "-- DISK completion event for process " << temp.front().processId << " at time " << temp.front().completionTime << endl;
                //diskQueue.pop();
                //diskQueue.front().currentState = RUNNING;
                diskQueue.front().completionTime += diskQueue.front().eventTime.front();
                temp.front().event.pop();
                temp.front().eventTime.pop();
                cout << "-- Process " << diskQueue.front().processId << " requests a core at time " << diskQueue.front().completionTime << " ms for " << temp.front().eventTime.front() <<" ms" << endl;
                processes = temp;
            }
            else if (diskQueue.front().currentState == READY) {
                diskQueue.front().currentState = RUNNING;
                dq.push(diskQueue.front());
                diskQueue.pop();
            }
            else if (diskQueue.front().currentState == BLOCKED) {
                if (counter == 0) {
                    diskQueue.front().currentState = READY;
                    dq.push(diskQueue.front());
                    ++counter;
                }
                else {
                    dq.push(diskQueue.front());
                }
                diskQueue.pop();
            }
            else {
                diskQueue.pop();
            }
        }
        temp.pop();
    }
    diskQueue = dq;
    coreRequest();
}

void Operations::userRequest(){
    queue<Processes> temp = userQueue;
    while (!temp.empty()) {
        if (temp.front().currentState == READY) {
            cout << "-- Process " << temp.front().processId << " requests disk access at time " << temp.front().completionTime << " ms for " << temp.front().eventTime.front() << " ms" << endl;
            userQueue.front().currentState = RUNNING;
            temp.front().completionTime += temp.front().eventTime.front();
            cout << "-- Process " << temp.front().processId << " will release the disk at time " << temp.front().completionTime << " ms" << endl << endl;
        }
        temp.pop();
    }
    userRelease();
}

void Operations::userRelease(){
    queue<Processes> temp = processes;
    queue<Processes> uq;
    int counter = 0;
    while (!temp.empty()) {
        while (!userQueue.empty()) {
            if (userQueue.front().currentState == RUNNING) {
                temp.front().completionTime += temp.front().eventTime.front();
                cout << "-- DISK completion event for process " << temp.front().processId << " at time " << temp.front().completionTime << endl;
                //diskQueue.pop();
                //diskQueue.front().currentState = RUNNING;
                userQueue.front().completionTime += userQueue.front().eventTime.front();
                temp.front().event.pop();
                temp.front().eventTime.pop();
                cout << "-- Process " << userQueue.front().processId << " requests a core at time " << userQueue.front().completionTime << " ms for " << temp.front().eventTime.front() <<" ms" << endl;
                processes = temp;
                userQueue.pop();
            }
            else if (userQueue.front().currentState == READY) {
                userQueue.front().currentState = RUNNING;
                uq.push(userQueue.front());
                userQueue.pop();
            }
            else if (userQueue.front().currentState == BLOCKED) {
                if (counter == 0) {
                    userQueue.front().currentState = READY;
                    uq.push(userQueue.front());
                    ++counter;
                }
                else {
                    uq.push(userQueue.front());
                }
                userQueue.pop();
            }
        }
        temp.pop();
    }
    userQueue = uq;
    coreRequest();
}

void Operations::terminatedProcess(){
    while (!processes.empty()) {
        if (processes.front().event.size() == 0) {
            cout << "Process " << processes.front().processId << " terminates at time " << processes.front().processTime << " ms" << endl;
        }
        processes.pop();
    }
}

void Operations::input(){
    int processIndex = 0;
    string keyword;
    int time;
    while (cin >> keyword >> time) {
        if (keyword == "NEW") {
            processes.push(Processes());
            processes.back().processId = processIndex++;
            processes.back().processTime = time;
            processes.back().completionTime = time;
            processes.back().currentState = NEWPROCESS;
            processes.back().device = NEW;
        }
        else {
            processes.back().event.push(keyword);
            processes.back().eventTime.push(time);
        }
    }
}

void Operations::checkinput(queue<Processes> processes){
    int errors = 0;
    while (!processes.empty()) {
        if (processes.front().event.front() != "CORE") {
            cout << "ERROR: Invalid operation" << endl;
            cout << "Enter valid file." << endl;
            input();
            break;
        }
        processes.pop();
    }
}

void Operations::print(queue<Processes> proc){
    while (!proc.empty()) {
        cout << proc.front().processId << "\t" << proc.front().processTime << "\t" << proc.front().currentState << "\t" << proc.front().completionTime << "\t" << proc.front().device << endl;
        while (!proc.front().event.empty()) {
            cout << proc.front().event.front() << "\t" << proc.front().eventTime.front() << endl;
            proc.front().event.pop();
            proc.front().eventTime.pop();
        }
        proc.pop();
    }
}