#include <iostream>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <string>

using namespace std;

// GLOBAL Data Types ---

// BLOCKED status is removed because CAS uses Busy Waiting
enum Status { READY, FINISHED };

struct Process {
    int id;
    int program_counter;
    int type;           // 0 = Reader, 1 = Writer
    Status status;
};

// Shared Data to track readers, writers in CS
int active_readers = 0;
int active_writers = 0;
int read_count = 0;

Process processes[6];

// *** LOCKS Integers replacement instead of Semaphores) ***
// 0 = Unlocked, 1 = Locked
int read_count_lock = 0;  // Protects read_count
int wrt = 0;              // Protects Critical Section
int reader_limiter = 2;   // Counts available slots (Starts at 2)


//  COMPARE AND SWAP  ---


// Returns the OLD value.
// If return value == expected_val, the swap happened (Success).
// If return value != expected_val, the swap failed (Failure).
int CompareAndSwap(int *value, int expected, int new_val) {
    int temp = *value;
    if (*value == expected) {
        *value = new_val;
    }
    return temp;
}

void check_panic() {
    if (active_writers > 1 || (active_writers > 0 && active_readers > 0) || active_readers > 2) {
        cout << "\n***************************************************" << endl;
        cout << "PANIC: Synchronization Rules Violated!" << endl;
        cout << "Active Writers: " << active_writers << endl;
        cout << "Active Readers: " << active_readers << endl;
        cout << "***************************************************\n" << endl;
    }
}

////// --- WORKER FUNCTIONS START--- /////

// Function for WRITERS (CAS Implementation)
void run_writer(int pid) {
    Process &current_process = processes[pid];
    switch (current_process.program_counter) {
        case 0: // Request Entry
            // Try to swap 0 (unlocked) to 1 (locked)
            // If it returns 0, it means it was 0 before, so we successfully swapped to 1.
            if (CompareAndSwap(&wrt, 0, 1) == 0) {
                current_process.program_counter++;
            }
            // If CAS failed, we stay at Case 0 and retry next turn (Busy Wait).
            break;

        case 1: // Instruction: ENTER Critical Section
            active_writers++;
            cout << "Writer " << pid << " enters. "
                 << "Other Readers: " << active_readers
                 << ", Other Writers: " << (active_writers - 1) << endl;
            // Advance but don't exit to allow collisions
            current_process.program_counter++;
            break;

        case 2: // Instruction: WORK
            cout << "Writer " << pid << " is WRITING." << endl;
            check_panic();
            current_process.program_counter++;
            break;

        case 3: // Exit Critical Section
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

// Function for READERS (CAS Implementation)
void run_reader(int pid) {
    Process &current_process = processes[pid];
    int old_val; // Helper for CAS

    switch (current_process.program_counter) {

        case 0: // Check Reader Limit (Max 2)
            old_val = reader_limiter;
            if (old_val > 0) {
                // Try to decrement (e.g., 2->1 or 1->0)
                if (CompareAndSwap(&reader_limiter, old_val, old_val - 1) == old_val) {
                    current_process.program_counter++;
                }
            }
            // If 0, or if CAS failed (changed by someone else), we spin.
            break;

        case 1: // Lock read_count
            if (CompareAndSwap(&read_count_lock, 0, 1) == 0) {
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
                if (CompareAndSwap(&wrt, 0, 1) == 0) {
                    current_process.program_counter++;
                }
                // If fail, we spin at Case 3 (Holding the read_count_lock!)
            } else {
                current_process.program_counter++;
            }
            break;

        case 4: // Release read_count lock
            read_count_lock = 0;
            current_process.program_counter++;
            break;

        case 5: // Instruction: ENTER Critical Section
            active_readers++;
            cout << "Reader " << pid << " enters. "
                 << "Other Readers: " << (active_readers - 1)
                 << ", Other Writers: " << active_writers << endl;
            // Stay in CS to allow collisions
            current_process.program_counter++;
            break;

        case 6: // Instruction: WORK
             cout << "Reader " << pid << " is READING (Busy work)..." << endl;
             check_panic();
             current_process.program_counter++;
             break;

        case 7: // Exit CS
            active_readers--;
            current_process.program_counter++;
            break;

        case 8: // Lock read_count for exit
            if (CompareAndSwap(&read_count_lock, 0, 1) == 0) {
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
             //  Increment loop to ensure success
             do {
                old_val = reader_limiter;
             } while (CompareAndSwap(&reader_limiter, old_val, old_val + 1) != old_val);

            current_process.program_counter++;
            break;

        case 13: // Finish
            cout << "Reader " << pid << " finished." << endl;
            current_process.status = FINISHED;
            break;
    }
}


////// --- WORKER FUNCTIONS END--- /////

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

        //  everyone is READY until FINISHED (No BLOCKED status)
        if (processes[pid].status == READY) {
            if (processes[pid].type == 0) {
                run_reader(pid);
            } else {
                run_writer(pid);
            }

            // Update completion count
            if (processes[pid].status == FINISHED) {
                completed++;
                // Using a hack to mark as "done" so scheduler skips it
                processes[pid].status = (Status)99;
            }
        }
    }

    cout << "DONE !!!" << endl;
    return 0; ///
}