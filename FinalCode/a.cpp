/*******************************************************
    Name:        
    ID:          
    Subject:     Operating System 3360
    Due Date:    Feburary 20, 2020
    Description: Process Scheduling.

********************************************************/
#include <iostream>

#include <fstream>

#include <string>

#include <iomanip>

#include <list>

#include <queue>

using namespace std;

// command structure for core, ssd, tty etc.
typedef struct command {
  int timeNeeded = 0;
  string name = "";
} command;

// process structure
typedef struct process {
  string name;
  bool inQueue = false;
  string status = "";
  int totalTime = 0;
  list < command > commandList;
} process;

// core structure
typedef struct core{
  process * inUseBY = nullptr;
  bool occupied = false;
  int totalTime = 0;
} core;

// ssd structure
typedef struct ssd {
  bool occupied = false;
  int totalTimeUse = 0;
  process * inUseBY = nullptr;
} ssd;

//tty structure
typedef struct tty {
  int totalTimeUse = 0;
  list < process * > occupiedBY;
} tty;

// process Scheduling class
class processSchedule {
  private:
    int finalTime;
    int totalCore;
    int ssdAccesses = 0;
    int totalCoreTime = 0;

    ifstream inFile;
    ofstream outFile;

    ssd mySSD;
    tty myTTY;
    core* myCore;
    
    list < process > myProcess;
    list < process > finishedProcess;
    
    queue < process * > nonItActQueue;
    queue < process * > itActQueue;
    queue < process * > ssdQueue;

    bool createCore();
    int getTime(string);
    int availCoreIDX();
    void preparePIDList();
    void startScheduling();
    void scheduleCore(process &, string);
    void scheduleSSD(process *, string);
    void scheduleTTY(process &, string);
    void updateTime(int);
    void removeProcess(list < process > & , process & );
    void removeProcessPtr(list < process * > & , process & );
    void clearCore(process & );
    void printCommand();
    void printPT();
    void exportSummary();
    string getCommand(string);
    process & findNextProcess();
    
  public:
    processSchedule(string, string);
    ~processSchedule();
    void start();
};

processSchedule::processSchedule(string inFileName, string outFileName) {
  inFile.open(inFileName);
  outFile.open(outFileName);
  finalTime = 0;
  totalCore= 0;
  myCore= nullptr;

  // if files did not ope, exit the program
  if (inFile.fail() || outFile.fail()) {
    cout << "ERROR: Invalid Input/Output File Name!" << endl;
    exit(0);
  }
}

// Destructor to close files and delete everything
processSchedule::~processSchedule() {
  inFile.close();
  outFile.close();
  delete myCore;
}

// Function to get the number of cores from the first line of a file  and creates an array of those cores
bool processSchedule::createCore() {

  bool coreCreated;
  
  string fileLine;

  getline(inFile, fileLine);

  if (this -> getCommand(fileLine) == "NCORES") {
    totalCore = getTime(fileLine);
    this -> myCore = new core[totalCore];
    coreCreated = true;
  } else {
    cout << "ERROR: Invalid CORE number in file" << endl;
    coreCreated = false;
  }
  return coreCreated;
}

// Function to get time from the command string in input file
int processSchedule::getTime(string fileLine) {
  string time = "";

  for (int i = 0; i < fileLine.size(); i++) {
    if (isdigit(fileLine[i])) {
      time += fileLine[i];
    }
  }
  return stoi(time);
}

// Function to check the index of core in the core array and return the index
int processSchedule::availCoreIDX() {
  int idx = -1;

  for (int i = 0; i < totalCore; i++) { 
    if (myCore[i].occupied == false) {
      idx = i;
      break;
    }
  }
  return idx;
}

// Functon to save PIDs and corresponding instructions into the a PID list.
void processSchedule::preparePIDList() {
  string fileLine;
  string cmnd;
  int time;
  // get Process Name and time Needed
  while (getline(inFile, fileLine)) {

    cmnd = this -> getCommand(fileLine);

    if (cmnd == "START") {

      time = this -> getTime(fileLine);
      // Create the command for start
      command newCmnd;
      newCmnd.name = cmnd;
      newCmnd.timeNeeded = time;
      // Read in the new PID
      getline(inFile, fileLine);
      cmnd = this -> getCommand(fileLine);
      time = this -> getTime(fileLine);

      // Creat the new PID
      process newProcess;
      newProcess.name = to_string(time);
      newProcess.commandList.push_back(newCmnd);

      // Add the new
      myProcess.push_front(newProcess);

    } else if (cmnd != "END") {
      time = this -> getTime(fileLine);
      command newCmnd;
      newCmnd.name = cmnd;
      newCmnd.timeNeeded = time;
      myProcess.front().commandList.push_back(newCmnd);
    }
  }
}

// Function to start Scheduling
void processSchedule::startScheduling() {
  process * nexttemp, * nextProcess;// = & findNextProcess();
  string prevCommand;
int count = 0;
  while (!myProcess.empty()) {
    count++;
    nextProcess = & findNextProcess();
    nexttemp = nextProcess;
    prevCommand = nextProcess -> commandList.front().name;
    
    // cout << count << " " << nexttemp -> commandList.front().name<<endl;
    if (nextProcess -> commandList.front().name == "START") {
      outFile << "Process " << nextProcess -> name << " starts at time ";
    	outFile << nextProcess -> commandList.front().timeNeeded << " ms";
    
      this -> printPT();
      outFile << endl << endl;
      updateTime(nextProcess -> commandList.front().timeNeeded);
      
      nextProcess -> commandList.pop_front();
      
      if (nextProcess -> commandList.empty()) {
      	this -> printPT();
        outFile << "Process: " << nextProcess -> name << " terminates at time ";
        outFile << finalTime << " ms.";
        this -> finishedProcess.push_back( * nextProcess);
        this -> removeProcess(myProcess, finishedProcess.front());
        this -> clearCore( * nextProcess);
      } else if (nextProcess -> commandList.front().name == "CORE") {
      	// printP();
        scheduleCore( * nextProcess, prevCommand);
      } else if (nextProcess -> commandList.front().name == "SSD") {
        scheduleSSD(nextProcess, prevCommand);
      } else if (nextProcess -> commandList.front().name == "TTY") {
        scheduleTTY( * nextProcess, prevCommand);
      }
    } else if (prevCommand != "END") {
    	// Increment the time for all dependent times
      updateTime(nextProcess -> commandList.front().timeNeeded);
      nextProcess -> commandList.pop_front();
      
      if (nextProcess -> commandList.empty()) {
      	nextProcess -> status = "TERMINATED";
        outFile << "Process: " << nextProcess -> name << " terminates at time ";
        outFile << finalTime << " ms.";
        
        this -> printPT();
        outFile << endl << endl;

        this -> finishedProcess.push_front( * nextProcess);
        this -> removeProcess(myProcess, finishedProcess.front());
        this -> clearCore( * nextProcess);
        
      } else if (nextProcess -> commandList.front().name == "CORE") {
        scheduleCore( * nextProcess, prevCommand);
      } else if (nextProcess -> commandList.front().name == "SSD") {
        scheduleSSD(nextProcess, prevCommand);
      } else if (nextProcess -> commandList.front().name == "TTY") {
        scheduleTTY( * nextProcess, prevCommand);
      }
    }
    
  }
}

// Function to schedule the CORE when the next process is going to CORE from other command.
void processSchedule::scheduleCore(process & PID, string prevCmnd) {
  int idx = this -> availCoreIDX();
  
  if (idx != -1 && prevCmnd == "START") {
  	myCore[idx].totalTime += PID.commandList.front().timeNeeded;
    myCore[idx].occupied = true;
    myCore[idx].inUseBY = & PID;
    PID.status = "RUNNING";
  } else if (idx != -1 && prevCmnd == "TTY") {
    myCore[idx].totalTime += PID.commandList.front().timeNeeded;
    myCore[idx].occupied = true;
    myCore[idx].inUseBY = & PID;
    removeProcessPtr(myTTY.occupiedBY, PID);
  } else if (idx != -1 && prevCmnd == "SSD") {
    myCore[idx].totalTime += PID.commandList.front().timeNeeded;
    myCore[idx].occupied = true;
    myCore[idx].inUseBY = & PID;

    // pop the ready process into the SSD if the SSD queue is not empty
    if (!this -> ssdQueue.empty()) {
      mySSD.occupied = true;
      mySSD.inUseBY = ssdQueue.front();
      ssdQueue.front() -> inQueue = false;
      ssdQueue.pop();
      ssdAccesses++;
    } else {
      // Create a dummy to fill into core is unoccupied
      process dummy;
      dummy.name = "DUMMY";
      dummy.inQueue = true;

      mySSD.inUseBY = & dummy;
      mySSD.occupied = false;
    }
  } else if (idx == -1 && prevCmnd == "START") {
    this -> nonItActQueue.push( & PID);
    PID.inQueue = true;
    if((this -> nonItActQueue.size() < 2)){
    	PID.status = "RUNNING";
    } else {
    	PID.status = "READY";
    }
  } else if (idx == -1 && prevCmnd == "TTY") {
    this -> itActQueue.push( & PID);
    PID.inQueue = true;
    removeProcessPtr(myTTY.occupiedBY, PID);
  } else if (idx == -1 && prevCmnd == "SSD") {
    // move the process to the non-interactive queue coz core is not avail
    this -> nonItActQueue.push( & PID);
    PID.inQueue = true;
    mySSD.occupied = false;

    // pop the ready process into the SSD, if the SSD queue is not empty
    if (!this -> ssdQueue.empty()) {
      mySSD.occupied = true;
      mySSD.inUseBY = ssdQueue.front();
      ssdQueue.front() -> inQueue = false;
      ssdQueue.pop();
      ssdAccesses++;
    }
  } else if (idx == -1 && prevCmnd == "TTY") {
    removeProcessPtr(myTTY.occupiedBY, PID);
    this -> itActQueue.push( & PID);
    PID.inQueue = true;
  } else if (prevCmnd == "QUEUE") {
    myCore[idx].totalTime += PID.commandList.front().timeNeeded;
    myCore[idx].occupied = true;
    myCore[idx].inUseBY = & PID;
    myCore[idx].inUseBY -> inQueue = false;
  } else {
    cout << "ERROR: Invalid Command " << endl;
  }
}

// Function to schedule the SSD when the next process is going to CORE from other command.
void processSchedule::scheduleSSD(process * PID, string prevLoc) {
  if (prevLoc == "START") {
    mySSD.inUseBY = PID;
    mySSD.occupied = true;
    mySSD.totalTimeUse += PID -> commandList.front().timeNeeded;
    ssdAccesses++;
  } else if (prevLoc == "CORE") {
    // Create a dummy to fill into core that is empty
    process dummy;
    dummy.name = "DUMMY";
    dummy.inQueue = true;

    // Find which core is releases
    int index = -1;
    for (int i = 0; i < totalCore; i++) {
      if (myCore[i].inUseBY == PID) {
        index = i;
        break;
      }
    }

    myCore[index].inUseBY = & dummy;
    myCore[index].occupied = false;

    // Update SSD and CORE
    if (mySSD.occupied == true) {
      // SSD is occiped at the moment
      ssdQueue.push(PID);
      PID -> inQueue = true;

      if (!itActQueue.empty()) {
        scheduleCore( * itActQueue.front(), "QUEUE");
        itActQueue.pop();
      } else if (!nonItActQueue.empty()) {
        scheduleCore( * nonItActQueue.front(), "QUEUE");
        nonItActQueue.pop();
      }
    } else {
      // SSD is unoccupied
      mySSD.inUseBY = PID;
      mySSD.totalTimeUse += PID -> commandList.front().timeNeeded;
      mySSD.occupied = true;
      ssdAccesses++;

      if (!itActQueue.empty()) {
        scheduleCore( * itActQueue.front(), "QUEUE");
        itActQueue.pop();

      } else if (!nonItActQueue.empty()) {
        nonItActQueue.front() -> inQueue = false;
        scheduleCore( * nonItActQueue.front(), "QUEUE");
        nonItActQueue.pop();

      }
    }
  }
}

// Function to schedule the TTY when the next process is going to CORE from other command.
void processSchedule::scheduleTTY(process & PID, string prevLoc) {
  if (prevLoc == "START") {
    myTTY.occupiedBY.push_back( & PID);
    myTTY.totalTimeUse += PID.commandList.front().timeNeeded;
  } else if (prevLoc == "CORE") {
    // Create a dummy to fill into CORE that is unoccupied
    process dummy;
    dummy.name = "DUMMY";
    dummy.inQueue = true;

    // Find which CORE is unoccupied
    int idx = -1;
    for (int i = 0; i < totalCore; i++) {
      if (myCore[i].inUseBY == & PID) {
        idx = i;
        break;
      }
    }

    // Give it the dummy variable.
    myCore[idx].inUseBY = & dummy;
    myCore[idx].occupied = false;

    if (!itActQueue.empty()) {
      itActQueue.front() -> inQueue = false;
      scheduleCore( * itActQueue.front(), "QUEUE");
      itActQueue.pop();
    } else if (!nonItActQueue.empty()) {
      scheduleCore( * nonItActQueue.front(), "QUEUE");
      nonItActQueue.pop();
    }

    myTTY.occupiedBY.push_back( & PID);
    myTTY.totalTimeUse += PID.commandList.front().timeNeeded;
  }
}

// Function to update time
void processSchedule::updateTime(int timeTaken) {
  this -> finalTime += timeTaken;
  for (auto itr = myProcess.begin(); itr != myProcess.end(); itr++) {
    if (( * itr).inQueue == false) {
      ( * itr).commandList.front().timeNeeded -= timeTaken;
    }
  }
}

// Function to remove process from addresses-of-process list
void processSchedule::removeProcess(list < process >  & myList, process & PID) {
  for (auto itr = myList.begin(); itr != myList.end(); itr++) {
    if (( * itr).name == PID.name) {
      myList.erase(itr++);
      break;
    }
  }
}

// Function to remove process from pointers-of-process list
void processSchedule::removeProcessPtr(list < process * > &myList, process & PID) {
  if (!myList.empty()) {
    auto it = myList.begin();
    if (it++ == myList.end()) {
      myList.pop_front();
    } else {
      for (auto itr = myList.begin(); itr != myList.end(); itr++) {
        if (( * itr) -> name == PID.name) {
          myList.erase(itr++);
          break;
        }
      }
    }
  } else {
    exit(0);
  }
}

// Funton to clear the core
void processSchedule::clearCore(process & PID) {
  process dummy;
  dummy.name = "DUMMY";
  dummy.inQueue = true;
  int idx = 0;
  for (int i = 0; i < totalCore; i++) {
    if (myCore[i].inUseBY == & PID) {
      idx = i;
    }
  }

  myCore[idx].inUseBY = & dummy;
  myCore[idx].occupied = false;

  if (!itActQueue.empty()) {
    scheduleCore( * itActQueue.front(), "QUEUE");
    itActQueue.pop();
  } else if (!nonItActQueue.empty()) {
    scheduleCore( * nonItActQueue.front(), "QUEUE");
    nonItActQueue.pop();
  }
}

// Funtion to display CORE, SSD, and TTY information
void processSchedule::printCommand() {
  bool coreOccupied = false;
  // Loop through the coreto display the corethat are occupied
  outFile << "\nCORE Information: " << endl;
  for (int i = 0; i < totalCore; i++) {
    if (myCore[i].occupied == true) {
      outFile << "CORE Number: " << (i + 1) << "\nOccupied by PID: " << myCore[i].inUseBY -> name << endl;
      coreOccupied = true;
    }
  }
  if (coreOccupied == false) {
    outFile << "CORE is unoccupied." << endl;
  }

  outFile << "\nSSD Information: " << endl;
  if (mySSD.occupied) {
    outFile << "Occupied by PID: " << mySSD.inUseBY -> name;
  } else {
    outFile << "SSD is unoccupied." << endl;
  }

  // Print all the items in the list of tty if is not empty
  outFile << "\nTTY Information: " << endl;
  if (!myTTY.occupiedBY.empty()) {
    for (auto itr = myTTY.occupiedBY.begin(); itr != myTTY.occupiedBY.end(); itr++) {
      outFile << "Occupied by PID: " << ( * itr) -> name << endl;
      outFile << "Occupied Time:" << ( * itr) -> commandList.front().timeNeeded << endl << endl;
    }
  } else {
    outFile << "TTY is unoccupied." << endl << endl;
  }
}


// Function to print what is in the queue
void processSchedule::printPT() {
	bool active = false;
	outFile << endl << "Process Table:";
	for (auto itr = myProcess.begin(); itr != myProcess.end(); ++itr) {
    if ((*itr).status != ""){
    	active = true;
      outFile << endl << "Process " << (*itr).name << " is " << (*itr).status << ".";
    }
  }
  if (not active) {
  	outFile << endl << "There are no active processes.";
  }
}

// Function to export all the event summary to a file
void processSchedule::exportSummary() {
  for (int i = 0; i < totalCore; i++) {
    totalCoreTime += myCore[i].totalTime;
  }

  outFile << "SUMMARY:\n";
  outFile << "Total elapsed time: " << finalTime << "ms" << endl;
  outFile << "Number of processes that completed: " << finishedProcess.size() << endl;
  outFile << "Total number of SSD access: " << ssdAccesses << endl;
  outFile << "Average number of busy CORE: " << fixed << setprecision(3) << ((double) totalCoreTime) / finalTime << endl;
  outFile << "SSD utilization: " << fixed << setprecision(3) << ((double) mySSD.totalTimeUse) / finalTime << endl;
}

// Fucntion to retrive the command from a file
string processSchedule::getCommand(string fileLine) {
  string process = "";

  for (int i = 0; i < fileLine.size(); i++) {
    if (isalpha(fileLine[i])) {
      process += fileLine[i];
    }
  }
  return process;
}

// Function to find the next process with lowest timing
process & processSchedule::findNextProcess() {
  
  process * s = & myProcess.front();

  // Find the s
  for (auto itr = myProcess.begin(); itr != myProcess.end(); ++itr) {
    if ((*itr).inQueue == false){
      s = & (*itr);
    }
  }

  // Save the smallest timing that is not in queue while iterating
  for (auto itr = myProcess.begin(); itr != myProcess.end(); ++itr) {
    if ((s -> commandList.front().timeNeeded > (*itr).commandList.front().timeNeeded) 
    	      && ((*itr).inQueue == false)) {
      s = & ( * itr);
    }
  }

  // If the smallest is in queue that means all our process are in queue, we did something wrong, EXIT.
  if (s -> inQueue) {
    cout << "ERROR: All process are in queues!" << endl;
    exit(0);
  }
  return (*s);
}

// Function to start all the tasks to be done
void processSchedule::start() {
  if (this -> createCore()) {
    this -> preparePIDList();
    this -> startScheduling();
    this -> exportSummary();
  } else {
    cout << "ERROR: Invalid Number of Core" << endl;
  }
}

int main(int argc, char const *argv[]) {
	string filecontent = "";
	string line;
	ofstream ofile;

	ofile.open("programcreatedinput.txt");
	
	
  while(getline(cin, line))
  {
  		filecontent = filecontent + line + '\n';
  }
  ofile<<filecontent;
  ofile.close();
	if (argc != 1) {
      cout << "Invalid parameters" << endl;
      exit(EXIT_FAILURE); // terminate with error
  }

  processSchedule myCore("programcreatedinput.txt", "output.txt");
  myCore.start();

  return 0;
}