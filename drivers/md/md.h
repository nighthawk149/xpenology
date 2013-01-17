/*
   md.h : kernel internal structure of the Linux MD driver
          Copyright (C) 1996-98 Ingo Molnar, Gadi Oxman
	  
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.
   
   You should have received a copy of the GNU General Public License
   (for example /usr/src/linux/COPYING); if not, write to the Free
   Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  
*/

#ifndef _MD_MD_H
#define _MD_MD_H

#include <linux/blkdev.h>
#include <linux/kobject.h>
#include <linux/list.h>
#include <linux/mm.h>
#include <linux/mutex.h>
#include <linux/timer.h>
#include <linux/wait.h>
#include <linux/workqueue.h>

#ifdef MY_ABC_HERE
#include <linux/raid/libmd-report.h>
#endif

#define MaxSector (~(sector_t)0)

/* Bad block numbers are stored sorted in a single page.
 * 64bits is used for each block or extent.
 * 54 bits are sector number, 9 bits are extent size,
 * 1 bit is an 'acknowledged' flag.
 */
#define MD_MAX_BADBLOCKS	(PAGE_SIZE/8)

#ifdef MY_ABC_HERE
typedef struct _tag_SYNO_WAKEUP_DEVICE_WORK{
    struct work_struct work;
    struct mddev *mddev;
} SYNO_WAKEUP_DEVICE_WORK;
#endif

/*
 * MD's 'extended' device
 */
struct md_rdev {
	struct list_head same_set;	/* RAID devices within the same set */

	sector_t sectors;		/* Device size (in 512bytes sectors) */
	struct mddev *mddev;		/* RAID array if running */
	int last_events;		/* IO event timestamp */

	/*
	 * If meta_bdev is non-NULL, it means that a separate device is
	 * being used to store the metadata (superblock/bitmap) which
	 * would otherwise be contained on the same device as the data (bdev).
	 */
	struct block_device *meta_bdev;
	struct block_device *bdev;	/* block device handle */

	struct page	*sb_page, *bb_page;
#ifdef MY_ABC_HERE
	struct page	*wakeup_page;
#endif
	int		sb_loaded;
	__u64		sb_events;
	sector_t	data_offset;	/* start of data in array */
	sector_t 	sb_start;	/* offset of the super block (in 512byte sectors) */
	int		sb_size;	/* bytes in the superblock */
	int		preferred_minor;	/* autorun support */

	struct kobject	kobj;

	/* A device can be in one of three states based on two flags:
	 * Not working:   faulty==1 in_sync==0
	 * Fully working: faulty==0 in_sync==1
	 * Working, but not
	 * in sync with array
	 *                faulty==0 in_sync==0
	 *
	 * It can never have faulty==1, in_sync==1
	 * This reduces the burden of testing multiple flags in many cases
	 */

	unsigned long	flags;
#define	Faulty		1		/* device is known to have a fault */
#define	In_sync		2		/* device is in_sync with rest of array */
#define	WriteMostly	4		/* Avoid reading if at all possible */
#define	AutoDetected	7		/* added by auto-detect */
#define Blocked		8		/* An error occurred but has not yet
					 * been acknowledged by the metadata
					 * handler, so don't allow writes
					 * until it is cleared */
#define WriteErrorSeen	9		/* A write error has been seen on this
					 * device
					 */
#define FaultRecorded	10		/* Intermediate state for clearing
					 * Blocked.  The Fault is/will-be
					 * recorded in the metadata, but that
					 * metadata hasn't been stored safely
					 * on disk yet.
					 */
#define BlockedBadBlocks 11		/* A writer is blocked because they
					 * found an unacknowledged bad-block.
					 * This can safely be cleared at any
					 * time, and the writer will re-check.
					 * It may be set at any time, and at
					 * worst the writer will timeout and
					 * re-check.  So setting it as
					 * accurately as possible is good, but
					 * not absolutely critical.
					 */
#ifdef MY_ABC_HERE
#define DiskError	12		/* device is know to have a fault in degraded state */
#endif /* MY_ABC_HERE */
	wait_queue_head_t blocked_wait;

	int desc_nr;			/* descriptor index in the superblock */
	int raid_disk;			/* role of device in array */
	int new_raid_disk;		/* role that the device will have in
					 * the array after a level-change completes.
					 */
	int saved_raid_disk;		/* role that device used to have in the
					 * array and could again if we did a partial
					 * resync from the bitmap
					 */
	sector_t	recovery_offset;/* If this device has been partially
					 * recovered, this is where we were
					 * up to.
					 */

	atomic_t	nr_pending;	/* number of pending requests.
					 * only maintained for arrays that
					 * support hot removal
					 */
	atomic_t	read_errors;	/* number of consecutive read errors that
					 * we have tried to ignore.
					 */
	struct timespec last_read_error;	/* monotonic time since our
						 * last read error
						 */
	atomic_t	corrected_errors; /* number of corrected read errors,
					   * for reporting to userspace and storing
					   * in superblock.
					   */
	struct work_struct del_work;	/* used for delayed sysfs removal */

	struct sysfs_dirent *sysfs_state; /* handle for 'state'
					   * sysfs entry */

	struct badblocks {
		int	count;		/* count of bad blocks */
		int	unacked_exist;	/* there probably are unacknowledged
					 * bad blocks.  This is only cleared
					 * when a read discovers none
					 */
		int	shift;		/* shift from sectors to block size
					 * a -ve shift means badblocks are
					 * disabled.*/
		u64	*page;		/* badblock list */
		int	changed;
		seqlock_t lock;

		sector_t sector;
		sector_t size;		/* in sectors */
	} badblocks;
};

#ifdef MY_ABC_HERE
typedef struct _tag_SYNO_UPDATE_SB_WORK{
    struct work_struct work;
    struct mddev *mddev;
}SYNO_UPDATE_SB_WORK;
#endif

#define BB_LEN_MASK	(0x00000000000001FFULL)
#define BB_OFFSET_MASK	(0x7FFFFFFFFFFFFE00ULL)
#define BB_ACK_MASK	(0x8000000000000000ULL)
#define BB_MAX_LEN	512
#define BB_OFFSET(x)	(((x) & BB_OFFSET_MASK) >> 9)
#define BB_LEN(x)	(((x) & BB_LEN_MASK) + 1)
#define BB_ACK(x)	(!!((x) & BB_ACK_MASK))
#define BB_MAKE(a, l, ack) (((a)<<9) | ((l)-1) | ((u64)(!!(ack)) << 63))

extern int md_is_badblock(struct badblocks *bb, sector_t s, int sectors,
			  sector_t *first_bad, int *bad_sectors);
static inline int is_badblock(struct md_rdev *rdev, sector_t s, int sectors,
			      sector_t *first_bad, int *bad_sectors)
{
	if (unlikely(rdev->badblocks.count)) {
		int rv = md_is_badblock(&rdev->badblocks, rdev->data_offset + s,
					sectors,
					first_bad, bad_sectors);
		if (rv)
			*first_bad -= rdev->data_offset;
		return rv;
	}
	return 0;
}
extern int rdev_set_badblocks(struct md_rdev *rdev, sector_t s, int sectors,
			      int acknowledged);
extern int rdev_clear_badblocks(struct md_rdev *rdev, sector_t s, int sectors);
extern void md_ack_all_badblocks(struct badblocks *bb);

struct mddev {
	void				*private;
	struct md_personality		*pers;
	dev_t				unit;
	int				md_minor;
	struct list_head 		disks;
	unsigned long			flags;
#define MD_CHANGE_DEVS	0	/* Some device status has changed */
#define MD_CHANGE_CLEAN 1	/* transition to or from 'clean' */
#define MD_CHANGE_PENDING 2	/* switch from 'clean' to 'active' in progress */
#define MD_ARRAY_FIRST_USE 3    /* First use of array, needs initialization */

	int				suspended;
	atomic_t			active_io;
	int				ro;
	int				sysfs_active; /* set when sysfs deletes
						       * are happening, so run/
						       * takeover/stop are not safe
						       */
	int				ready; /* See when safe to pass 
						* IO requests down */
	struct gendisk			*gendisk;

	struct kobject			kobj;
	int				hold_active;
#define	UNTIL_IOCTL	1
#define	UNTIL_STOP	2

	/* Superblock information */
	int				major_version,
					minor_version,
					patch_version;
	int				persistent;
	int 				external;	/* metadata is
							 * managed externally */
	char				metadata_type[17]; /* externally set*/
	int				chunk_sectors;
	time_t				ctime, utime;
	int				level, layout;
	char				clevel[16];
	int				raid_disks;
	int				max_disks;
	sector_t			dev_sectors; 	/* used size of
							 * component devices */
	sector_t			array_sectors; /* exported array size */
	int				external_size; /* size managed
							* externally */
	__u64				events;
	/* If the last 'event' was simply a clean->dirty transition, and
	 * we didn't write it to the spares, then it is safe and simple
	 * to just decrement the event count on a dirty->clean transition.
	 * So we record that possibility here.
	 */
	int				can_decrease_events;

	char				uuid[16];

	/* If the array is being reshaped, we need to record the
	 * new shape and an indication of where we are up to.
	 * This is written to the superblock.
	 * If reshape_position is MaxSector, then no reshape is happening (yet).
	 */
	sector_t			reshape_position;
	int				delta_disks, new_level, new_layout;
	int				new_chunk_sectors;

	atomic_t			plug_cnt;	/* If device is expecting
							 * more bios soon.
							 */
	struct md_thread		*thread;	/* management thread */
	struct md_thread		*sync_thread;	/* doing resync or reconstruct */
	sector_t			curr_resync;	/* last block scheduled */
	/* As resync requests can complete out of order, we cannot easily track
	 * how much resync has been completed.  So we occasionally pause until
	 * everything completes, then set curr_resync_completed to curr_resync.
	 * As such it may be well behind the real resync mark, but it is a value
	 * we are certain of.
	 */
	sector_t			curr_resync_completed;
	unsigned long			resync_mark;	/* a recent timestamp */
	sector_t			resync_mark_cnt;/* blocks written at resync_mark */
	sector_t			curr_mark_cnt; /* blocks scheduled now */

	sector_t			resync_max_sectors; /* may be set by personality */

	sector_t			resync_mismatches; /* count of sectors where
							    * parity/replica mismatch found
							    */

	/* allow user-space to request suspension of IO to regions of the array */
	sector_t			suspend_lo;
	sector_t			suspend_hi;
	/* if zero, use the system-wide default */
	int				sync_speed_min;
	int				sync_speed_max;

	/* resync even though the same disks are shared among md-devices */
	int				parallel_resync;

	int				ok_start_degraded;
	/* recovery/resync flags 
	 * NEEDED:   we might need to start a resync/recover
	 * RUNNING:  a thread is running, or about to be started
	 * SYNC:     actually doing a resync, not a recovery
	 * RECOVER:  doing recovery, or need to try it.
	 * INTR:     resync needs to be aborted for some reason
	 * DONE:     thread is done and is waiting to be reaped
	 * REQUEST:  user-space has requested a sync (used with SYNC)
	 * CHECK:    user-space request for check-only, no repair
	 * RESHAPE:  A reshape is happening
	 *
	 * If neither SYNC or RESHAPE are set, then it is a recovery.
	 */
#define	MD_RECOVERY_RUNNING	0
#define	MD_RECOVERY_SYNC	1
#define	MD_RECOVERY_RECOVER	2
#define	MD_RECOVERY_INTR	3
#define	MD_RECOVERY_DONE	4
#define	MD_RECOVERY_NEEDED	5
#define	MD_RECOVERY_REQUESTED	6
#define	MD_RECOVERY_CHECK	7
#define MD_RECOVERY_RESHAPE	8
#define	MD_RECOVERY_FROZEN	9

	unsigned long			recovery;
	/* If a RAID personality determines that recovery (of a particular
	 * device) will fail due to a read error on the source device, it
	 * takes a copy of this number and does not attempt recovery again
	 * until this number changes.
	 */
	int				recovery_disabled;

	int				in_sync;	/* know to not need resync */
	/* 'open_mutex' avoids races between 'md_open' and 'do_md_stop', so
	 * that we are never stopping an array while it is open.
	 * 'reconfig_mutex' protects all other reconfiguration.
	 * These locks are separate due to conflicting interactions
	 * with bdev->bd_mutex.
	 * Lock ordering is:
	 *  reconfig_mutex -> bd_mutex : e.g. do_md_run -> revalidate_disk
	 *  bd_mutex -> open_mutex:  e.g. __blkdev_get -> md_open
	 */
	struct mutex			open_mutex;
	struct mutex			reconfig_mutex;
	atomic_t			active;		/* general refcount */
	atomic_t			openers;	/* number of active opens */

	int				changed;	/* True if we might need to
							 * reread partition info */
	int				degraded;	/* whether md should consider
							 * adding a spare
							 */

	atomic_t			recovery_active; /* blocks scheduled, but not written */
	wait_queue_head_t		recovery_wait;
	sector_t			recovery_cp;
	sector_t			resync_min;	/* user requested sync
							 * starts here */
	sector_t			resync_max;	/* resync should pause
							 * when it gets here */

	struct sysfs_dirent		*sysfs_state;	/* handle for 'array_state'
							 * file in sysfs.
							 */
	struct sysfs_dirent		*sysfs_action;  /* handle for 'sync_action' */

	struct work_struct del_work;	/* used for delayed sysfs removal */

	spinlock_t			write_lock;
	wait_queue_head_t		sb_wait;	/* for waiting on superblock updates */
	atomic_t			pending_writes;	/* number of active superblock writes */

	unsigned int			safemode;	/* if set, update "clean" superblock
							 * when no writes pending.
							 */ 
	unsigned int			safemode_delay;
	struct timer_list		safemode_timer;
	atomic_t			writes_pending; 
	struct request_queue		*queue;	/* for plugging ... */

	struct bitmap                   *bitmap; /* the bitmap for the device */
	struct {
		struct file		*file; /* the bitmap file */
		loff_t			offset; /* offset from superblock of
						 * start of bitmap. May be
						 * negative, but not '0'
						 * For external metadata, offset
						 * from start of device. 
						 */
		loff_t			default_offset; /* this is the offset to use when
							 * hot-adding a bitmap.  It should
							 * eventually be settable by sysfs.
							 */
		struct mutex		mutex;
		unsigned long		chunksize;
		unsigned long		daemon_sleep; /* how many jiffies between updates? */
		unsigned long		max_write_behind; /* write-behind mode */
		int			external;
	} bitmap_info;

	atomic_t 			max_corr_read_errors; /* max read retries */
	struct list_head		all_mddevs;
#ifdef MY_ABC_HERE
	unsigned char			blActive;  /* to record whether this md is in active or not */
	spinlock_t				ActLock;   /* lock for Active attr. */
	unsigned long			ulLastReq; /* the last time received request */
#endif
#ifdef MY_ABC_HERE
    unsigned char           nodev_and_crashed;     // 1 ==> nodev && crashed. deny make_request
#endif
#ifdef MY_ABC_HERE
	unsigned char			auto_remap;     // 1 ==> set all rdevs to remap mode.
#endif
#ifdef MY_ABC_HERE
	unsigned char                   force_auto_remap; // user open auto remap manually
	void                            *syno_private;    // store lv struct for auto remap report
	char                            lv_name[16];
#endif

	struct attribute_group		*to_remove;

	struct bio_set			*bio_set;

	/* Generic flush handling.
	 * The last to finish preflush schedules a worker to submit
	 * the rest of the request (without the REQ_FLUSH flag).
	 */
	struct bio *flush_bio;
	atomic_t flush_pending;
	struct work_struct flush_work;
	struct work_struct event_work;	/* used by dm to report failure event */
	void (*sync_super)(struct mddev *mddev, struct md_rdev *rdev);
};


static inline void rdev_dec_pending(struct md_rdev *rdev, struct mddev *mddev)
{
	int faulty = test_bit(Faulty, &rdev->flags);
	if (atomic_dec_and_test(&rdev->nr_pending) && faulty)
		set_bit(MD_RECOVERY_NEEDED, &mddev->recovery);
}

static inline void md_sync_acct(struct block_device *bdev, unsigned long nr_sectors)
{
        atomic_add(nr_sectors, &bdev->bd_contains->bd_disk->sync_io);
}

struct md_personality
{
	char *name;
	int level;
	struct list_head list;
	struct module *owner;
	void (*make_request)(struct mddev *mddev, struct bio *bio);
	int (*run)(struct mddev *mddev);
	int (*stop)(struct mddev *mddev);
	void (*status)(struct seq_file *seq, struct mddev *mddev);
	/* error_handler must set ->faulty and clear ->in_sync
	 * if appropriate, and should abort recovery if needed 
	 */
#ifdef MY_ABC_HERE
	/**
	 *  for our special purpose, like raid1, there is not exist a
	 *  easy way for distinguish between hotplug or read/write error
	 *  on last one disk which is in sync
	 */
	void (*syno_error_handler)(struct mddev *mddev, struct md_rdev *rdev);
#endif /* MY_ABC_HERE */
	void (*error_handler)(struct mddev *mddev, struct md_rdev *rdev);
	int (*hot_add_disk) (struct mddev *mddev, struct md_rdev *rdev);
	int (*hot_remove_disk) (struct mddev *mddev, int number);
	int (*spare_active) (struct mddev *mddev);
	sector_t (*sync_request)(struct mddev *mddev, sector_t sector_nr, int *skipped, int go_faster);
	int (*resize) (struct mddev *mddev, sector_t sectors);
	sector_t (*size) (struct mddev *mddev, sector_t sectors, int raid_disks);
	int (*check_reshape) (struct mddev *mddev);
	int (*start_reshape) (struct mddev *mddev);
	void (*finish_reshape) (struct mddev *mddev);
	/* quiesce moves between quiescence states
	 * 0 - fully active
	 * 1 - no new requests allowed
	 * others - reserved
	 */
	void (*quiesce) (struct mddev *mddev, int state);
	/* takeover is used to transition an array from one
	 * personality to another.  The new personality must be able
	 * to handle the data in the current layout.
	 * e.g. 2drive raid1 -> 2drive raid5
	 *      ndrive raid5 -> degraded n+1drive raid6 with special layout
	 * If the takeover succeeds, a new 'private' structure is returned.
	 * This needs to be installed and then ->run used to activate the
	 * array.
	 */
#ifdef MY_ABC_HERE
	unsigned char (*ismaxdegrade) (struct mddev *mddev);
#endif
	void *(*takeover) (struct mddev *mddev);
};


struct md_sysfs_entry {
	struct attribute attr;
	ssize_t (*show)(struct mddev *, char *);
	ssize_t (*store)(struct mddev *, const char *, size_t);
};
extern struct attribute_group md_bitmap_group;

static inline struct sysfs_dirent *sysfs_get_dirent_safe(struct sysfs_dirent *sd, char *name)
{
	if (sd)
		return sysfs_get_dirent(sd, NULL, name);
	return sd;
}
static inline void sysfs_notify_dirent_safe(struct sysfs_dirent *sd)
{
	if (sd)
		sysfs_notify_dirent(sd);
}

static inline char * mdname (struct mddev * mddev)
{
	return mddev->gendisk ? mddev->gendisk->disk_name : "mdX";
}

static inline int sysfs_link_rdev(struct mddev *mddev, struct md_rdev *rdev)
{
	char nm[20];
	sprintf(nm, "rd%d", rdev->raid_disk);
	return sysfs_create_link(&mddev->kobj, &rdev->kobj, nm);
}

static inline void sysfs_unlink_rdev(struct mddev *mddev, struct md_rdev *rdev)
{
	char nm[20];
	sprintf(nm, "rd%d", rdev->raid_disk);
	sysfs_remove_link(&mddev->kobj, nm);
}

/*
 * iterates through some rdev ringlist. It's safe to remove the
 * current 'rdev'. Dont touch 'tmp' though.
 */
#define rdev_for_each_list(rdev, tmp, head)				\
	list_for_each_entry_safe(rdev, tmp, head, same_set)

/*
 * iterates through the 'same array disks' ringlist
 */
#define rdev_for_each(rdev, tmp, mddev)				\
	list_for_each_entry_safe(rdev, tmp, &((mddev)->disks), same_set)

#define rdev_for_each_rcu(rdev, mddev)				\
	list_for_each_entry_rcu(rdev, &((mddev)->disks), same_set)

struct md_thread {
	void			(*run) (struct mddev *mddev);
	struct mddev		*mddev;
	wait_queue_head_t	wqueue;
	unsigned long           flags;
	struct task_struct	*tsk;
	unsigned long		timeout;
};

#define THREAD_WAKEUP  0

#define __wait_event_lock_irq(wq, condition, lock, cmd) 		\
do {									\
	wait_queue_t __wait;						\
	init_waitqueue_entry(&__wait, current);				\
									\
	add_wait_queue(&wq, &__wait);					\
	for (;;) {							\
		set_current_state(TASK_UNINTERRUPTIBLE);		\
		if (condition)						\
			break;						\
		spin_unlock_irq(&lock);					\
		cmd;							\
		schedule();						\
		spin_lock_irq(&lock);					\
	}								\
	current->state = TASK_RUNNING;					\
	remove_wait_queue(&wq, &__wait);				\
} while (0)

#define wait_event_lock_irq(wq, condition, lock, cmd) 			\
do {									\
	if (condition)	 						\
		break;							\
	__wait_event_lock_irq(wq, condition, lock, cmd);		\
} while (0)

static inline void safe_put_page(struct page *p)
{
	if (p) put_page(p);
}

#ifdef MY_ABC_HERE
extern void syno_md_error (struct mddev *mddev, struct md_rdev *rdev);
extern int IsDeviceDisappear(struct block_device *bdev);
#endif
#ifdef MY_ABC_HERE
extern void SynoUpdateSBTask(struct work_struct *work);
#endif
extern int register_md_personality(struct md_personality *p);
extern int unregister_md_personality(struct md_personality *p);
extern struct md_thread *md_register_thread(
	void (*run)(struct mddev *mddev),
	struct mddev *mddev,
	const char *name);
extern void md_unregister_thread(struct md_thread **threadp);
extern void md_wakeup_thread(struct md_thread *thread);
extern void md_check_recovery(struct mddev *mddev);
extern void md_write_start(struct mddev *mddev, struct bio *bi);
extern void md_write_end(struct mddev *mddev);
extern void md_done_sync(struct mddev *mddev, int blocks, int ok);
extern void md_error(struct mddev *mddev, struct md_rdev *rdev);

extern int mddev_congested(struct mddev *mddev, int bits);
extern void md_flush_request(struct mddev *mddev, struct bio *bio);
extern void md_super_write(struct mddev *mddev, struct md_rdev *rdev,
			   sector_t sector, int size, struct page *page);
extern void md_super_wait(struct mddev *mddev);
extern int sync_page_io(struct md_rdev *rdev, sector_t sector, int size, 
			struct page *page, int rw, bool metadata_op);
extern void md_do_sync(struct mddev *mddev);
extern void md_new_event(struct mddev *mddev);
extern int md_allow_write(struct mddev *mddev);
extern void md_wait_for_blocked_rdev(struct md_rdev *rdev, struct mddev *mddev);
extern void md_set_array_sectors(struct mddev *mddev, sector_t array_sectors);
extern int md_check_no_bitmap(struct mddev *mddev);
extern int md_integrity_register(struct mddev *mddev);
extern void md_integrity_add_rdev(struct md_rdev *rdev, struct mddev *mddev);
extern int strict_strtoul_scaled(const char *cp, unsigned long *res, int scale);
extern void restore_bitmap_write_access(struct file *file);

#ifdef MY_ABC_HERE
void SynoAutoRemapReport(struct mddev *mddev, sector_t sector, struct block_device *bdev);
#endif
#ifdef MY_ABC_HERE
void RaidRemapModeSet(struct block_device *, unsigned char);

static inline void
RaidMemberAutoRemapSet(struct mddev *mddev)
{
	struct md_rdev *rdev, *tmp;
	char b[BDEVNAME_SIZE];

	/* enum all rdev and set the bdev */
	rdev_for_each(rdev, tmp, mddev) {
		bdevname(rdev->bdev,b);
		RaidRemapModeSet(rdev->bdev, mddev->auto_remap);
		printk("md: %s: set %s to auto_remap [%d]\n", mdname(mddev), b, mddev->auto_remap);
	}
}
#endif

#ifdef MY_ABC_HERE
void SYNORaidRdevUnplug(struct mddev *mddev, struct md_rdev *rdev);
#endif
extern void mddev_init(struct mddev *mddev);
extern int md_run(struct mddev *mddev);
extern void md_stop(struct mddev *mddev);
extern void md_stop_writes(struct mddev *mddev);
extern int md_rdev_init(struct md_rdev *rdev);

extern void mddev_suspend(struct mddev *mddev);
extern void mddev_resume(struct mddev *mddev);
extern struct bio *bio_clone_mddev(struct bio *bio, gfp_t gfp_mask,
				   struct mddev *mddev);
extern struct bio *bio_alloc_mddev(gfp_t gfp_mask, int nr_iovecs,
				   struct mddev *mddev);
extern int mddev_check_plugged(struct mddev *mddev);
extern void md_trim_bio(struct bio *bio, int offset, int size);
#endif /* _MD_MD_H */
