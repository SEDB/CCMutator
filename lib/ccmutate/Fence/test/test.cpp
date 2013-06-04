#include <atomic>

int a;

int main() {
    a = 0;
    std::atomic_thread_fence(std::memory_order_acquire);
    a++;

    return 0;
}
