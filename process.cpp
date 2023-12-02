#include <iostream>
#include <fstream> 
#include <unistd.h>
#include <sys/resource.h>
#include <ctime>
#include <sys/types.h>
#include <sys/wait.h>
#include <chrono>
#include <string>
#include <cmath>
#include <cstdlib>

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
    // Initialiser function to assign the Block Data
    void initialiser(Block* s, int si, bool alloc, Block* next, int prev_size)
    {
        startaddress = s;
        size = si;
        allocated = alloc;
        this->next = next;
        PrevSize = prev_size;
        endaddress = startaddress + (size * 1024) - 1;
    }

    // Constructor Function
    Block(Block* s, int size_given, bool alloc, Block* next, int prev_size) : startaddress(s), size(size_given), allocated(alloc), next(next) {
        endaddress = startaddress + (size * 1024) - 1;
        PrevSize = prev_size;
        id = 0;
    }

    // Friend function
    friend class Memory;
};

class Memory {
private:
    int size;
    int no_blocks; // Number of Blocks in Linked List
    int no_processes; // Number of Process which have been allocated memory
    Block* head;
    Block* tail;

public:
    // Constructor function
    Memory(int size) : size(size), no_blocks(0), no_processes(0), head(nullptr), tail(nullptr) {}

    // Function to split the memory in powers of 2 in the form of blocks in linked list
    void splitmem() {
        int x = 2;
        while (x <= size) {
            Block* block = static_cast<Block*>(std::malloc(x * 1024));
            block->initialiser(block, x, false, nullptr, x);
            // create a linked list
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

    // Function to Allocate memory for a given size
    void allocate(int given_size, int id, double time) {
        if (given_size > pow(2, no_blocks))
        {
            std::cout << "\nCannot Allocate Memory due to large size\n";
            return;
        }
        Block* temp = head;
        Block* temp1 = nullptr;
        int bestused = 99999; // To save the Bestfit block

        // To find the bestFit block for efficient allocation
        while (temp != nullptr) {
            if (temp->allocated == false && given_size <= temp->size && bestused > temp->size) {
                temp1 = temp;
                bestused = temp->size;
            }
            temp = temp->next;
        }

        // Best Fit block Found!
        if (temp1 != nullptr) {
            // If the allocated block has a size greater than the required size then split memory so that other processes can use them.
            if (temp1->size > given_size) {
                int b = temp1->size;
                int unused = temp1->size - given_size;

                // Creating a new block with the unused space
                Block* newnode = static_cast<Block*>(std::malloc(unused * 1024));
                newnode->initialiser(newnode, unused, false, nullptr, b);

                // Inserting the unused and allocated block in linked list
                Block* curr = temp1->next;
                newnode->next = curr;
                temp1->next = newnode;
            }
            // Resize the Allocated block to the given_size
            Block* resizedPtr = static_cast<Block*>(std::realloc(temp1, given_size * 1024));
            if (resizedPtr == nullptr) {
                std::cout << "\nMemory reallocation failed.\n" << std::endl;
            }
            // Updating the Necessary values.
            temp1 = resizedPtr;
            temp1->size = given_size;
            temp1->allocated = true;
            temp1->id = id;
            no_processes++;
        }
        std::cout << "\nMemory has been allocated successfully\n";
    }

    // Deallocate Memory given the process ID
    void deallocate(int pid) {
        Block* temp = head;
        Block* prev = nullptr;
        int flag = 0;
        // Loop through the linked list to search for the process ID
        while (temp != nullptr) {
            // Process ID Found
            if (temp->id == pid) {
                flag = 1;
                temp->allocated = false; // Setting allocated as false 
                temp->id = 0;
                no_processes--;

                // If There was a split in allocation and both are not allocated then join the nodes back.
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
                    temp->id = 0;
                }
            }
            prev = temp;
            temp = temp->next;
        }
        if (temp == nullptr && flag == 0)
            std::cout << "\nProcess Id Not Found . Please enter valid\n";
        else
            std::cout << "\nProcess Id Found and Deleted Successfully\n";
    }

    // Function to Print the Linked List
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

// Function to get memory usage of the current process
size_t GetProcessMemoryUsage() {
    struct rusage usage;
    getrusage(RUSAGE_SELF, &usage);
    return usage.ru_maxrss;
}

// Creating a new Process using Fork and calculating the elapsed time.
void process_function(int& pi, int& mem, double& time)
{
    auto start_time = std::chrono::high_resolution_clock::now();
    pid_t pid = fork();
    if (pid == -1) {
        std::cerr << "Failed to fork process." << std::endl;
        return;
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
        pi = pid;
        mem = parent_memory_used;
        time = elapsed_time;
        std::cout << "Child process finished." << std::endl;
        std::cout << "Process ID: " << pid << std::endl;
        std::cout << "Parent Memory Used: " << parent_memory_used << " bytes" << std::endl;
        std::cout << "Time elapsed: " << elapsed_time << " seconds" << std::endl;
    }
}

int main() {
    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~PROCESS CREATION~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    int choice;
    Memory m(10000);
    m.splitmem();
    m.print();
    std::cout << "\nPlease Enter the Function Number :\n";
    std::cout << "1.Allocation\n2.Deallocation\n3.Display\n4.Exit\n";
    std::cin >> choice;
    while (true)
    {
        int pid;
        switch (choice)
        {
        case 1:
            int mem;
            double time;
            process_function(pid, mem, time);
            m.allocate(mem, pid, time);
            printf("**************************************************************************\n");
            std::cout << "\nMemory is allocated";
            break;
        case 2:
            int p;
            std::cout << "\nThe Process Id Deleted is:";
            std::cout << pid;  // corrected '>>' instead of '>>'
            m.deallocate(pid);
            printf("***************************************************************************\n");
            break;
        case 3:
            std::cout << "\n~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n";
            m.print();
            break;
        case 4:
            exit(1);
        default:
            std::cout << "\n Please Enter a valid choice \n";
            break;
        }
        std::cout << "Please Enter the Function Number :\n";
        std::cout << "1.Allocation\n2.Deallocation\n3.Display\n4.Exit\n";
        std::cin >> choice;
    }
    return 0;
}
