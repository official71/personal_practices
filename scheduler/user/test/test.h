#define SCHED_WRR 6

#define SC_GET_WRR_INFO 244
#define SC_SET_WRR_WEIGHT 245
#define SC_GET_CPU 168

#define MAX_CPUS 8 /* upper bound of the number of CPUs */
struct wrr_info {
	int num_cpus;
	int nr_running[MAX_CPUS];
	int total_weight[MAX_CPUS];
};

