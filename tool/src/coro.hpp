// Stackful coroutines on one thread, for interleaving the pieces of a chopped plain search on the move structure
// (latency hiding on the move structure, part 2).  A switch saves the callee-saved registers and the stack pointer
// and loads another coroutine's (katka_ctx_swap): no system call, unlike swapcontext.  x86-64 System V only.
// co_sched::run(K, f) runs f(id) in K coroutines, round-robin: yield() passes to the next live coroutine, and a
// coroutine whose f returns leaves the ring; run returns when all have.  g_co is the scheduler of this thread while
// run is active, else null.  The two assembler functions are weak, so the header can be in several translation units.
#pragma once
#include <cstdint>
#include <vector>
#include <functional>
extern "C" void katka_ctx_swap(void **save_sp, void *to_sp);
extern "C" void katka_co_entry();
asm(R"(
    .text
    .weak katka_ctx_swap
    .type katka_ctx_swap, @function
katka_ctx_swap:
    pushq %rbp
    pushq %rbx
    pushq %r12
    pushq %r13
    pushq %r14
    pushq %r15
    movq %rsp, (%rdi)
    movq %rsi, %rsp
    popq %r15
    popq %r14
    popq %r13
    popq %r12
    popq %rbx
    popq %rbp
    ret
    .size katka_ctx_swap, .-katka_ctx_swap
    .weak katka_co_entry
    .type katka_co_entry, @function
katka_co_entry:
    movq %r12, %rdi
    call katka_co_main
    ud2
    .size katka_co_entry, .-katka_co_entry
)");
namespace katka {
struct co_sched {
    struct co { void *sp = nullptr; std::vector<char> stack; int id = 0; co_sched *s = nullptr; };
    std::vector<co> cos;
    std::vector<int> ring;          // the live coroutines, in turn order
    size_t pos = 0;                 // ring[pos] is running
    void *main_sp = nullptr;
    std::function<void(int)> body;
    static constexpr size_t STACK = 1 << 20;
    inline void yield() {
        const int me = ring[pos];
        if (++pos == ring.size()) pos = 0;
        if (ring[pos] != me) katka_ctx_swap(&cos[me].sp, cos[ring[pos]].sp);
    }
    [[noreturn]] void finish() {    // the running coroutine's body has returned
        const int me = ring[pos];
        ring.erase(ring.begin() + pos);
        if (pos == ring.size()) pos = 0;
        katka_ctx_swap(&cos[me].sp, ring.empty() ? main_sp : cos[ring[pos]].sp);
        __builtin_unreachable();
    }
    void run(int K, std::function<void(int)> f);
};
inline thread_local co_sched *g_co = nullptr;
}  // namespace katka
extern "C" [[noreturn]] __attribute__((used)) inline void katka_co_main(katka::co_sched::co *c) {
    c->s->body(c->id);
    c->s->finish();
}
namespace katka {
inline void co_sched::run(int K, std::function<void(int)> f) {
    body = std::move(f);
    cos.assign(K, co());
    ring.clear();
    for (int i = 0; i < K; ++i) {
        co &c = cos[i];
        c.id = i; c.s = this; c.stack.assign(STACK, 0);
        // the initial frame popped by katka_ctx_swap: r15, r14, r13, r12 (= &c, for katka_co_entry), rbx, rbp, then the
        // return address katka_co_entry; the stack is 16-byte aligned at its call of katka_co_main
        uintptr_t top = ((uintptr_t)c.stack.data() + STACK) & ~(uintptr_t)15;
        void **sp = (void **)(top - 72);
        for (int r = 0; r < 6; ++r) sp[r] = nullptr;
        sp[3] = &c; sp[6] = (void *)&katka_co_entry;
        c.sp = sp;
        ring.push_back(i);
    }
    pos = 0;
    co_sched *prev = g_co; g_co = this;
    katka_ctx_swap(&main_sp, cos[0].sp);
    g_co = prev;
    cos.clear();
}
}  // namespace katka
