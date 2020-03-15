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

#include "ProcessSchedule.h"

int main() {
  processSchedule myCore("input11.txt", "output.txt");
  myCore.start();

  return 0;
}