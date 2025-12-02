#include <iostream>
#include <omp.h> // Include the OpenMP header

int main() {

    // This pragma tells OpenMP to create a team of threads
    // and have each thread execute the code block below.
#pragma omp parallel
    {
        // Get the unique ID of the current thread
        int thread_ID = omp_get_thread_num();

        std::cout << "Hello from thread: " << thread_ID << std::endl;
    }

    return 0;
}