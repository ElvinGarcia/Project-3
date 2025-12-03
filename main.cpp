#include <iostream>
#include <vector>
#include <list>
#include <cstdlib>

using namespace std;

// Process States
enum Status {
    READY,
    BLOCKED,
    FINISHED
};

// Process Control Block (PCB)
struct Process {
    int id;             // 0-5
    int pc;             // Program Counter
    int type;           // 0 = Reader, 1 = Writer
    Status status;      // Current state
};

// Custom Semaphore Definition
struct Semaphore {
    int value;              // The counter
    list<int> wait_queue;   // List of Process IDs waiting on this semaphore
    string name;            // For debugging (e.g., "mutex" or "wrt")
};

// //////////////////////////////////////////////////////////////////////////////////////

// Global array of processes (assuming 6 total: 3 readers, 3 writers)
Process processes[6];

// --- WAIT OPERATION (P) ---
// Returns true if the process can proceed immediately.
// Returns false if the process was blocked.
bool SemWait(Semaphore &sem, int pid) {
    sem.value--;

    if (sem.value < 0) {
        // Resource busy: Block the process
        sem.wait_queue.push_back(pid);
        processes[pid].status = BLOCKED;

        cout << "Process " << pid << " is BLOCKED on semaphore " << sem.name << endl;
        return false; // Do not increment PC yet
    }

    // Resource acquired
    return true; // Can increment PC
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
            cout << "Process " << wakeup_pid << " unblocked from " << sem.name << endl;
        }
    }
}

// //////////////////////Integrating into the Switch Statement////////////////////////////////////////////////////////////////

// Global Semaphores
Semaphore mutex = {1, {}, "mutex"}; // Initialize to 1 (Binary)
Semaphore wrt = {1, {}, "wrt"};     // Initialize to 1 (Binary)
int read_count = 0;
Semaphore reader_limiter = {2, {}, "reader_limiter"};

void run_writer(int pid) {
    Process &p = processes[pid]; // Reference to current process

    void run_reader(int pid) {
    Process &p = processes[pid];

    switch (p.pc) {
        case 0:
            // Instruction: Wait(reader_limiter)
            // Enforces the "Up to two readers" rule
            if (SemWait(reader_limiter, pid)) {
                p.pc++;
            }
            break;

        case 1:
            // Instruction: Wait(mutex)
            // Protects the read_count variable
            if (SemWait(mutex, pid)) {
                p.pc++;
            }
            break;

        case 2:
            // Instruction: Increment read_count
            read_count++;
            p.pc++;
            break;

        case 3:
            // Instruction: First reader locks the writer
            // Logic: IF (read_count == 1) THEN Wait(wrt)
            if (read_count == 1) {
                // Try to acquire writer lock
                if (SemWait(wrt, pid)) {
                    // Success: acquired lock, move next
                    p.pc++;
                }
                // Else: SemWait returned false (blocked).
                // We stay at Case 3. Logic pauses here until unblocked.
            } else {
                // If read_count > 1, we don't need to wait on wrt
                p.pc++;
            }
            break;

        case 4:
            // Instruction: Signal(mutex)
            // Done modifying read_count, release it so other readers can enter
            SemSignal(mutex);
            p.pc++;
            break;

        case 5:
            // Instruction: CRITICAL SECTION (Reading)
            cout << "*** Reader " << pid << " is READING ***" << endl;
            active_readers++;

            // PANIC CHECK [cite: 10, 12]
            // Panic if a writer is present OR if > 2 readers (though semaphore prevents this)
            if (active_writers > 0 || active_readers > 2) {
                cout << "PANIC: Rules violation! Writer present or too many readers." << endl;
            }
            p.pc++;
            break;

        case 6:
            // Instruction: Leave CS, prepare to exit
            active_readers--;
            p.pc++;
            break;

        case 7:
            // Instruction: Wait(mutex)
            // Need to decrement read_count safely
            if (SemWait(mutex, pid)) {
                p.pc++;
            }
            break;

        case 8:
            // Instruction: Decrement read_count
            read_count--;
            p.pc++;
            break;

        case 9:
            // Instruction: Last reader releases the writer
            // Logic: IF (read_count == 0) THEN Signal(wrt)
            if (read_count == 0) {
                SemSignal(wrt);
            }
            p.pc++;
            break;

        case 10:
            // Instruction: Signal(mutex)
            SemSignal(mutex);
            p.pc++;
            break;

        case 11:
            // Instruction: Signal(reader_limiter)
            // Free up a slot for another reader
            SemSignal(reader_limiter);
            p.pc++;
            break;

        case 12:
            cout << "Reader " << pid << " finished." << endl;
            p.status = FINISHED;
            break;
    }
}
// //////////////////////The Main Loop (The Scheduler)////////////////////////////////////////////////////////////////

int main() {
    srand(time(0));

    // Initialize Processes (Example)
    for(int i=0; i<6; i++) {
        processes[i].id = i;
        processes[i].pc = 0;
        processes[i].status = READY;
        // Assign types: 0-2 are Readers, 3-5 are Writers
        processes[i].type = (i < 3) ? 0 : 1;
    }

    bool all_finished = false;
    while (!all_finished) {
        // 1. Pick a random process [cite: 14]
        int pid = rand() % 6;

        // 2. Only run if READY
        if (processes[pid].status == READY) {
            if (processes[pid].type == 0) {
                 // run_reader(pid); // You need to implement this
            } else {
                 run_writer(pid);
            }
        }
        // If BLOCKED or FINISHED, do nothing (simulates waiting)

        // 3. Check termination condition (if all processes are FINISHED)
        // ... (loop through all processes to check if status == FINISHED)
    }

    return 0;
}