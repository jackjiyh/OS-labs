#include <assert.h>
#include <stdlib.h>
#include <ucontext.h>
#include "thread.h"
#include "interrupt.h"

enum t_state {
    RUNNING = 0,
    READY,
    BLOCKED,
    EXITED
};

/* This is the thread control block */
struct thread {
	/* ... Fill this in ... */
	enum t_state state;
        Tid tid;
        ucontext_t t_context;
};

struct t_queue {
    struct thread t;
    struct t_queue * next;
};

struct t_queue * dequeue(struct t_queue ** tq, struct t_queue ** rear, Tid tid);
Tid enqueue(struct t_queue ** tq, struct t_queue ** rear, struct t_queue * newT);
void thread_stub(void(*fn)(void *), void *argv);

/* Global Variables */
Tid numT;
struct t_queue * curT;
struct t_queue * readyQ;
struct t_queue * rearRq;
struct t_queue * exitQ;
struct t_queue * rearEq;
Tid killedTid[THREAD_MAX_THREADS];
int numKT;

struct t_queue * dequeue(struct t_queue ** tq, struct t_queue ** rear, Tid tid) {
    //struct t_queue invalid;
    //invalid.t.tid = THREAD_INVALID;
    if (*tq == NULL) {
        return NULL;
    }
    struct t_queue * ret;
    struct t_queue * curTq = *tq;
    struct t_queue * preTq = NULL;
    while (curTq != NULL) {
        if (curTq->t.tid == tid) {
            if (preTq == NULL) {
                if (curTq == *rear) (*rear) = curTq->next;
                *tq = curTq->next;
            } else {
                preTq->next = curTq->next;
                if (curTq == *rear) (*rear) = preTq;
            }
            ret = curTq;
            return ret;
        }
        preTq = curTq;
        curTq = curTq->next;
    }
    return NULL;
}

Tid enqueue(struct t_queue ** tq, struct t_queue ** rear, struct t_queue * newT) {
    //struct t_queue temp;
    //temp.t = t;
    //temp.next = NULL;
    
    struct t_queue * curTq = *tq;
    if (curTq == NULL) {
        *tq = newT;
        (*rear) = newT;
        return newT->t.tid;
    }
    /*while (curTq != NULL) {
        if (curTq->next == NULL) {
            curTq->next = newT;
            return newT->t.tid;
        }
        curTq = curTq->next;
    }*/
    (*rear)->next = newT;
    (*rear) = newT;
    return newT->t.tid;
    //return THREAD_FAILED;
}

void
thread_init(void)
{
	/* your optional code here */
        numT = 1;
        curT = (struct t_queue *)malloc(sizeof(struct t_queue));
        curT->t.tid = 0;
        curT->t.state = RUNNING;
        curT->next = NULL;
        readyQ = NULL;
        rearRq = NULL;
        exitQ = NULL;
        rearEq = NULL;
        numKT = 0;
}

Tid
thread_id()
{
	//TBD();
	return curT->t.tid;
}

void thread_stub(void(*fn)(void *), void *argv) {
    Tid ret;
    fn(argv);
    ret = thread_exit();
    assert(ret == THREAD_NONE);
    printf("here\n");
    while (exitQ != NULL) {
        struct t_queue * ret = dequeue(&exitQ, &rearEq, exitQ->t.tid);
        ret->next = NULL;
        free(ret->t.t_context.uc_stack.ss_sp);
        killedTid[numKT] = ret->t.tid;
        numKT += 1;
        free(ret);
        numT -= 1;
    }

    exit(0);
}

Tid
thread_create(void (*fn) (void *), void *parg)
{
	//TBD();
	struct t_queue * newT = (struct t_queue *)malloc(sizeof(struct t_queue));
	ucontext_t newContext;
	if (numT >= THREAD_MAX_THREADS) return THREAD_NOMORE;
	getcontext(&newContext);
	newContext.uc_stack.ss_sp = malloc(THREAD_MIN_STACK);
	if (newContext.uc_stack.ss_sp == NULL) return THREAD_NOMEMORY;
	newContext.uc_stack.ss_size = THREAD_MIN_STACK;
	newContext.uc_stack.ss_flags= 0;
	unsigned long int * sp = (unsigned long int *)((unsigned long int)newContext.uc_stack.ss_sp + (unsigned long int)newContext.uc_stack.ss_size);
	sp = (unsigned long int *)(((unsigned long int)sp & -16L) - 8);
        newContext.uc_mcontext.gregs[REG_RSP] = (unsigned long int)sp;
        newContext.uc_mcontext.gregs[REG_RIP] = (unsigned long int)thread_stub;
        newContext.uc_mcontext.gregs[REG_RDI] = (unsigned long int)fn;
        newContext.uc_mcontext.gregs[REG_RSI] = (unsigned long int)parg;

        //makecontext(&newContext, (void(*)(void))fn, 1, parg);
        
        
        if (numKT == 0) newT->t.tid = numT++;
        else {
            newT->t.tid = killedTid[numKT-1];
            numKT -= 1;
            numT++;
        }
        newT->t.state = READY;
        newT->t.t_context = newContext;
        newT->next = NULL;

        enqueue(&readyQ, &rearRq, newT);

	return newT->t.tid;
}

Tid
thread_yield(Tid want_tid)
{
        int swapFlag = 0;
	//TBD();
        if (want_tid == THREAD_ANY) {
            if (readyQ == NULL) return THREAD_NONE;
            else want_tid = readyQ->t.tid;
        }

        if (want_tid == THREAD_SELF || want_tid == thread_id()) return thread_id();

        struct t_queue * ret = dequeue(&readyQ, &rearRq, want_tid);
        if (ret == NULL) return THREAD_INVALID;
        ret->next = NULL;
        
        
        ucontext_t save;
        getcontext(&save);
        if (swapFlag == 0) {
            swapFlag = 1;
            //save.uc_mcontext.gregs[REG_RIP] += 113;
            curT->t.t_context = save;
            curT->t.state = READY;
            //curT->next = NULL;
            ret->t.state = RUNNING;
            enqueue(&readyQ, &rearRq, curT);
            curT = ret;

            //swapcontext(&save, &(curT->t.t_context));
            setcontext(&(curT->t.t_context));
        }

        return want_tid;
}

Tid
thread_exit()
{
	//TBD();
	//return THREAD_FAILED;
        while (exitQ != NULL) {
            struct t_queue * ret = dequeue(&exitQ, &rearEq, exitQ->t.tid);
            ret->next = NULL;
            free(ret->t.t_context.uc_stack.ss_sp);
            killedTid[numKT] = ret->t.tid;
            numKT += 1;
            free(ret);
            numT -= 1;
	}

	if (readyQ == NULL) return THREAD_NONE;
	
	struct t_queue * ret = dequeue(&readyQ, &rearRq, readyQ->t.tid);
        Tid curId = thread_id();
        ret->next = NULL;
        //int swapFlag = 0;

	ucontext_t save;
	getcontext(&save);
	curT->t.t_context = save;
	curT->t.state = EXITED;
        ret->t.state = RUNNING;
        enqueue(&exitQ, &rearEq, curT);
        curT = ret;

        //swapcontext(&save, &(curT->t.t_context));
        setcontext(&(curT->t.t_context));
        
	return curId;
}

Tid
thread_kill(Tid tid)
{
        //return THREAD_FAILED;
	//TBD();
	while (exitQ != NULL) {
            struct t_queue * ret = dequeue(&exitQ, &rearEq, exitQ->t.tid);
            ret->next = NULL;
            free(ret->t.t_context.uc_stack.ss_sp);
            killedTid[numKT] = ret->t.tid;
            numKT += 1;
            free(ret);
            numT -= 1;
	}

        struct t_queue * ret = dequeue(&readyQ, &rearRq, tid);
        if (ret == NULL) return THREAD_INVALID;
        ret->t.state = EXITED;
        free(ret->t.t_context.uc_stack.ss_sp);
        killedTid[numKT] = ret->t.tid;
        numKT += 1;
        free(ret);
        numT -= 1;

	return tid;
}

/*******************************************************************
 * Important: The rest of the code should be implemented in Lab 3. *
 *******************************************************************/

/* This is the wait queue structure */
struct wait_queue {
	/* ... Fill this in ... */
};

struct wait_queue *
wait_queue_create()
{
	struct wait_queue *wq;

	wq = malloc(sizeof(struct wait_queue));
	assert(wq);

	TBD();

	return wq;
}

void
wait_queue_destroy(struct wait_queue *wq)
{
	TBD();
	free(wq);
}

Tid
thread_sleep(struct wait_queue *queue)
{
	TBD();
	return THREAD_FAILED;
}

/* when the 'all' parameter is 1, wakeup all threads waiting in the queue.
 * returns whether a thread was woken up on not. */
int
thread_wakeup(struct wait_queue *queue, int all)
{
	TBD();
	return 0;
}

struct lock {
	/* ... Fill this in ... */
};

struct lock *
lock_create()
{
	struct lock *lock;

	lock = malloc(sizeof(struct lock));
	assert(lock);

	TBD();

	return lock;
}

void
lock_destroy(struct lock *lock)
{
	assert(lock != NULL);

	TBD();

	free(lock);
}

void
lock_acquire(struct lock *lock)
{
	assert(lock != NULL);

	TBD();
}

void
lock_release(struct lock *lock)
{
	assert(lock != NULL);

	TBD();
}

struct cv {
	/* ... Fill this in ... */
};

struct cv *
cv_create()
{
	struct cv *cv;

	cv = malloc(sizeof(struct cv));
	assert(cv);

	TBD();

	return cv;
}

void
cv_destroy(struct cv *cv)
{
	assert(cv != NULL);

	TBD();

	free(cv);
}

void
cv_wait(struct cv *cv, struct lock *lock)
{
	assert(cv != NULL);
	assert(lock != NULL);

	TBD();
}

void
cv_signal(struct cv *cv, struct lock *lock)
{
	assert(cv != NULL);
	assert(lock != NULL);

	TBD();
}

void
cv_broadcast(struct cv *cv, struct lock *lock)
{
	assert(cv != NULL);
	assert(lock != NULL);

	TBD();
}
