#ifndef _DTRACE_DEV_H_
#define _DTRACE_DEV_H_

#define DT_DEV_DTRACE_MINOR	(16)
#define DT_DEV_HELPER_MINOR	(DT_DEV_DTRACE_MINOR + 1)
#define DT_DEV_PROFILE_MINOR	(DT_DEV_HELPER_MINOR + 1)
#define DT_DEV_SYSTRACE_MINOR	(DT_DEV_PROFILE_MINOR + 1)
#define DT_DEV_FBT_MINOR	(DT_DEV_SYSTRACE_MINOR + 1)
#define DT_DEV_SDT_MINOR	(DT_DEV_FBT_MINOR + 1)
#define DT_DEV_FASTTRAP_MINOR	(DT_DEV_SDT_MINOR + 1)
#define DT_DEV_LOCKSTAT_MINOR	(DT_DEV_FASTTRAP_MINOR + 1)
#define DT_DEV_DT_TEST_MINOR	(DT_DEV_LOCKSTAT_MINOR + 1)

extern int dtrace_dev_init(void);
extern void dtrace_dev_exit(void);

#endif /* _DTRACE_DEV_H_ */
