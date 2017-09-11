#ifndef LIGHT_EVENT_H
#define LIGHT_EVENT_H
/*
 * light_event.h
 */

#include <linux/types.h>
#include <linux/list.h>
#include <linux/wait.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/atomic.h>

#define NOISE 10
#define WINDOW 20

/*
 * whether the intensity i meets the required range
 * defined by required intensity r and noise n
 */
#define INTENSITY_IN_RANGE(i, r, n) ((i) >= (r)-(n))

/*
 * The data structure for passing light intensity data to the
 * kernel and storing the data in the kernel.
 */
struct light_intensity {
	/* scaled intensity as read from the light sensor */
	int cur_intensity;
};

/*
 * Defines a light event.
 *
 * Event is defined by a required intensity and frequency.
 */
struct event_requirements {
	int req_intensity; /* scaled value of light intensity in centi-lux */
	int frequency;     /* number of samples with intensity-noise > req_intensity */
};

/*
 * light event data structure
 */
struct light_event {
	int nr_event; /* num of event id(s) associated */
	struct event_requirements req;
	wait_queue_head_t queue;
	struct list_head evt_entry; /* entry in event list */
	struct list_head id_list; /* list of event id(s) */
};

/*
 * entry in the event id list under light event data structure
 */
struct evt_id_entry {
	int id;
	struct list_head entry;
};

/*
 * entry in the light intensity window(list)
 */
struct light_int_entry {
	struct light_intensity intensity;
	struct list_head entry;
};
#endif

