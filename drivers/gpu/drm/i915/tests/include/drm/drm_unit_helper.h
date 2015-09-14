#ifndef __DRM_UNIT_HELPER_H_
#define __DRM_UNIT_HELPER_H_

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>
#include <limits.h>
#include <ctype.h>

#include <string.h>
#include <bsd/string.h>

#include <errno.h>

#include <linux/types.h>
#include <linux/i2c.h>

#include "../../../../../../../tools/include/linux/compiler.h"

typedef uint64_t u64;
typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t u8;

#define __printf(a, b)		__attribute__((format(printf, a, b)))
#define __read_mostly

#define EXPORT_SYMBOL(x)

static inline void mdelay(unsigned long t) {}
static inline void udelay(unsigned long t) {}
static inline void usleep_range(unsigned long t1, unsigned long t2) {}

struct i2c_adapter;

struct i2c_algorithm {
	int (*master_xfer)(struct i2c_adapter *, struct i2c_msg *, int);
	u32 (*functionality)(struct i2c_adapter *);
};

struct device {
	void *parent;
	int of_node;
};

struct i2c_adapter {
	struct device dev;
	const struct i2c_algorithm *algo;
	char name[48];
	void *algo_data;
	void *adapdata;
	int owner;
	int retries;
	unsigned int class;
};

static inline int
i2c_add_adapter(struct i2c_adapter *a)
{
	return 0;
}

static inline int
i2c_del_adapter(struct i2c_adapter *a)
{
	return 0;
}

#define THIS_MODULE 0
#define I2C_CLASS_DDC		(1<<3)	/* DDC bus on graphics adapters */


struct mutex {
};
static inline void mutex_lock(struct mutex *lock) {}
static inline void mutex_unlock(struct mutex *lock) {}
#define mutex_init(x)

#define module_param_unsafe(...)
#define MODULE_PARM_DESC(...)

#define DIV_ROUND_UP(n,d) (((n) + (d) - 1) / (d))
#define max(a,b) ((a) > (b) ? (a) : (b))
#define min(a,b) ((a) > (b) ? (b) : (a))
#define clamp(a,b,c) min(max((a), (b)), (c))

static inline const char *dev_name(const struct device *dev)
{
	return NULL;
}

struct drm_device {
};

struct drm_connector {
};

struct delayed_work {
};

struct notifier_block {
};

/* compiler.h stuff */
#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))

/**
 * Returns a pointer to the container of this list element.
 *
 * Example:
 * struct foo* f;
 * f = container_of(&foo->entry, struct foo, entry);
 * assert(f == foo);
 *
 * @param ptr Pointer to the struct list_head.
 * @param type Data type of the list element.
 * @param member Member name of the struct list_head field in the list element.
 * @return A pointer to the data struct containing the list head.
 */
#ifndef container_of
#define container_of(ptr, type, member) ({ \
    typeof(((type *)0)->member) *__mptr = (ptr); \
    (type *)((char *)(__mptr) - (char *) &((type *)0)->member); \
})
#endif


#endif /* __DRM_UNIT_HELPER_H_ */
