#include <atomic>

std::atomic<int> counter;

int main() {
    counter.store(counter+1, std::memory_order_release);
    return 0;
}
