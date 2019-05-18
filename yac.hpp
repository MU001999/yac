#ifndef YETANOTHERCOROUTINE_HPP
#define YETANOTHERCOROUTINE_HPP

#include <list>
#include <vector>
#include <cassert>
#include <cstdint>
#include <functional>

#include <ucontext.h>

namespace yac
{
constexpr size_t STACK_LIMIT = 1024 * 1024;

using routine_t = uint64_t;

struct Routine
{
    std::function<void()> func;
    char *stack;
    bool finished;
    ucontext_t ctx;

    Routine(std::function<void()> func)
      : func(std::move(func)),
        stack(nullptr),
        finished(false)
    {
        // ...
    }

    ~Routine()
    {
        delete[] stack;
    }
};

struct Ordinator
{
    std::vector<Routine *> routines;
    std::list<routine_t> deleted;
    routine_t current;
    size_t stack_size;
    ucontext_t ctx;

    Ordinator(size_t ss = STACK_LIMIT)
      : current(0),
        stack_size(ss)
    {
        // ...
    }

    ~Ordinator()
    {
        for (auto &routine : routines)
            delete routine;
    }
};

thread_local inline Ordinator ordinator;

inline routine_t
create(std::function<void()> func)
{
    auto routine = new Routine(func);

    if (ordinator.deleted.empty())
    {
        ordinator.routines.push_back(routine);
        return ordinator.routines.size();
    }
    else
    {
        auto id = ordinator.deleted.front();
        ordinator.deleted.pop_front();
        assert(ordinator.routines[id-1] == nullptr);
        ordinator.routines[id-1] = routine;
        return id;
    }
}

inline void
destory(routine_t id)
{
    auto routine = ordinator.routines[id-1];
    assert(routine != nullptr);
    
    delete routine;
    ordinator.routines[id-1] = nullptr;
    ordinator.deleted.push_back(id);
}

inline void
entry()
{
    auto id = ordinator.current;
    auto routine = ordinator.routines[id-1];
    routine->func();

    routine->finished = true;
    ordinator.current = 0;
    ordinator.deleted.push_back(id);
}

inline int
resume(routine_t id)
{
    assert(ordinator.current == 0);

    auto routine = ordinator.routines[id-1];
    if (routine == nullptr)
    {
        return -1;
    }
    else if (routine->finished)
    {
        return -2;
    }

    if (routine->stack == nullptr)
    {
        getcontext(&routine->ctx);

        routine->stack = new char[ordinator.stack_size];
        routine->ctx.uc_stack.ss_sp = routine->stack;
        routine->ctx.uc_stack.ss_size = ordinator.stack_size;
        routine->ctx.uc_link = &ordinator.ctx;
        ordinator.current = id;

        makecontext(&routine->ctx, reinterpret_cast<void(*)(void)>(entry), 0);
        swapcontext(&ordinator.ctx, &routine->ctx);
    }
    else
    {
        ordinator.current = id;
        swapcontext(&ordinator.ctx, &routine->ctx);
    }
    
    return 0;
}

inline void
yield()
{
    auto id = ordinator.current;
    auto routine = ordinator.routines[id-1];
    assert(routine != nullptr);

    char *stack_top = routine->stack + ordinator.stack_size;
    char stack_bottom = 0;
    assert(size_t(stack_top - &stack_bottom) <= ordinator.stack_size);

    ordinator.current = 0;
    swapcontext(&routine->ctx, &ordinator.ctx);
}

inline routine_t
current()
{
    return ordinator.current;
}
}

#endif // YETANOTHERCOROUTINE_HPP