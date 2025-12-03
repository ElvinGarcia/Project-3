#include <iostream>
#include <vector>
#include <list>
#include <cstdlib>
#include <ctime>
#include <string>

using namespace std;

// --- 1. GLOBAL VARIABLES & STRUCTURES ---

enum Status { READY, BLOCKED, FINISHED };

struct Process {
    int id;
    int pc;
    int type;           // 0 = Reader, 1 = Writer
    Status status;
};

struct Semaphore {
    int value;
    list<int> wait_queue;
    string name;
};

// Global Shared Data
int active_readers = 0;
int active_writers = 0;
int read_count = 0;

// Global Processes Array
Process processes[6];

// Global Semaphores
Semaphore mutex = {1, {}, "mutex"};
Semaphore wrt = {1, {}, "wrt"};
Semaphore reader_limiter = {2, {}, "reader_limiter"};

// --- 2. SEMAPHORE FUNCTIONS ---

bool SemWait(Semaphore &sem, int pid) {
    sem.value--;
    if (sem.value < 0) {
        sem.wait_queue.push_back(pid);
        processes[pid].status = BLOCKED;
        return false;
    }
    return true;
}

// --- SIGNAL OPERATION (V) ---
void SemSignal(Semaphore &sem) {
    sem.value++;

    if (sem.value <= 0) {
        // Someone is waiting: Wake them up
        if (!sem.wait_queue.empty()) {
            int wakeup_pid = sem.wait_queue.front();
            sem.wait_queue.pop_front();

            processes[wakeup_pid].status = READY;

            // *** FIX: Advance their PC so they don't Wait again ***
            processes[wakeup_pid].pc++;

            // cout << "Process " << wakeup_pid << " UNBLOCKED from " << sem.name << endl;
        }
    }
}
// --- 3. WORKER FUNCTIONS ---

// Function for WRITERS
void run_writer(int pid) {
    Process &p = processes[pid];
    switch (p.pc) {
        case 0: // Request Entry
            if (SemWait(wrt, pid)) p.pc++;
            break;
        case 1: // Critical Section
            active_writers++;
            cout << "Writer " << pid << " is WRITING." << endl;
            // Panic Check [cite: 10, 12]
            if (active_readers > 0 || active_writers > 1)
                cout << "PANIC: Rules violation! (Writer with others)" << endl;
            p.pc++;
            break;
        case 2: // Exit Critical Section
            active_writers--;
            SemSignal(wrt);
            p.pc++;
            break;
        case 3: // Finish
            cout << "Writer " << pid << " finished." << endl;
            p.status = FINISHED;
            break;
    }
}

// Function for READERS
void run_reader(int pid) {
    Process &p = processes[pid];
    switch (p.pc) {
        case 0: // Check Reader Limit (Max 2) [cite: 6]
            if (SemWait(reader_limiter, pid)) p.pc++;
            break;
        case 1: // Lock read_count
            if (SemWait(mutex, pid)) p.pc++;
            break;
        case 2: // Increment read_count
            read_count++;
            p.pc++;
            break;
        case 3: // First reader locks writer
            if (read_count == 1) {
                // If we get the lock, we move manually.
                // If we BLOCK, SemSignal will move us when we wake up.
                if (SemWait(wrt, pid)) {
                    p.pc++;
                }
            } else {
                p.pc++;
            }
            break;case 4: // Release read_count lock
            SemSignal(mutex);
            p.pc++;
            break;
        case 5: // Critical Section
            active_readers++;
            cout << "Reader " << pid << " is READING." << endl;
            // Panic Check [cite: 10, 12]
            if (active_writers > 0 || active_readers > 2)
                cout << "PANIC: Rules violation! (Writer present or >2 readers)" << endl;
            p.pc++;
            break;
        case 6: // Exit CS
            active_readers--;
            p.pc++;
            break;
        case 7: // Lock read_count for exit
            if (SemWait(mutex, pid)) p.pc++;
            break;
        case 8: // Decrement read_count
            read_count--;
            p.pc++;
            break;
        case 9: // Last reader releases writer
            if (read_count == 0) SemSignal(wrt);
            p.pc++;
            break;
        case 10: // Release read_count lock
            SemSignal(mutex);
            p.pc++;
            break;
        case 11: // Release slot for other readers
            SemSignal(reader_limiter);
            p.pc++;
            break;
        case 12: // Finish
            cout << "Reader " << pid << " finished." << endl;
            p.status = FINISHED;
            break;
    }
}

// --- 4. MAIN SCHEDULER ---

int main() {
    srand(time(0)); // [cite: 14]

    // Initialize Processes
    for(int i=0; i<6; i++) {
        processes[i].id = i;
        processes[i].pc = 0;
        processes[i].status = READY;
        processes[i].type = (i < 3) ? 0 : 1; // 0-2=Reader, 3-5=Writer
    }

    int completed = 0;
    while (completed < 6) {
        // Pick random process [cite: 14]
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
                // Hack: mark as BLOCKED
                processes[pid].status = BLOCKED; // Remove from scheduling
            }
        }
    }

    cout << "All processes completed." << endl;
    return 0;
}