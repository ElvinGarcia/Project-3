

#include <iostream>  // i/o operation
#include <vector>    // dynamic arrays
#include <omp.h>    // OpenMP functions & directives
#include <chrono>   // time

using namespace  std;

//  fills vectors A and B
void initializeVectors( vector<int> & A, vector<int> & B, int n) {
    for (int i = 0; i < n; ++i) {
        A[i] = i + 1;
        B[i] = n - i;
    }
}

// Performs vector addition by dividing the work load into vectors A & B among multiple threads.
void parallelVectorAddition(const vector<int>& A, const vector<int>& B, vector<int>& C, int n, int thread_num) {
    #pragma omp parallel num_threads(thread_num) // directive to set number of threads to run
    {
        // Each thread gets its own private copies of these variables
        int thread_id = omp_get_thread_num(); // captures the thread's ID for use in calculation
        int num_threads     = omp_get_num_threads();  // the number of threads.
        int start_index = thread_id * n / num_threads;    // dividing the total number of elements among the number of threads

        int end_index;
        if (thread_id == num_threads - 1) // checks thread is last
            {
            end_index = n;  // last thread goes to the end
        } else {
            end_index = (thread_id + 1) * n / num_threads; //
        }
        // loops through each its own range and computes individual vectors
        for (int i = start_index; i < end_index; ++i) {
            C[i] = A[i] + B[i];
        }
    }
}

// checks that the output value from vector C is 10001
bool checkOutput(const vector<int>& C, int n) {
    for (int i = 0; i < n; ++i) {
        if (C[i] != (n + 1)) {
            return false;
        }
    }
    return true;
}

int main(int argc, char* argv[]) {
// Checks the correct of arguments where provided
    if (argc != 3) {
        cerr << "Usage: " << argv[0] << " <n> <threads>" << endl;
        return 1;
    }

    // converts user input form strings to integers
    int n = stoi(argv[1]);
    int num_threads = stoi(argv[2]);

    // Initialization of vectors A, B, C with size of n =  use input
    vector<int> A(n), B(n), C(n);
    initializeVectors(A, B, n);

    // Needed to measure the exact time each thread starts and stops
    // chrono uses the system timer
    auto start = chrono::high_resolution_clock::now();

    // Performs the parallel vector addition.
    // this function helps C contain the sum of the other vectors
    parallelVectorAddition(A, B, C, n, num_threads);

    // Stop the timer that was previously started
    // checks the difference between the end and start time
    // and stores it in elapsed variable
    auto end = chrono::high_resolution_clock::now();
    chrono::duration<double> elapsed = end - start;

    // Checks the computed output
    // the results are stored in variable isCorrect
    // based on the boolean returned the message "correct" or "incorrect" is returned.
    bool isCorrect = checkOutput(C, n);
    if (isCorrect == true) {
        cout << "Output is correct" << endl;
    } else {
        cout << "Output is incorrect" << endl;
    }

    // Print the running time along with the number of threads
    cout << "Threads: " << num_threads
          << "\tTime: " << elapsed.count()
          << endl;


    // mini version of original vector
    // run only when n == 10000 and mini demo for when n = 100 with 6 threads
    if (n == 10000) {
        int mini_n = 100;
        vector<int> mini_A(mini_n), mini_B(mini_n), mini_C(mini_n);
        initializeVectors(mini_A, mini_B, mini_n);
        parallelVectorAddition(mini_A, mini_B, mini_C, mini_n, 6);

        cout << "\nvmini version of original vector output (first 10 elements):" << endl;
        for (int i = 0; i < 10; ++i) {
            cout << "C[" << i << "] = " << mini_C[i] << endl;
        }
    }

    return 0;
}
