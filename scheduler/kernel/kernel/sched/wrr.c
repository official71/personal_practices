/*
 * Weighted Round-Robin Scheduling Class (mapped to the SCHED_WRR policy)
 */

#include "sched.h"
#include <linux/slab.h>

static int wrr_unit_timeslice = WRR_TIMESLICE; /* unit timeslice */
static int wrr_non_boost_weight = WRR_NON_BOOST_WEIGHT;
static int wrr_def_weight = WRR_DEF_WEIGHT;

static inline int on_wrr_rq(struct sched_wrr_entity *wrr_se)
{
	return !list_empty(&wrr_se->run_list);
}

static inline struct task_struct *wrr_task_of(struct sched_wrr_entity *wrr_se)
{
	return container_of(wrr_se, struct task_struct, wrr);
}

void init_wrr_rq(struct wrr_rq *wrr_rq)
{
	pr_info("Called %s with wrr_rq %p\n", __func__, wrr_rq);

	wrr_rq->wrr_nr_running = 0;
	INIT_LIST_HEAD(&wrr_rq->wrr_rq_list);
	raw_spin_lock_init(&wrr_rq->wrr_rq_lock);
#ifdef CONFIG_SMP
	wrr_rq->wrr_total_timeslice = 0;
#endif
}

static void __enqueue_task_wrr(struct wrr_rq *wrr_rq,
		struct sched_wrr_entity *wrr_se, int weight)
{
	struct list_head *wrr_rq_list;

	wrr_rq_list = &wrr_rq->wrr_rq_list;

	list_add_tail(&wrr_se->run_list, &wrr_rq->wrr_rq_list);
	wrr_rq->wrr_nr_running++;
	wrr_rq->wrr_total_timeslice += wrr_se->wrr_timeslice;
}

static void enqueue_task_wrr(struct rq *rq, struct task_struct *p, int flags)
{
	struct wrr_rq *wrr_rq;
	struct sched_wrr_entity *wrr_se;

	wrr_rq = &rq->wrr;
	wrr_se = &p->wrr;

	if (p->cred->uid >= 10000)
		wrr_se->wrr_weight = wrr_def_weight;
	else
		wrr_se->wrr_weight = wrr_non_boost_weight;

	/* Calculate the weight before enqueue task operationg */
	wrr_se->wrr_timeslice = wrr_se->wrr_weight * wrr_unit_timeslice;

	__enqueue_task_wrr(wrr_rq, wrr_se, wrr_se->wrr_timeslice);

	wrr_se->wrr_old_tl = wrr_se->wrr_timeslice;
	inc_nr_running(rq);

}

static void __dequeue_task_wrr(struct wrr_rq *wrr_rq,
		struct sched_wrr_entity *wrr_se, int weight)
{
	if (on_wrr_rq(wrr_se)) {
		list_del_init(&wrr_se->run_list);
		wrr_rq->wrr_nr_running--;
	}

}
static void dequeue_task_wrr(struct rq *rq, struct task_struct *p, int flags)
{
	struct wrr_rq *wrr_rq = &rq->wrr;
	struct sched_wrr_entity *wrr_se = &p->wrr;

#ifdef CONFIG_SMP
	wrr_rq->wrr_total_timeslice -= wrr_se->wrr_old_tl;
#endif
	__dequeue_task_wrr(wrr_rq, wrr_se, wrr_se->wrr_timeslice);
	dec_nr_running(rq);

}

static void requeue_task_wrr(struct rq *rq, struct task_struct *p, int flags)
{
	struct wrr_rq *wrr_rq;
	struct sched_wrr_entity *wrr_se;
	struct list_head *wrr_rq_list;

	wrr_rq = &rq->wrr;
	wrr_se = &p->wrr;
	wrr_rq_list = &wrr_rq->wrr_rq_list;

	if (wrr_rq->wrr_nr_running > 1)
		list_move_tail(&wrr_se->run_list, wrr_rq_list);
#ifdef CONFIG_SMP
	wrr_rq->wrr_total_timeslice -= (wrr_se->wrr_old_tl -
					wrr_se->wrr_timeslice);
#endif
	wrr_se->wrr_old_tl = wrr_se->wrr_timeslice;

}

static void yield_task_wrr(struct rq *rq)
{
	requeue_task_wrr(rq, rq->curr, 0);
}

static int select_min_wrr_rq(void)
{
	int cpu;
	int min_cpu;
	unsigned long min_load, cur_load;
	struct rq *rq;
	struct wrr_rq *wrr_rq;

	cpu = smp_processor_id();
	min_cpu = cpu;
	min_load = ULONG_MAX;

	for_each_online_cpu(cpu) {
		rq = cpu_rq(cpu);
		wrr_rq = &rq->wrr;

		raw_spin_lock(&wrr_rq->wrr_rq_lock);

		cur_load = wrr_rq->wrr_total_timeslice;

		if (cur_load < min_load) {
			min_load = cur_load;
			min_cpu = cpu;
		}

		raw_spin_unlock(&wrr_rq->wrr_rq_lock);
	}

	return min_cpu;
}

void check_preempt_wakeup_wrr(struct rq *rq, struct task_struct *p,
		int flags) { }

static struct task_struct *pick_next_task_wrr(struct rq *rq)
{
	struct sched_wrr_entity *wrr_se;
	struct task_struct *p;
	struct wrr_rq *wrr_rq = &rq->wrr;

	wrr_rq = &rq->wrr;
	if (!wrr_rq->wrr_nr_running)
		return NULL;

	wrr_se = list_entry(wrr_rq->wrr_rq_list.next,
		    struct sched_wrr_entity, run_list);

	p = wrr_task_of(wrr_se);

	return p;
}

static void put_prev_task_wrr(struct rq *rq, struct task_struct *prev)
{
}

#ifdef CONFIG_SMP
static int select_task_rq_wrr(struct task_struct *p, int sd_flag,
		int wake_flags)
{
	int cpu, best_cpu;

	rcu_read_lock();

	cpu = task_cpu(p);
	best_cpu = -1;

	if (p->policy == SCHED_WRR)
		best_cpu = select_min_wrr_rq();

	if (best_cpu != -1)
		cpu = best_cpu;

	rcu_read_unlock();

	return cpu;
}

static void set_curr_task_wrr(struct rq *rq)
{
}

static void set_cpus_allowed_wrr(struct task_struct *p,
				 const struct cpumask *new_mask)
{
}

static void rq_online_wrr(struct rq *rq)
{
}

static void rq_offline_wrr(struct rq *rq)
{
}

static void task_waking_wrr(struct task_struct *p)
{
}
#endif


static void task_tick_wrr(struct rq *rq, struct task_struct *p, int queued)
{
	struct sched_wrr_entity *wrr_se = &p->wrr;

	if (--wrr_se->wrr_timeslice)
		return;

	/* timeslice has decreased to 0 */
	if (wrr_se->wrr_weight > 1)
		wrr_se->wrr_weight--;
	wrr_se->wrr_timeslice = wrr_se->wrr_weight * wrr_unit_timeslice;

	requeue_task_wrr(rq, p, 0);
	resched_task(rq->curr);
}

static void prio_changed_wrr(struct rq *rq, struct task_struct *p, int oldprio)
{
}

static void switched_from_wrr(struct rq *rq, struct task_struct *p)
{
}

static void switched_to_wrr(struct rq *rq, struct task_struct *p)
{
}

static unsigned int get_rr_interval_wrr(struct rq *rq, struct task_struct *task)
{
	return 0;
}

/* function for syscall get_wrr_info */
int __get_wrr_info(struct wrr_info *p)
{
	struct rq *rq;
	struct wrr_rq *wrr_rq;
	int cpu, nr_cpu = 0;

	for_each_online_cpu(cpu) {
		if (nr_cpu >= MAX_CPUS)
			break;

		rq = cpu_rq(cpu);
		wrr_rq = &rq->wrr;

		p->nr_running[nr_cpu] = (int)wrr_rq->wrr_nr_running;
		p->total_weight[nr_cpu] = (int)wrr_rq->wrr_total_timeslice;

		nr_cpu++;
	}
	p->num_cpus = nr_cpu;

	return nr_cpu;
}

/* function for syscall set_wrr_weight */
void __set_wrr_weight(int new_weight)
{
	wrr_def_weight = new_weight;
}

/*
 * All the scheduling class methods:
 */
const struct sched_class wrr_sched_class = {

	.next                   = &fair_sched_class,
	.enqueue_task           = enqueue_task_wrr,
	.dequeue_task           = dequeue_task_wrr,
	.yield_task             = yield_task_wrr,

	.check_preempt_curr     = check_preempt_wakeup_wrr,

	.pick_next_task         = pick_next_task_wrr,
	.put_prev_task          = put_prev_task_wrr,

#ifdef CONFIG_SMP
	.select_task_rq         = select_task_rq_wrr,
	.set_cpus_allowed       = set_cpus_allowed_wrr,
	.rq_online              = rq_online_wrr,
	.rq_offline             = rq_offline_wrr,

	.task_waking            = task_waking_wrr,
#endif
	.set_curr_task          = set_curr_task_wrr,
	.task_tick              = task_tick_wrr,
	.prio_changed           = prio_changed_wrr,
	.switched_from          = switched_from_wrr,
	.switched_to            = switched_to_wrr,

	.get_rr_interval        = get_rr_interval_wrr,
};

#ifdef CONFIG_SCHED_DEBUG

void print_wrr_stats(struct seq_file *m, int cpu)
{
}
#endif
