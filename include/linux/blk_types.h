/*
 * Block data types and constants.  Directly include this file only to
 * break include dependency loop.
 */
#ifndef __LINUX_BLK_TYPES_H
#define __LINUX_BLK_TYPES_H

#include <linux/types.h>
#include <linux/bvec.h>

struct bio_set;
struct bio;
struct bio_integrity_payload;
struct page;
struct block_device;
struct io_context;
struct cgroup_subsys_state;
typedef void (bio_end_io_t) (struct bio *);

/*
 * Block error status values.  See block/blk-core:blk_errors for the details.
 */
typedef u8 __bitwise blk_status_t;
#define	BLK_STS_OK 0
#define BLK_STS_NOTSUPP		((__force blk_status_t)1)
#define BLK_STS_TIMEOUT		((__force blk_status_t)2)
#define BLK_STS_NOSPC		((__force blk_status_t)3)
#define BLK_STS_TRANSPORT	((__force blk_status_t)4)
#define BLK_STS_TARGET		((__force blk_status_t)5)
#define BLK_STS_NEXUS		((__force blk_status_t)6)
#define BLK_STS_MEDIUM		((__force blk_status_t)7)
#define BLK_STS_PROTECTION	((__force blk_status_t)8)
#define BLK_STS_RESOURCE	((__force blk_status_t)9)
#define BLK_STS_IOERR		((__force blk_status_t)10)

/* hack for device mapper, don't use elsewhere: */
#define BLK_STS_DM_REQUEUE    ((__force blk_status_t)11)

struct blk_issue_stat {
	u64 stat;
};

#ifdef CONFIG_BLOCK
/*
 * main unit of I/O for the block layer and lower layers (ie drivers and
 * stacking drivers)
 */
struct bio {
	struct bio		*bi_next;	/* request queue link */
	struct block_device	*bi_bdev;
	unsigned short		bi_write_hint;
	blk_status_t		bi_status;
	unsigned int		bi_opf;		/* bottom bits req flags,
						 * top bits REQ_OP. Use
						 * accessors.
						 */
	unsigned short		bi_flags;	/* status, command, etc */
	unsigned short		bi_ioprio;

	struct bvec_iter	bi_iter;

	/* Number of segments in this BIO after
	 * physical address coalescing is performed.
	 */
	unsigned int		bi_phys_segments;

	/*
	 * To keep track of the max segment size, we account for the
	 * sizes of the first and last mergeable segments in this bio.
	 */
	unsigned int		bi_seg_front_size;
	unsigned int		bi_seg_back_size;

	atomic_t		__bi_remaining;

	bio_end_io_t		*bi_end_io;

	void			*bi_private;
#ifdef CONFIG_BLK_CGROUP
	/*
	 * Optional ioc and css associated with this bio.  Put on bio
	 * release.  Read comment on top of bio_associate_current().
	 */
	struct io_context	*bi_ioc;
	struct cgroup_subsys_state *bi_css;
#ifdef CONFIG_BLK_DEV_THROTTLING_LOW
	void			*bi_cg_private;
	struct blk_issue_stat	bi_issue_stat;
#endif
#endif
	union {
#if defined(CONFIG_BLK_DEV_INTEGRITY)
		struct bio_integrity_payload *bi_integrity; /* data integrity */
#endif
	};

#ifdef CONFIG_PFK
	/* Encryption key to use (NULL if none) */
	const struct blk_encryption_key	*bi_crypt_key;

	/*
	* When using dircet-io (O_DIRECT), we can't get the inode from a bio
	* by walking bio->bi_io_vec->bv_page->mapping->host
	* since the page is anon.
	*/
	struct inode            *bi_dio_inode;
#endif
#ifdef CONFIG_DM_DEFAULT_KEY
	int bi_crypt_skip;
#endif

	unsigned short		bi_vcnt;	/* how many bio_vec's */

	/*
	 * Everything starting with bi_max_vecs will be preserved by bio_reset()
	 */

	unsigned short		bi_max_vecs;	/* max bvl_vecs we can hold */

	atomic_t		__bi_cnt;	/* pin count */

	struct bio_vec		*bi_io_vec;	/* the actual vec list */

	struct bio_set		*bi_pool;

	/*
	 * We can inline a number of vecs at the end of the bio, to avoid
	 * double allocations for a small number of bio_vecs. This member
	 * MUST obviously be kept at the very end of the bio.
	 */
	struct bio_vec		bi_inline_vecs[0];
};

#define BIO_RESET_BYTES		offsetof(struct bio, bi_max_vecs)
/*
struct blk_issue_stat {
	u64 stat;
};*/

/*
 * bio flags
 */
#define BIO_SEG_VALID	1	/* bi_phys_segments valid */
#define BIO_CLONED	2	/* doesn't own data */
#define BIO_BOUNCED	3	/* bio is a bounce bio */
#define BIO_USER_MAPPED 4	/* contains user pages */
#define BIO_NULL_MAPPED 5	/* contains invalid user pages */
#define BIO_WORKINGSET	6	/* contains userspace workingset pages */
#define BIO_QUIET	7	/* Make BIO Quiet */
#define BIO_CHAIN	8	/* chained bio, ->bi_remaining in effect */
#define BIO_REFFED	9	/* bio has elevated ->bi_cnt */
#define BIO_THROTTLED	10	/* This bio has already been subjected to
				 * throttling rules. Don't do it again. */

/*
 * Flags starting here get preserved by bio_reset() - this includes
 * BVEC_POOL_IDX()
 */
#define BIO_RESET_BITS	10


/*
 * Added for Req based dm which need to perform post processing. This flag
 * ensures blk_update_request does not free the bios or request, this is done
 * at the dm level
 */
#define BIO_DONTFREE	10
#define BIO_INLINECRYPT	11

/*
 * We support 6 different bvec pools, the last one is magic in that it
 * is backed by a mempool.
 */
#define BVEC_POOL_NR		6
#define BVEC_POOL_MAX		(BVEC_POOL_NR - 1)

/*
 * Top 4 bits of bio flags indicate the pool the bvecs came from.  We add
 * 1 to the actual index so that 0 indicates that there are no bvecs to be
 * freed.
 */
#define BVEC_POOL_BITS		(4)
#define BVEC_POOL_OFFSET	(16 - BVEC_POOL_BITS)
#define BVEC_POOL_IDX(bio)	((bio)->bi_flags >> BVEC_POOL_OFFSET)

#endif /* CONFIG_BLOCK */

/*
 * Operations and flags common to the bio and request structures.
 * We use 8 bits for encoding the operation, and the remaining 24 for flags.
 */
#define REQ_OP_BITS	8
#define REQ_OP_MASK	((1 << REQ_OP_BITS) - 1)
#define REQ_FLAG_BITS	24

enum req_opf {
	REQ_OP_READ,
	REQ_OP_WRITE,
	REQ_OP_DISCARD,		/* request to discard sectors */
	REQ_OP_SECURE_ERASE,	/* request to securely erase sectors */
	REQ_OP_WRITE_SAME,	/* write same block many times */
	REQ_OP_FLUSH,		/* request for cache flush */
	REQ_OP_ZONE_REPORT,	/* Get zone information */
	REQ_OP_ZONE_RESET,	/* Reset a zone write pointer */

	REQ_OP_LAST,
};

enum req_flag_bits {
	__REQ_FAILFAST_DEV =	/* no driver retries of device errors */
		REQ_OP_BITS,
	__REQ_FAILFAST_TRANSPORT, /* no driver retries of transport errors */
	__REQ_FAILFAST_DRIVER,	/* no driver retries of driver errors */
	__REQ_SYNC,		/* request is sync (sync write or read) */
	__REQ_META,		/* metadata io request */
	__REQ_PRIO,		/* boost priority in cfq */
	__REQ_NOMERGE,		/* don't touch this for merging */
	__REQ_NOIDLE,		/* don't anticipate more IO after this one */
	__REQ_INTEGRITY,	/* I/O includes block integrity payload */
	__REQ_FUA,		/* forced unit access */
	__REQ_PREFLUSH,		/* request for cache flush */
	__REQ_RAHEAD,		/* read ahead, can fail anytime */
	__REQ_BARRIER,		/* marks flush req as barrier */
        /* Android specific flags */
	__REQ_NOENCRYPT,	/* ok to not encrypt (already encrypted at fs
				   level) */
	__REQ_URGENT,		/* urgent request */
	__REQ_NR_BITS,		/* stops here */
};

#define REQ_FAILFAST_DEV	(1ULL << __REQ_FAILFAST_DEV)
#define REQ_FAILFAST_TRANSPORT	(1ULL << __REQ_FAILFAST_TRANSPORT)
#define REQ_FAILFAST_DRIVER	(1ULL << __REQ_FAILFAST_DRIVER)
#define REQ_SYNC		(1ULL << __REQ_SYNC)
#define REQ_META		(1ULL << __REQ_META)
#define REQ_PRIO		(1ULL << __REQ_PRIO)
#define REQ_NOMERGE		(1ULL << __REQ_NOMERGE)
#define REQ_URGENT		(1ULL << __REQ_URGENT)
#define REQ_NOIDLE		(1ULL << __REQ_NOIDLE)
#define REQ_INTEGRITY		(1ULL << __REQ_INTEGRITY)
#define REQ_NOENCRYPT		(1ULL << __REQ_NOENCRYPT)
#define REQ_BARRIER		(1ULL << __REQ_BARRIER)
#define REQ_FUA			(1ULL << __REQ_FUA)
#define REQ_PREFLUSH		(1ULL << __REQ_PREFLUSH)
#define REQ_RAHEAD		(1ULL << __REQ_RAHEAD)

#define REQ_FAILFAST_MASK \
	(REQ_FAILFAST_DEV | REQ_FAILFAST_TRANSPORT | REQ_FAILFAST_DRIVER)
#define REQ_COMMON_MASK \
	(REQ_FAILFAST_MASK | REQ_SYNC | REQ_META | REQ_PRIO | REQ_NOIDLE | \
	 REQ_PREFLUSH | REQ_FUA | REQ_INTEGRITY | REQ_NOMERGE | REQ_RAHEAD | REQ_BARRIER)
#define REQ_CLONE_MASK		REQ_COMMON_MASK

#define REQ_NOMERGE_FLAGS \
	(REQ_NOMERGE | REQ_PREFLUSH | REQ_FUA)

#define bio_op(bio) \
	((bio)->bi_opf & REQ_OP_MASK)
#define req_op(req) \
	((req)->cmd_flags & REQ_OP_MASK)

/* obsolete, don't use in new code */
#define bio_set_op_attrs(bio, op, op_flags) \
	((bio)->bi_opf |= (op | op_flags))

static inline bool op_is_sync(unsigned int op)
{
	return (op & REQ_OP_MASK) == REQ_OP_READ || (op & REQ_SYNC);
}

typedef unsigned int blk_qc_t;
#define BLK_QC_T_NONE	-1U
#define BLK_QC_T_SHIFT	16

static inline bool blk_qc_t_valid(blk_qc_t cookie)
{
	return cookie != BLK_QC_T_NONE;
}

static inline blk_qc_t blk_tag_to_qc_t(unsigned int tag, unsigned int queue_num)
{
	return tag | (queue_num << BLK_QC_T_SHIFT);
}

static inline unsigned int blk_qc_t_to_queue_num(blk_qc_t cookie)
{
	return cookie >> BLK_QC_T_SHIFT;
}

static inline unsigned int blk_qc_t_to_tag(blk_qc_t cookie)
{
	return cookie & ((1u << BLK_QC_T_SHIFT) - 1);
}

#endif /* __LINUX_BLK_TYPES_H */
