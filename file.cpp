#include <iostream>
#include<fstream> 
#include <unistd.h>
#include <sys/resource.h>
#include <ctime>
#include <sys/types.h>
#include <sys/wait.h>
#include <chrono>
#include<string>
#include<cmath>
#include<cstdlib>

class Block {
private:
    Block* startaddress;
    Block* endaddress;
    int size;
    bool allocated;
    int PrevSize;
    int id;
    Block* next;

public:
    void initialiser(Block *s,int si,bool alloc,Block* next,int b)//Initialiser function to assign the private variables
    {
        startaddress=s;size=si;allocated=alloc;next=next;PrevSize=b;
        endaddress=startaddress+(size*1024)-1;
    }
    //Constructor Function
    Block(Block* s, int si, bool alloc, Block* next, int b) : startaddress(s), size(si), allocated(alloc), next(next) {
        endaddress = startaddress + (size*1024) - 1;
        PrevSize = b;
        id = 0;
    }
    //Friend function
    friend class Memory;
};

class Memory {
private:
    int size;
    int no_blocks;
    int no_processes;
    Block* head;
    Block* tail;

public:
    //Constructor funtion
    Memory(int size) : size(size), no_blocks(0), no_processes(0), head(nullptr), tail(nullptr) {}
    //Function to split the memory in powers of 2 in the form of blocks in linked list
    void splitmem() {
        int x = 2;
        while (x <= size) {
            Block* block = static_cast<Block*>(std::malloc(x *1024));
            block->initialiser(block,x,false,nullptr,x);
            //create a linked list
            if (head == nullptr) {
                head = block;
                tail = head;
            } else {
                tail->next = block;
                tail = tail->next;
            }

            x *= 2;
            no_blocks++;
        }
    }
    //A
    void allocate(int a, int id,double time) {
        if(a>pow(2,no_blocks))
        {
            std::cout<<"Cannot Allocate Memory due to large size\n";
            return;
        }
        Block* temp = head;
        Block* temp1 = nullptr; // To save the Bestfit block
        int bestused = 9999;
        while (temp != nullptr) {
            if (temp->allocated == false && a <= temp->size && bestused > temp->size) {
                temp1 = temp;
                bestused = temp->size;
            }
            temp = temp->next;
        }
        if (temp1 != nullptr) {
            if (temp1->size > a) {
                int b = temp1->size;
                int unused = temp1->size - a;
                Block* newnode = static_cast<Block*>(std::malloc(unused *1024));
                newnode->initialiser(newnode,unused,false,nullptr,b);
                Block* curr = temp1->next;
                newnode->next = curr;
                temp1->next = newnode;
            }
            Block* resizedPtr = static_cast<Block*>(std::realloc(temp1,a*1024));
            if (resizedPtr == nullptr) {
                std::cout << "Memory reallocation failed.\n" << std::endl;
            }
            temp1 = resizedPtr;
            temp1->size = a;
            temp1->allocated = true;
            temp1->id = id;
            no_processes++;
        }
        std::cout<<"Memory has been allocated successfully\n";

        //waiting for some time and then calling the destructor function
        // Convert the duration to std::chrono::duration<float, std::milli>
            std::chrono::duration<float, std::milli> duration(time) ;
            // Get the current time
            auto startTime = std::chrono::steady_clock::now();
            // Calculate the end time
            auto endTime = startTime + duration;
            // Run the loop until the end time is reached
            while (std::chrono::steady_clock::now() < endTime) 
            {
            }
    }

    void deallocate(int pid) {
        Block* temp = head;
        Block* prev = nullptr;
        int flag=0;
        while (temp != nullptr) {
            if (temp->id == pid) {
                flag=1;
                temp->allocated = false;
                temp->id=0;
                no_processes--;
                if ((temp->next != nullptr && temp->next->PrevSize == temp->PrevSize && temp->next->allocated == false)) {
                    temp->size += temp->next->size;
                    Block* nextNext = temp->next->next;
                    delete temp->next;
                    temp->next = nextNext;
                }

                if (prev != nullptr && prev->PrevSize == temp->PrevSize && prev->allocated == false) {   
                    prev->size += temp->size;
                    prev->next = temp->next;
                    delete temp;
                    temp = prev;
                    temp->id=0;
                }
            }
            prev = temp;
            temp = temp->next;
        }
        if(temp==nullptr && flag==0)
            std::cout<<"\nProcess Id Not Found . Please enter valid";
    }

    void print() {
        Block* temp = head;
        while (temp != nullptr) {
            std::cout << "Size: " << temp->size << std::endl;
            std::cout << "Start address: " << temp->startaddress << std::endl;
            std::cout << "End address: " << temp->endaddress << std::endl;
            std::cout << "Allocated: " << temp->allocated << std::endl;
            std::cout << "prevSize: " << temp->PrevSize << std::endl;
            std::cout << "process id " << temp->id << std::endl;
            std::cout << "------------------------------" << std::endl;

            temp = temp->next;
        }
        std::cout << "Number of Process Running : " << no_processes << std::endl;
        std::cout << "No of blocks: " << no_blocks << std::endl;
    }
};

double calculateProcessTime(clock_t startTime, clock_t endTime)
{
    return static_cast<double>(endTime - startTime) / CLOCKS_PER_SEC;
}
// Function to get memory usage of the current process
size_t GetProcessMemoryUsage() {
    struct rusage usage;
    getrusage(RUSAGE_SELF, &usage);
    return usage.ru_maxrss;
}
void process_function(int& pi, int& mem, double& time)
{
    auto start_time = std::chrono::high_resolution_clock::now();
    pid_t pid = fork();
    if (pid == -1) {
        std::cerr << "Failed to fork process." << std::endl;
        return ;
    } else if (pid == 0) {
        sleep(5);
        exit(0);
    } 
    else {
        size_t parent_memory_used = GetProcessMemoryUsage();

        // Wait for the child process to finish
        int status;
        waitpid(pid, &status, 0);

        auto end_time = std::chrono::high_resolution_clock::now();
        double elapsed_time = std::chrono::duration<double>(end_time - start_time).count();
        pi=pid;
        mem=parent_memory_used;
        time=elapsed_time;
        std::cout << "Child process finished." << std::endl;
        std::cout << "Process ID: " << pid << std::endl;
        std::cout << "Parent Memory Used: " << parent_memory_used << " bytes" << std::endl;
        std::cout << "Time elapsed: " << elapsed_time << " seconds" << std::endl;
}
}

//~~~~~~~~~~~~~~~~~~~~~~FUNCTION FOR FILE~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void function(int &f,int &pid)
{
    int fileSize;//current read or write position.
    std::string filename;
    std::cout<<"\nEnter the File Name :";
    std::cin>>filename;
    //std::cout<<"\nEnter the Process Id :";
    //std::cin>>pid;
    std::ifstream file(filename, std::ios::ate);//ate=at end 
    if (file.is_open()) 
    {
        fileSize = file.tellg();
        f=static_cast<int>(fileSize);
        std::cout << "File size: " << fileSize << " bytes" << std::endl;
        pid=rand();
        std::cout << "The pid of the process is "<< pid<<std::endl;
    } else 
    {
    std::cout << "Failed to open the file." << std::endl;}
 }
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
int main() {
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~FILE~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    int choice;
    Memory m(1000);
    m.splitmem();
    m.print();
    std::cout<<"\nPlease Enter the Function Number :\n";
    std::cout<<"1.Allocation\n2.Deallocation\n3.Display\n4.Exit\n";
    std::cin>>choice;
    while(true)
    {
        switch(choice)
        {
            case 1:
                int f,pid;
                function(f,pid);
                m.allocate(f,pid,0);
                break;
            case 2: int p;
                    std::cout<<"\nEnter The Process Id :";
                    std::cin>>p;
                    m.deallocate(p);
                    break;
            case 3: std::cout<<"\n~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n";
                    m.print();
                    break;
            case 4: exit(1);
            default : std::cout<<"/n Please Enter a valid choice \n";break;

        }
        std::cout<<"Please Enter the Function Number :\n";
        std::cout<<"1.Allocation\n2.Deallocation\n3.Display\n4.Exit\n";
        std::cin>>choice;
    }

    return 0;

}