/*
 * Phase 10: Performance Tuning Module
 * Optimized scheduler and memory management algorithms
 */

#include <stdint.h>
#include <stddef.h>

/* Performance tuning constants */
#define MAX_PROCESSES 64
#define TIME_QUANTUM_BASE 10
#define CACHE_LINE_SIZE 64
#define PAGE_SIZE 4096
#define MEMORY_POOL_SIZE (1024 * 1024) /* 1MB memory pool */

/* Process priority levels */
typedef enum {
    PRIORITY_IDLE = 0,
    PRIORITY_LOW = 1,
    PRIORITY_NORMAL = 2,
    PRIORITY_HIGH = 3,
    PRIORITY_REALTIME = 4,
    PRIORITY_COUNT
} process_priority_t;

/* Process states */
typedef enum {
    STATE_CREATED,
    STATE_READY,
    STATE_RUNNING,
    STATE_BLOCKED,
    STATE_TERMINATED
} process_state_t;

/* Optimized process control block */
typedef struct __attribute__((aligned(CACHE_LINE_SIZE))) {
    uint32_t pid;
    process_state_t state;
    process_priority_t priority;
    uint32_t time_quantum;
    uint32_t cpu_time_used;
    uint32_t last_scheduled;
    
    /* Register context */
    uint32_t eax, ebx, ecx, edx, esi, edi, ebp;
    uint32_t esp, eip;
    uint32_t eflags;
    
    /* Memory management */
    uint32_t page_directory;
    uint32_t stack_start;
    uint32_t stack_size;
    
    /* Performance metrics */
    uint32_t context_switches;
    uint32_t syscalls_count;
    uint32_t page_faults;
    uint32_t cpu_usage;
    
    /* Cache optimization */
    uint32_t last_cpu;
    uint32_t cache_hotness;
    
    /* Scheduling info */
    uint32_t timeslice_remaining;
    uint32_t total_runtime;
    uint32_t wait_time;
    uint32_t last_ready_time;
    
    /* List management */
    struct process* next;
    struct process* prev;
} process_t;

/* Memory block header for optimized allocator */
typedef struct __attribute__((aligned(CACHE_LINE_SIZE))) {
    uint32_t size;
    uint32_t flags;
    struct memory_block* next;
    struct memory_block* prev;
    uint32_t padding[4]; /* Cache line alignment */
} memory_block_t;

/* Memory pool management */
typedef struct {
    uint8_t pool[MEMORY_POOL_SIZE];
    memory_block_t* free_list[PRIORITY_COUNT]; /* Per-priority free lists */
    uint32_t total_allocated;
    uint32_t total_freed;
    uint32_t fragmentation_count;
    uint32_t allocation_failures;
    uint32_t cache_hits;
    uint32_t cache_misses;
} memory_pool_t;

/* Scheduler statistics */
typedef struct {
    uint32_t total_context_switches;
    uint32_t schedule_calls;
    uint32_t idle_time;
    uint32_t starvation_preventions;
    uint32_t load_balance_ops;
    uint64_t total_schedule_time;
    uint32_t average_schedule_latency;
} scheduler_stats_t;

/* Performance counters */
typedef struct {
    uint64_t tsc_start;
    uint64_t tsc_end;
    uint32_t cache_flushes;
    uint32_t tlb_flushes;
    uint32_t page_walks;
    uint32_t interrupt_latency;
    uint32_t syscall_latency;
} performance_counters_t;

/* Global variables */
static process_t processes[MAX_PROCESSES];
static memory_pool_t memory_pool;
static scheduler_stats_t scheduler_stats;
static performance_counters_t perf_counters;
static process_t* ready_queues[PRIORITY_COUNT];
static process_t* current_process = NULL;
static uint32_t next_pid = 1;
static uint32_t scheduler_running = 0;

/* CPUID and RDTSC support */
static inline uint64_t rdtsc(void) {
    uint32_t low, high;
    __asm__ __volatile__ ("rdtsc" : "=a"(low), "=d"(high));
    return ((uint64_t)high << 32) | low;
}

static inline void cpuid(int info, int* eax, int* ebx, int* ecx, int* edx) {
    __asm__ __volatile__ ("cpuid" : "=a"(*eax), "=b"(*ebx), "=c"(*ecx), "=d"(*edx) : "a"(info));
}

/* Cache optimization functions */
static inline void flush_cache_line(void* addr) {
    __asm__ __volatile__ ("clflush %0" : : "m"(*(char*)addr));
}

static inline void prefetch(void* addr) {
    __asm__ __volatile__ ("prefetcht0 %0" : : "m"(*(char*)addr));
}

/* Memory management functions */
static void memory_pool_init(void) {
    memset(&memory_pool, 0, sizeof(memory_pool_t));
    
    /* Initialize the memory pool as one large free block */
    memory_block_t* initial_block = (memory_block_t*)memory_pool.pool;
    initial_block->size = MEMORY_POOL_SIZE - sizeof(memory_block_t);
    initial_block->flags = 0;
    initial_block->next = NULL;
    initial_block->prev = NULL;
    
    /* Add to normal priority free list */
    memory_pool.free_list[PRIORITY_NORMAL] = initial_block;
    
    /* Initialize statistics */
    memory_pool.total_allocated = 0;
    memory_pool.total_freed = 0;
    memory_pool.fragmentation_count = 0;
    memory_pool.allocation_failures = 0;
    memory_pool.cache_hits = 0;
    memory_pool.cache_misses = 0;
}

static memory_block_t* find_best_fit(uint32_t size, process_priority_t priority) {
    memory_block_t* best_block = NULL;
    uint32_t best_size = 0xFFFFFFFF;
    
    /* Search in the appropriate priority list */
    memory_block_t* current = memory_pool.free_list[priority];
    while (current) {
        if (current->size >= size && current->size < best_size) {
            best_block = current;
            best_size = current->size;
            
            /* Early exit if we find an exact fit */
            if (current->size == size) {
                memory_pool.cache_hits++;
                return current;
            }
        }
        current = current->next;
    }
    
    if (best_block) {
        memory_pool.cache_hits++;
    } else {
        memory_pool.cache_misses++;
    }
    
    return best_block;
}

static void split_block(memory_block_t* block, uint32_t size) {
    if (block->size > size + sizeof(memory_block_t) + 32) {
        memory_block_t* new_block = (memory_block_t*)((uint8_t*)block + sizeof(memory_block_t) + size);
        new_block->size = block->size - size - sizeof(memory_block_t);
        new_block->flags = 0;
        new_block->next = block->next;
        new_block->prev = block;
        
        if (block->next) {
            block->next->prev = new_block;
        }
        
        block->next = new_block;
        block->size = size;
    }
}

static void coalesce_blocks(void) {
    /* Coalesce adjacent free blocks to reduce fragmentation */
    for (int priority = 0; priority < PRIORITY_COUNT; priority++) {
        memory_block_t* current = memory_pool.free_list[priority];
        while (current && current->next) {
            if ((uint8_t*)current + sizeof(memory_block_t) + current->size == (uint8_t*)current->next) {
                /* Adjacent blocks found, merge them */
                memory_block_t* next = current->next;
                current->size += sizeof(memory_block_t) + next->size;
                current->next = next->next;
                if (next->next) {
                    next->next->prev = current;
                }
                memory_pool.fragmentation_count--;
            } else {
                current = current->next;
            }
        }
    }
}

void* optimized_malloc(uint32_t size, process_priority_t priority) {
    /* Align size to cache line boundary */
    size = (size + CACHE_LINE_SIZE - 1) & ~(CACHE_LINE_SIZE - 1);
    
    /* Find best fit block */
    memory_block_t* block = find_best_fit(size, priority);
    if (!block) {
        memory_pool.allocation_failures++;
        return NULL;
    }
    
    /* Remove block from free list */
    if (block->prev) {
        block->prev->next = block->next;
    } else {
        memory_pool.free_list[priority] = block->next;
    }
    
    if (block->next) {
        block->next->prev = block->prev;
    }
    
    /* Split block if it's too large */
    split_block(block, size);
    
    /* Mark as allocated */
    block->flags = 1;
    
    /* Update statistics */
    memory_pool.total_allocated += size;
    
    /* Return pointer to data area */
    return (uint8_t*)block + sizeof(memory_block_t);
}

void optimized_free(void* ptr, process_priority_t priority) {
    if (!ptr) return;
    
    memory_block_t* block = (memory_block_t*)((uint8_t*)ptr - sizeof(memory_block_t));
    
    /* Mark as free */
    block->flags = 0;
    
    /* Add to free list */
    block->next = memory_pool.free_list[priority];
    block->prev = NULL;
    
    if (memory_pool.free_list[priority]) {
        memory_pool.free_list[priority]->prev = block;
    }
    
    memory_pool.free_list[priority] = block;
    
    /* Update statistics */
    memory_pool.total_freed += block->size;
    
    /* Periodically coalesce blocks to reduce fragmentation */
    if (memory_pool.fragmentation_count++ > 100) {
        coalesce_blocks();
        memory_pool.fragmentation_count = 0;
    }
}

/* Process management functions */
static process_t* create_process(const char* name, process_priority_t priority) {
    if (next_pid >= MAX_PROCESSES) {
        return NULL;
    }
    
    process_t* proc = &processes[next_pid];
    memset(proc, 0, sizeof(process_t));
    
    proc->pid = next_pid++;
    proc->state = STATE_CREATED;
    proc->priority = priority;
    proc->time_quantum = TIME_QUANTUM_BASE * (priority + 1);
    proc->last_scheduled = 0;
    proc->last_cpu = 0;
    proc->cache_hotness = 0;
    
    /* Allocate stack */
    proc->stack_size = PAGE_SIZE;
    proc->stack_start = (uint32_t)optimized_malloc(proc->stack_size, priority);
    if (!proc->stack_start) {
        return NULL;
    }
    
    proc->esp = proc->stack_start + proc->stack_size;
    
    return proc;
}

static void destroy_process(process_t* proc) {
    if (proc->state != STATE_TERMINATED) {
        proc->state = STATE_TERMINATED;
    }
    
    /* Free stack memory */
    if (proc->stack_start) {
        optimized_free((void*)proc->stack_start, proc->priority);
        proc->stack_start = 0;
    }
}

/* Optimized scheduler implementation */
static void update_process_stats(process_t* proc) {
    uint64_t current_time = rdtsc();
    uint64_t runtime = current_time - proc->last_scheduled;
    
    proc->cpu_time_used += runtime;
    proc->total_runtime += runtime;
    proc->timeslice_remaining--;
    
    /* Update cache hotness */
    if (proc->last_cpu == 0) { /* Assuming CPU 0 for now */
        proc->cache_hotness++;
    } else {
        proc->cache_hotness = 0;
    }
    
    proc->last_scheduled = current_time;
}

static process_t* select_next_process(void) {
    process_t* selected = NULL;
    uint32_t highest_priority = 0;
    uint32_t min_wait_time = 0xFFFFFFFF;
    
    /* Multi-level feedback queue scheduling */
    for (int priority = PRIORITY_REALTIME; priority >= PRIORITY_IDLE; priority--) {
        process_t* current = ready_queues[priority];
        
        while (current) {
            /* Skip terminated processes */
            if (current->state == STATE_TERMINATED) {
                process_t* next = current->next;
                /* Remove from ready queue */
                if (current->prev) {
                    current->prev->next = current->next;
                } else {
                    ready_queues[priority] = current->next;
                }
                if (current->next) {
                    current->next->prev = current->prev;
                }
                destroy_process(current);
                current = next;
                continue;
            }
            
            /* Check if process is ready and has time remaining */
            if (current->state == STATE_READY && current->timeslice_remaining > 0) {
                /* Priority-based selection */
                if (priority > highest_priority) {
                    highest_priority = priority;
                    selected = current;
                    min_wait_time = current->wait_time;
                } else if (priority == highest_priority) {
                    /* Within same priority, use round-robin with aging */
                    if (current->wait_time > min_wait_time + 1000) {
                        selected = current;
                        min_wait_time = current->wait_time;
                        scheduler_stats.starvation_preventions++;
                    } else if (current->wait_time == min_wait_time && !selected) {
                        selected = current;
                    }
                }
            }
            
            current = current->next;
        }
        
        /* If we found a process at this priority level, break */
        if (selected) {
            break;
        }
    }
    
    /* If no process found, check for processes that need time quantum refresh */
    if (!selected) {
        for (int priority = PRIORITY_REALTIME; priority >= PRIORITY_IDLE; priority--) {
            process_t* current = ready_queues[priority];
            while (current) {
                if (current->state == STATE_READY && current->timeslice_remaining == 0) {
                    /* Refresh time quantum */
                    current->timeslice_remaining = TIME_QUANTUM_BASE * (priority + 1);
                    selected = current;
                    break;
                }
                current = current->next;
            }
            if (selected) break;
        }
    }
    
    return selected;
}

static void add_to_ready_queue(process_t* proc) {
    proc->state = STATE_READY;
    proc->last_ready_time = rdtsc();
    
    /* Add to appropriate priority queue */
    proc->next = ready_queues[proc->priority];
    proc->prev = NULL;
    
    if (ready_queues[proc->priority]) {
        ready_queues[proc->priority]->prev = proc;
    }
    
    ready_queues[proc->priority] = proc;
}

static void update_wait_times(void) {
    uint64_t current_time = rdtsc();
    
    for (int priority = 0; priority < PRIORITY_COUNT; priority++) {
        process_t* current = ready_queues[priority];
        while (current) {
            if (current->state == STATE_READY) {
                current->wait_time = current_time - current->last_ready_time;
            }
            current = current->next;
        }
    }
}

static void context_switch(process_t* next) {
    if (!next) return;
    
    uint64_t switch_start = rdtsc();
    
    /* Save current process context */
    if (current_process && current_process != next) {
        __asm__ __volatile__ (
            "mov %%eax, %0\n"
            "mov %%ebx, %1\n"
            "mov %%ecx, %2\n"
            "mov %%edx, %3\n"
            "mov %%esi, %4\n"
            "mov %%edi, %5\n"
            "mov %%ebp, %6\n"
            "mov %%esp, %7\n"
            "mov %%eip, %8\n"
            "pushfl\n"
            "pop %9\n"
            : "=m"(current_process->eax), "=m"(current_process->ebx),
              "=m"(current_process->ecx), "=m"(current_process->edx),
              "=m"(current_process->esi), "=m"(current_process->edi),
              "=m"(current_process->ebp), "=m"(current_process->esp),
              "=m"(current_process->eip), "=m"(current_process->eflags)
        );
        
        current_process->state = STATE_READY;
        current_process->context_switches++;
        
        /* Add back to ready queue */
        add_to_ready_queue(current_process);
    }
    
    /* Update scheduler statistics */
    scheduler_stats.total_context_switches++;
    uint64_t switch_end = rdtsc();
    scheduler_stats.total_schedule_time += (switch_end - switch_start);
    
    /* Switch to next process */
    current_process = next;
    current_process->state = STATE_RUNNING;
    current_process->last_scheduled = rdtsc();
    current_process->last_cpu = 0; /* Assuming CPU 0 */
    
    /* Restore next process context */
    __asm__ __volatile__ (
        "mov %0, %%eax\n"
        "mov %1, %%ebx\n"
        "mov %2, %%ecx\n"
        "mov %3, %%edx\n"
        "mov %4, %%esi\n"
        "mov %5, %%edi\n"
        "mov %6, %%ebp\n"
        "mov %7, %%esp\n"
        "mov %8, %%eip\n"
        "push %9\n"
        "popfl\n"
        :
        : "m"(next->eax), "m"(next->ebx), "m"(next->ecx), "m"(next->edx),
          "m"(next->esi), "m"(next->edi), "m"(next->ebp), "m"(next->esp),
          "m"(next->eip), "m"(next->eflags)
    );
}

void optimized_scheduler(void) {
    if (!scheduler_running) return;
    
    uint64_t schedule_start = rdtsc();
    scheduler_stats.schedule_calls++;
    
    /* Update wait times for all ready processes */
    update_wait_times();
    
    /* Update current process stats */
    if (current_process) {
        update_process_stats(current_process);
        
        /* Check if current process has used up its time quantum */
        if (current_process->timeslice_remaining <= 0) {
            current_process->state = STATE_READY;
        }
    }
    
    /* Select next process to run */
    process_t* next = select_next_process();
    
    /* If no process is ready, keep current process or idle */
    if (!next) {
        if (current_process && current_process->state == STATE_RUNNING) {
            /* Current process continues running */
            return;
        } else {
            /* System idle */
            scheduler_stats.idle_time++;
            return;
        }
    }
    
    /* Perform context switch if different process */
    if (next != current_process) {
        context_switch(next);
    }
    
    uint64_t schedule_end = rdtsc();
    uint64_t schedule_latency = schedule_end - schedule_start;
    
    /* Update average schedule latency */
    scheduler_stats.average_schedule_latency = 
        (scheduler_stats.average_schedule_latency * 99 + schedule_latency) / 100;
}

/* Performance monitoring functions */
void get_scheduler_stats(scheduler_stats_t* stats) {
    if (stats) {
        memcpy(stats, &scheduler_stats, sizeof(scheduler_stats_t));
    }
}

void get_memory_stats(uint32_t* total_allocated, uint32_t* total_freed, 
                     uint32_t* fragmentation, uint32_t* cache_hit_ratio) {
    if (total_allocated) *total_allocated = memory_pool.total_allocated;
    if (total_freed) *total_freed = memory_pool.total_freed;
    if (fragmentation) *fragmentation = memory_pool.fragmentation_count;
    if (cache_hit_ratio) {
        uint32_t total_accesses = memory_pool.cache_hits + memory_pool.cache_misses;
        *cache_hit_ratio = total_accesses > 0 ? 
            (memory_pool.cache_hits * 100) / total_accesses : 0;
    }
}

/* Optimization recommendations */
void analyze_performance(void) {
    scheduler_stats_t stats;
    uint32_t total_allocated, total_freed, fragmentation, cache_hit_ratio;
    
    get_scheduler_stats(&stats);
    get_memory_stats(&total_allocated, &total_freed, &fragmentation, &cache_hit_ratio);
    
    /* Analyze scheduler performance */
    if (stats.total_context_switches > 10000) {
        /* High context switch rate - may need optimization */
    }
    
    if (stats.average_schedule_latency > 1000) {
        /* High scheduling latency - may need algorithm optimization */
    }
    
    /* Analyze memory performance */
    if (fragmentation > 50) {
        /* High fragmentation - may need better coalescing strategy */
    }
    
    if (cache_hit_ratio < 80) {
        /* Low cache hit ratio - may need better allocation strategy */
    }
}

/* Initialize performance tuning system */
void performance_tuning_init(void) {
    /* Initialize memory pool */
    memory_pool_init();
    
    /* Initialize scheduler statistics */
    memset(&scheduler_stats, 0, sizeof(scheduler_stats_t));
    
    /* Initialize performance counters */
    memset(&perf_counters, 0, sizeof(performance_counters_t));
    perf_counters.tsc_start = rdtsc();
    
    /* Initialize ready queues */
    for (int i = 0; i < PRIORITY_COUNT; i++) {
        ready_queues[i] = NULL;
    }
    
    /* Create init process */
    process_t* init_proc = create_process("init", PRIORITY_HIGH);
    if (init_proc) {
        add_to_ready_queue(init_proc);
        current_process = init_proc;
    }
    
    scheduler_running = 1;
}