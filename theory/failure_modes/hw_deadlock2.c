// FIX: swapped set_lock order for proc_b func and unset_lock order for
//      both, but unsure if that last fix was necessary.

#include <omp.h>
#include <stdio.h>
#include <stdlib.h>

omp_lock_t lockA;
omp_lock_t lockB;
int A=1;
int B=4;

void proc_a() {
    int x,y;
    // lock a so that the value won't change unexpectedly
    omp_set_lock(&lockA);
    x = A;
    x = x*2;
    // lock b during modification
    omp_set_lock(&lockB);
    y = B;
    y -= x;
    B = y;
    // update A and release locks
    A = x;
    omp_unset_lock(&lockB);
    omp_unset_lock(&lockA);
}

void proc_b() {
    int x,y;
    // lock b so that the value won't change unexpectedly
    omp_set_lock(&lockA);
    x = B;
    x = x/2;
    // lock a during modification
    omp_set_lock(&lockB);
    y = A;
    y += x;
    A = y;
    // update B and release locks
    B = x;
    omp_unset_lock(&lockB);
    omp_unset_lock(&lockA);
}

int main(int argc, char** argv) {
    omp_init_lock(&lockA);
    omp_init_lock(&lockB);
    #pragma omp parallel num_threads(8)
    {
        while(1) {
            int x=rand();
            printf("."); fflush(stdout);
            if(x%2==0) {
                proc_a();
            } else {
                proc_b();
            }
        }
    }
    return 0;
}
