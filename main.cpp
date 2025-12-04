#include <iostream>
#include <vector>
#include <list>
#include <cstdlib>
#include <ctime>
#include <string>

using namespace std;

// GLOBAL VARIABLES & STRUCTURES ---

enum Status { READY, BLOCKED, FINISHED };

struct Process {
    int id;
    int program_counter;
    int type;
    Status status;
};

struct SimSemaphore {
    int value;
    list<int> wait_queue;
    string name;
};

// Shared Data
int active_readers = 0; // track readers in critical section, case 5 ++ case 6 --
int active_writers = 0; // track writers in critical section, case 1 ++ case 2 --
int read_count = 0; // tracks how many enter/exit readers in critical section, if == 1 blocks writers, else == 0 allows writer

// Processes Array
Process processes[6];

// Simulated Semaphores
SimSemaphore read_count_lock = {1, {}, "read_count_lock"};
SimSemaphore wrt = {1, {}, "wrt"};
SimSemaphore reader_limiter = {2, {}, "reader_limiter"};

// ---  SimSemaphore FUNCTIONS ---

bool SemWait(SimSemaphore &sem, int pid) {
    sem.value--;
    if (sem.value < 0) {
        sem.wait_queue.push_back(pid);
        processes[pid].status = BLOCKED;
        return false;
    }
    return true;
}

void check_panic() { // Checks if the Critical Section rules are being violated.
    // Rule 1: No writer and reader together.|| // Rule 2: No two writers together. || // Rule 3: Max 2 readers.
    if (active_writers > 1 || (active_writers > 0 && active_readers > 0) || active_readers > 2) {
        cout << "\n***************************************************" << endl;
        cout << "PANIC: Synchronization Rules Violated!" << endl;
        cout << "Active Writers: " << active_writers << endl;
        cout << "Active Readers: " << active_readers << endl;
        cout << "***************************************************\n" << endl;
    }
}


// --- SIGNAL OPERATION (V) ---
void SemSignal(SimSemaphore &sem) {
    sem.value++;

    if (sem.value <= 0) {
        // Someone is waiting: Wake them up
        if (!sem.wait_queue.empty()) {
            int wakeup_pid = sem.wait_queue.front();
            sem.wait_queue.pop_front();

            processes[wakeup_pid].status = READY;
            // ***  Advances thread ***
            processes[wakeup_pid].program_counter++;
            // cout << "Process " << wakeup_pid << " UNBLOCKED from " << sem.name << endl;
        }
    }
}
// --- WORKER FUNCTIONS ---

// Function for WRITERS
void run_writer(int pid) {
    Process &current_process = processes[pid];
    switch (current_process.program_counter) {
        case 0: // Request Entry
            if (SemWait(wrt, pid)) current_process.program_counter++;
            break;
        case 1: // Instruction: CRITICAL SECTION (Writing)
            active_writers++;

            // REQUIREMENT: Report other readers/writers
            cout << "Writer " << pid << " enters. "
                 << "Other Readers: " << active_readers
                 << ", Other Writers: " << (active_writers - 1) << endl;

            cout << "Writer " << pid << " is WRITING." << endl;

            // --- PANIC CHECK ---
            check_panic();

            current_process.program_counter++;
            break;
        case 2: // Exit Critical Section
            active_writers--;
            SemSignal(wrt);
            current_process.program_counter++;
            break;
        case 3: // Finish
            cout << "Writer " << pid << " finished." << endl;
            current_process.status = FINISHED;
            break;
    }
}

//////// Function for READERS ////////
void run_reader(int pid) {
    Process &current_process = processes[pid];
    switch (current_process.program_counter) {

        case 0: // Check Reader Limit (Max 2) [cite: 6]
            if (SemWait(reader_limiter, pid)) current_process.program_counter++;
            break;

        case 1: // Lock read_count
            if (SemWait(read_count_lock, pid)) current_process.program_counter++;
            break;

        case 2: // Increment read_count
            read_count++;
            current_process.program_counter++;
            break;

        case 3: // First reader locks writer
            if (read_count == 1) {
                // If we get the lock, we move manually.
                // If we BLOCK, SemSignal will move us when we wake ucurrent_process.
                if (SemWait(wrt, pid)) {  current_process.program_counter++;}
            } else { current_process.program_counter++; }
            break;

        case 4: // Release read_count lock
            SemSignal(read_count_lock);
            current_process.program_counter++;
            break;

        case 5: // Instruction: CRITICAL SECTION (Reading)
            active_readers++;

            // REQUIREMENT: Report other readers/writers
            cout << "Reader " << pid << " enters. "
                 << "Other Readers: " << (active_readers - 1)
                 << ", Other Writers: " << active_writers << endl;

            cout << "Reader " << pid << " is READING." << endl;

            // --- PANIC CHECK ---
            check_panic();
            current_process.program_counter++;
            break;

        case 6: // Exit CS
            active_readers--;
            current_process.program_counter++;
            break;

        case 7: // Lock read_count for exit
            if (SemWait(read_count_lock, pid)) current_process.program_counter++;
            break;

        case 8: // Decrement read_count
            read_count--;
            current_process.program_counter++;
            break;

        case 9: // Last reader releases writer
            if (read_count == 0) SemSignal(wrt);
            current_process.program_counter++;
            break;

        case 10: // Release read_count lock
            SemSignal(read_count_lock);
            current_process.program_counter++;
            break;

        case 11: // Release slot for other readers
            SemSignal(reader_limiter);
            current_process.program_counter++;
            break;

        case 12: // Finish

            cout << "Reader " << pid << " finished." << endl;
            current_process.status = FINISHED;
            break;
    }
}

// SCHEDULER ---

int main() {
    srand(time(0));

    // Initialize Processes
    for(int i=0; i<6; i++) {
        processes[i].id = i;
        processes[i].program_counter = 0;
        processes[i].status = READY;
        processes[i].type = (i < 3) ? 0 : 1; // 0-2=Reader, 3-5=Writer
    }

    int completed = 0;
    while (completed < 6) {
        // Pick random process
        int pid = rand() % 6;

        // Only run if READY
        if (processes[pid].status == READY) {
            if (processes[pid].type == 0) {
                run_reader(pid);
            } else {
                run_writer(pid);
            }

            // Update completion count
            if (processes[pid].status == FINISHED) {
                completed++;
                //  mark as BLOCKED
                processes[pid].status = BLOCKED; // Remove from scheduling
            }
        }
    }

    cout << "DONE !!!" << endl;
    return 0;
}