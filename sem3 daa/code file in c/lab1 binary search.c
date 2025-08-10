#define _POSIX_C_SOURCE 199309L
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#include <time.h>

#if defined(_WIN32)
#include <windows.h>
#endif

// Iterative binary search: returns index or -1 if not found
int binary_search(int *arr, int n, int target) {
    int left = 0, right = n - 1;
    while (left <= right) {
        int mid = left + (right - left) / 2;
        if (arr[mid] == target) return mid;
        else if (arr[mid] < target) left = mid + 1;
        else right = mid - 1;
    }
    return -1;
}

// Cross-platform nanosecond timer using clock() as fallback
uint64_t get_time_ns() {
#if defined(_WIN32)
    // Windows: use QueryPerformanceCounter for higher precision if needed
    LARGE_INTEGER freq, counter;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&counter);
    return (uint64_t)((counter.QuadPart * 1000000000ULL) / freq.QuadPart);
#else
    // Use clock() as fallback (microsecond precision)
    return (uint64_t)clock() * (1000000000ULL / CLOCKS_PER_SEC);
#endif
}

int main() {
    // Define array sizes to test (various scales)
    const int sizes[5] = {1000, 5000, 10000, 50000, 100000};
    // We'll create 5 best, 5 worst, 5 average cases = 15 total
    // For reproducibility, use a fixed seed
    srand(123456789);

    // Print CSV header
    printf("case_type,case_id,n,reps,time_ns\n");

    int case_id = 1;

    for (int category = 0; category < 3; ++category) {
        // category: 0=best,1=worst,2=average
        for (int si = 0; si < 5; ++si) {
            int n = sizes[si];
            // generate sorted array with negatives and duplicates
            int *arr = (int*)malloc(sizeof(int)*n);
            if (!arr) {
                fprintf(stderr, "malloc failed for n=%d\n", n);
                return 2;
            }
            // Fill with a sequence that includes negatives and duplicates:
            // arr[i] = (i/3) - (n/2) so duplicates every 3 elements and negatives included
            for (int i = 0; i < n; ++i) {
                arr[i] = (i/3) - (n/2);
            }

            int target;
            if (category == 0) {
                // best-case: target is at middle element
                int mid = n/2;
                target = arr[mid];
            } else if (category == 1) {
                // worst-case: target absent (value less than min - 1)
                target = arr[0] - 1;
            } else {
                // average-case: pick a random position (not middle, not guaranteed absent)
                int idx = (rand() % n);
                target = arr[idx];
            }

            // Determine number of repetitions to get measurable time.
            // Aim for roughly 10 million comparisons per test overall: reps * log2(n) ~ 1e7
            double logn = (n>1) ? log((double)n)/log(2.0) : 1.0;
            long reps = (long)(10000000.0 / (logn + 1.0));
            if (reps < 1) reps = 1;
            if (reps > 20000000) reps = 20000000; // cap to avoid runaway runs

            uint64_t t0 = get_time_ns();
            // run repeated searches
            volatile int sink = 0; // prevent optimization
            for (long r = 0; r < reps; ++r) {
                int found = binary_search(arr, n, target);
                sink ^= found;
            }
            uint64_t t1 = get_time_ns();
            uint64_t dt = t1 - t0;

            // Output CSV line
            const char *catname = (category==0) ? "best" : (category==1) ? "worst" : "average";
            printf("%s,%d,%d,%ld,%llu\n", catname, case_id, n, reps, (unsigned long long)dt);

            free(arr);
            case_id++;
        }
    }

    return 0;
}
