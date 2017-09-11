/*
 * light_event.c
 *
 * Implementation of syscalls:
 * - set_light_intensity
 * - get_light_intensity
 * -
 *
 */
#include <linux/unistd.h>
#include <linux/stddef.h>
#include <linux/light.h>
#include <linux/syscalls.h>
#include <linux/spinlock_types.h>
#include <linux/printk.h>
#include <linux/uidgid.h>


static LIST_HEAD(glob_light_evt_list);
static DEFINE_SPINLOCK(light_evt_lock);
static LIST_HEAD(glob_light_int_window);
static DEFINE_SPINLOCK(light_int_lock);
static int light_int_count;
static atomic_t max_id;
static int initialized;

/* functions */
static int light_evt_init(void);
static struct list_head *get_glob_light_evt_list(void);
static struct list_head *get_glob_light_int_window(void);
static int equal_reqs(struct event_requirements *x,
		      struct event_requirements *y);
static struct light_event *find_light_evt_by_req(
				struct event_requirements *req);
static struct light_event *find_light_evt_by_id(int id);
static struct light_event *new_light_evt_entry(struct event_requirements *req);
static void free_light_evt_entry(struct light_event *entry);
static int light_event_add_id(struct light_event *entry);
static int light_event_del_id(struct light_event *entry, int id);
static int light_event_occur(struct light_event *entry);
static int update_light_int_window(struct light_intensity *intensity);


/*
 * initialize globa data
 */
static int light_evt_init(void)
{
	if (initialized)
		return 0;

	initialized = 1;

	/* initialize global light event list */
	INIT_LIST_HEAD(&glob_light_evt_list);

	/* initialize global light intensity window */
	INIT_LIST_HEAD(&glob_light_int_window);
	light_int_count = 0;

	atomic_set(&max_id, 0);

	return 0;
}

/*
 * returns the global light event list
 */
static struct list_head *get_glob_light_evt_list(void)
{
	light_evt_init();
	return &glob_light_evt_list;
}

/*
 * returns the global light intensity window (list)
 */
static struct list_head *get_glob_light_int_window(void)
{
	light_evt_init();
	return &glob_light_int_window;
}

/*
 * compares two light event requirements
 * returns 1 if the requirements are identical
 */
static int equal_reqs(struct event_requirements *x,
		      struct event_requirements *y)
{
	if (!x || !y)
		return 0;

	if (x->req_intensity != y->req_intensity)
		return 0;

	if (x->frequency != y->frequency)
		return 0;

	return 1;
}

/*
 * finds and returns the light event in global light event list by the
 * event requirement
 */
static struct light_event *find_light_evt_by_req(struct event_requirements *req)
{
	struct list_head *pos;
	struct light_event *tmp;

	list_for_each(pos, get_glob_light_evt_list()) {
		tmp = list_entry(pos, struct light_event, evt_entry);
		if (tmp && equal_reqs(req, &tmp->req))
			return tmp;
	}

	return NULL;
}

/*
 * finds and returns the light event in global light event list by the
 * event id
 */
static struct light_event *find_light_evt_by_id(int id)
{
	struct list_head *pos;
	struct light_event *tmp;
	struct list_head *id_pos;
	struct evt_id_entry *id_tmp;

	list_for_each(pos, get_glob_light_evt_list()) {
		tmp = list_entry(pos, struct light_event, evt_entry);
		if (!tmp)
			continue;

		list_for_each(id_pos, &tmp->id_list) {
			id_tmp = list_entry(id_pos, struct evt_id_entry,
					   entry);
			if (id_tmp && id_tmp->id == id)
				return tmp;
		}
	}

	return NULL;
}

/*
 * allocates and initializes a new light event entry and
 * add the entry in the global light event list
 *
 * returns the pointer to the new light event entry
 */
static struct light_event *new_light_evt_entry(struct event_requirements *req)
{
	struct light_event *entry;

	entry = kmalloc(sizeof(struct light_event),
					      GFP_KERNEL);
	if (!entry)
		return NULL;

	entry->nr_event = 0;
	entry->req.req_intensity = req->req_intensity;
	entry->req.frequency = req->frequency;
	INIT_LIST_HEAD(&entry->id_list);
	init_waitqueue_head(&entry->queue);

	/* add the entry into the global light event list */
	list_add(&entry->evt_entry, get_glob_light_evt_list());

	return entry;
}

/*
 * removes the light event entry from the global light event list and
 * free the entry
 */
static void free_light_evt_entry(struct light_event *entry)
{
	struct list_head *pos, *n;
	struct evt_id_entry *id_entry;

	list_for_each_safe(pos, n, &entry->id_list) {
		list_del(pos);
		id_entry = list_entry(pos, struct evt_id_entry, entry);
		kfree(id_entry);
	}
	list_del(&entry->evt_entry);
	kfree(entry);
}

/*
 * allocates a new event id and associates the id with its belonging light
 * event entry
 *
 * returns the new event id
 */
static int light_event_add_id(struct light_event *entry)
{
	int new_id;
	struct evt_id_entry *id_entry;

	id_entry = kmalloc(sizeof(struct evt_id_entry), GFP_KERNEL);
	if (!id_entry)
		return -1;

	new_id = atomic_add_return(1, &max_id);
	id_entry->id = new_id;
	list_add(&id_entry->entry, &entry->id_list);
	entry->nr_event++;

	return new_id;
}

/*
 * removes the event id from its associated light event entry
 *
 * returns the number of event id(s) associated with the light event
 * entry after the input event id is removed
 */
static int light_event_del_id(struct light_event *entry, int id)
{
	struct list_head *pos;
	struct evt_id_entry *tmp;
	struct evt_id_entry *found = NULL;

	list_for_each(pos, &entry->id_list) {
		tmp = list_entry(pos, struct evt_id_entry, entry);
		if (tmp && tmp->id == id) {
			found = tmp;
			break;
		}
	}

	if (found) {
		list_del(&found->entry);
		kfree(found);
		entry->nr_event--;
	}

	return entry->nr_event;
}

/*
 * determines whether the input light event has occurred
 *
 * a light event occurs when the number of light intensities in the
 * global light intensity window that exceed the required intensity
 * (with tolerance to some noise) is no less than the required frequency
 *
 * returns 1 if occurred, 0 otherwise
 */
static int light_event_occur(struct light_event *entry)
{
	int ret = 0;
	int count = 0;
	int req_intensity = entry->req.req_intensity;
	int frequency = entry->req.frequency;
	struct list_head *pos;
	struct light_int_entry *tmp;

	list_for_each(pos, get_glob_light_int_window()) {
		tmp = list_entry(pos, struct light_int_entry, entry);
		if (tmp && INTENSITY_IN_RANGE(tmp->intensity.cur_intensity,
					      req_intensity, NOISE))
			count++;
	}

	if (count >= frequency)
		ret = 1;

	return ret;
}

/*
 * updates the global light intensity window (list) with the latest intensity
 *
 * the number of intensity records stored in the window is bounded by the
 * macro WINDOW, if the list is full, the oldest intensity record will be
 * replaced by the latest(input) intensity
 */
static int update_light_int_window(struct light_intensity *intensity)
{
	struct light_int_entry *p;
	struct light_int_entry *prev;
	struct list_head *window = get_glob_light_int_window();

	p = kmalloc(sizeof(struct light_int_entry), GFP_KERNEL);
	if (!p)
		return -ENOMEM;

	p->intensity.cur_intensity = intensity->cur_intensity;
	list_add(&p->entry, window);

	/* number of entries should not exceed WINDOW */
	if (++light_int_count > WINDOW) {
		light_int_count--;

		prev = list_entry(window->prev, struct light_int_entry, entry);
		if (prev) {
			list_del(&prev->entry);
			kfree(prev);
		}
	}

	return 0;
}

/*
 * Syscall: sys_set_light_intensity
 *
 * Set current ambient intensity in the kernel.
 *
 * The parameter user_light_intensity is the pointer to the address
 * where the sensor data is stored in user space. Follow system call
 * convention to return 0 on success and the appropriate error value
 * on failure.
 *
 * syscall number 244
 */
SYSCALL_DEFINE1(set_light_intensity, struct light_intensity *,
		user_light_intensity)
{
	int retval = 0;

	return retval;
}

/*
 * Syscall: sys_get_light_intensity
 *
 * Retrive the scaled intensity set in the kernel.
 *
 * The same convention as the previous system call but
 * you are reading the value that was just set.
 * Handle error cases appropriately and return values according to
 * convention.
 * The calling process should provide memory in userspace to return the
 * intensity.
 *
 * syscall number 245
 */
SYSCALL_DEFINE1(get_light_intensity, struct light_intensity *,
		user_light_intensity)
{
	int retval = 0;
	return retval;
}

/*
 * Syscall: sys_light_evt_create
 *
 * Create an event based on light intensity.
 *
 * If frequency exceeds WINDOW, cap it at WINDOW.
 * Return an event_id on success and the appropriate error on failure.
 *
 * system call number 246
 */
SYSCALL_DEFINE1(light_evt_create, struct event_requirements *,
		intensity_params)
{
	int retval = 0;
	struct event_requirements req;
	struct light_event *entry;

	if (!intensity_params) {
		retval = -EINVAL;
		goto out;
	}

	if (copy_from_user(&req, intensity_params,
			   sizeof(struct event_requirements))) {
		retval = -EFAULT;
		goto out;
	}

	/* validate the input requirement */
	if (req.req_intensity < 0 || req.frequency <= 0) {
		retval = -EINVAL;
		goto out;
	}
	if (req.frequency > WINDOW)
		req.frequency = WINDOW;

	spin_lock(&light_evt_lock);
	entry = find_light_evt_by_req(&req);

	if (!entry) {
		/* create new entry */
		entry = new_light_evt_entry(&req);
	}

	if (!entry) {
		retval = -ENOMEM;
		goto unlock_out;
	}

	retval = light_event_add_id(entry);

unlock_out:
	spin_unlock(&light_evt_lock);
out:
	return retval;
}

/*
 * Syscall: sys_light_evt_wait
 *
 * Block a process on an event.
 *
 * It takes the event_id as parameter. The event_id requires verification.
 * Return 0 on success and the appropriate error on failure.
 *
 * system call number 247
 */
SYSCALL_DEFINE1(light_evt_wait, int, event_id)
{
	int retval = 0;
	struct light_event *entry;
	DEFINE_WAIT(wait);

	if (event_id <= 0)
		return -EINVAL;

	spin_lock(&light_evt_lock);

	entry = find_light_evt_by_id(event_id);
	if (!entry) {
		spin_unlock(&light_evt_lock);
		return -EINVAL;
	}

	add_wait_queue(&entry->queue, &wait);

	spin_lock(&light_int_lock);
	while (!light_event_occur(entry)) {
		prepare_to_wait(&entry->queue, &wait, TASK_INTERRUPTIBLE);

		spin_unlock(&light_int_lock);
		spin_unlock(&light_evt_lock);

		schedule();

		spin_lock(&light_evt_lock);
		spin_lock(&light_int_lock);

		if (!find_light_evt_by_id(event_id)) {
			retval = -EINVAL;
			goto unlock_out;
		}
	}
	finish_wait(&entry->queue, &wait);

unlock_out:
	spin_unlock(&light_int_lock);
	spin_unlock(&light_evt_lock);

	return retval;
}


/*
 * Syscall: sys_light_evt_signal
 *
 * The light_evt_signal system call.
 *
 * Takes sensor data from user, stores the data in the kernel,
 * and notifies all open events whose
 * baseline is surpassed.  All processes waiting on a given event
 * are unblocked.
 *
 * Return 0 success and the appropriate error on failure.
 *
 * system call number 248
 */
SYSCALL_DEFINE1(light_evt_signal, struct light_intensity *,
		user_light_intensity)
{
	int retval = 0;
	struct light_intensity intensity;
	struct list_head *pos;
	struct light_event *tmp;

	if (!user_light_intensity) {
		retval = -EINVAL;
		goto out;
	}

	if (copy_from_user(&intensity, user_light_intensity,
			   sizeof(struct light_intensity))) {
		retval = -EFAULT;
		goto out;
	}

	spin_lock(&light_int_lock);
	retval = update_light_int_window(&intensity);
	if (retval < 0)
		goto unlock_int_out;

	/* traverse the light event list to unblock events that occur */
	spin_lock(&light_evt_lock);
	list_for_each(pos, get_glob_light_evt_list()) {
		tmp = list_entry(pos, struct light_event, evt_entry);
		if (tmp && light_event_occur(tmp))
			wake_up(&tmp->queue);
	}

	spin_unlock(&light_evt_lock);
unlock_int_out:
	spin_unlock(&light_int_lock);
out:
	return retval;
}

/*
 * Syscall: sys_light_evt_destroy
 *
 * Destroy an event using the event_id.
 *
 * Return 0 on success and the appropriate error on failure.
 *
 * system call number 249
 */
SYSCALL_DEFINE1(light_evt_destroy, int, event_id)
{
	int retval = 0;
	int nr;
	struct light_event *entry;

	if (event_id <= 0)
		return -EINVAL;

	spin_lock(&light_evt_lock);

	entry = find_light_evt_by_id(event_id);
	if (!entry) {
		retval = -EINVAL;
		goto unlock_out;
	}

	nr = light_event_del_id(entry, event_id);
	wake_up(&entry->queue);
	if (nr == 0) {
		/* now the light event has no associated event_id */
		free_light_evt_entry(entry);
	}

unlock_out:
	spin_unlock(&light_evt_lock);

	return retval;
}
