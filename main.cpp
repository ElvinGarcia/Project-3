#include <iostream>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <string>

using namespace std;

//  GLOBAL VARIABLES & STRUCTURES ---

// Note: BLOCKED status is removed because CAS uses Busy Waiting (Spinning)
enum Status { READY, FINISHED };

struct Process {
    int id;
    int program_counter;
    int type;           // 0 = Reader, 1 = Writer
    Status status;
};

// Shared Data
int active_readers = 0;
int active_writers = 0;
int read_count = 0;

// *** LOCKS using integers instead of Semaphores ***
// 0 = Unlocked, 1 = Locked
int read_count_lock = 0;  // Protects read_count
int wrt = 0;              // Protects Critical Section
int reader_limiter = 2;   // Counts available slots (Starts at 2)

Process processes[6];


// --- COMPARE AND SWAP  ---


int CompareAndSwap(int &value, int expected, int new_val) {
    int temp = value;
    if (value == expected) {
        value = new_val;
    }
    return temp;
}

// Helper for panic checks
void check_panic() {
    if (active_writers > 1 || (active_writers > 0 && active_readers > 0) || active_readers > 2) {
        cout << "\n***************************************************" << endl;
        cout << "PANIC: Synchronization Rules Violated!" << endl;
        cout << "Active Writers: " << active_writers << endl;
        cout << "Active Readers: " << active_readers << endl;
        cout << "***************************************************\n" << endl;
    }
}


// --- 3. WORKER FUNCTIONS ---

void run_writer(int pid) {
    Process &current_process = processes[pid];
    switch (current_process.program_counter) {
        case 0: // Request Entry (Spinlock)
            // Try to swap 0 (unlocked) to 1 (locked)
            if (CompareAndSwap(wrt, 0, 1) == 0) {
                current_process.program_counter++;
            }
            // If CAS failed (returns 1), we stay at Case 0 and retry next turn.
            break;

        case 1: // ENTER Critical Section
            active_writers++;
            cout << "Writer " << pid << " enters. "
                 << "Other Readers: " << active_readers
                 << ", Other Writers: " << (active_writers - 1) << endl;
            current_process.program_counter++;
            break;

        case 2: // WORK
            cout << "Writer " << pid << " is WRITING." << endl;
            check_panic();
            current_process.program_counter++;
            break;

        case 3: // EXIT Critical Section
            active_writers--;
            // Unlock: Simply set back to 0
            wrt = 0;
            current_process.program_counter++;
            break;

        case 4: // Finish
            cout << "Writer " << pid << " finished." << endl;
            current_process.status = FINISHED;
            break;
    }
}

void run_reader(int pid) {
    Process &current_process = processes[pid];
    // Helper var for CAS operations
    int old_val;

    switch (current_process.program_counter) {
        case 0: // Check Reader Limit (Max 2)
            // Read the current value
            old_val = reader_limiter;
            if (old_val > 0) {
                // Try to decrement it (e.g., from 2 to 1, or 1 to 0)
                if (CompareAndSwap(reader_limiter, old_val, old_val - 1) == old_val) {
                    current_process.program_counter++;
                }
            }
            // If 0, or if CAS failed, we spin (stay at case 0)
            break;

        case 1: // Lock read_count
            if (CompareAndSwap(read_count_lock, 0, 1) == 0) {
                current_process.program_counter++;
            }
            break;

        case 2: // Increment read_count
            read_count++;
            current_process.program_counter++;
            break;

        case 3: // First reader locks writer
            if (read_count == 1) {
                // Try to acquire writer lock
                if (CompareAndSwap(wrt, 0, 1) == 0) {
                    current_process.program_counter++;
                }
                // If fail, we stay at case 3 (Busy Wait while holding read_count_lock!)
            } else {
                current_process.program_counter++;
            }
            break;

        case 4: // Release read_count lock
            read_count_lock = 0;
            current_process.program_counter++;
            break;

        case 5: // ENTER Critical Section
            active_readers++;
            cout << "Reader " << pid << " enters. "
                 << "Other Readers: " << (active_readers - 1)
                 << ", Other Writers: " << active_writers << endl;
            current_process.program_counter++;
            break;

        case 6: // WORK
            cout << "Reader " << pid << " is READING." << endl;
            check_panic();
            current_process.program_counter++;
            break;

        case 7: // EXIT Critical Section
            active_readers--;
            current_process.program_counter++;
            break;

        case 8: // Lock read_count for exit
            if (CompareAndSwap(read_count_lock, 0, 1) == 0) {
                current_process.program_counter++;
            }
            break;

        case 9: // Decrement read_count
            read_count--;
            current_process.program_counter++;
            break;

        case 10: // Last reader releases writer
            if (read_count == 0) {
                wrt = 0;
            }
            current_process.program_counter++;
            break;

        case 11: // Release read_count lock
            read_count_lock = 0;
            current_process.program_counter++;
            break;

        case 12: // Release slot for other readers
            // Atomic Increment: Read old, try to swap with old+1
            // (Looping to ensure it happens, though unlikely to fail here)
            do {
                old_val = reader_limiter;
            } while (CompareAndSwap(reader_limiter, old_val, old_val + 1) != old_val);

            current_process.program_counter++;
            break;

        case 13: // Finish
            cout << "Reader " << pid << " finished." << endl;
            current_process.status = FINISHED;
            break;
    }
}

// ============================================================================
// --- 4. MAIN SCHEDULER ---
// ============================================================================

int main() {
    srand(time(0));

    for(int i=0; i<6; i++) {
        processes[i].id = i;
        processes[i].program_counter = 0;
        processes[i].status = READY;
        processes[i].type = (i < 3) ? 0 : 1;
    }

    int completed = 0;
    while (completed < 6) {
        int pid = rand() % 6;

        // In CAS, we don't have BLOCKED. Everyone is READY or FINISHED.
        if (processes[pid].status == READY) {
            if (processes[pid].type == 0) {
                run_reader(pid);
            } else {
                run_writer(pid);
            }

            if (processes[pid].status == FINISHED) {
                completed++;
                // We leave status as FINISHED so scheduler ignores it
                // (Using a simple hack to ensure we don't run finished processes)
                processes[pid].status = (Status)99;
            }
        }
    }

    cout << "DONE !!!" << endl;
    return 0;
}