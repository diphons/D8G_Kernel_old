#
# Makefile for the linux ext4-filesystem routines.
#
ccflags-y += -Ifs/crypto

obj-$(CONFIG_EXT4_FS) += ext4.o

ext4-y	:= balloc.o bitmap.o dir.o file.o fsync.o ialloc.o inode.o page-io.o \
		ioctl.o namei.o super.o symlink.o hash.o resize.o extents.o \
		ext4_jbd2.o migrate.o mballoc.o block_validity.o move_extent.o \
		mmp.o indirect.o extents_status.o xattr.o xattr_user.o \
		xattr_trusted.o inline.o readpage.o sysfs.o

ext4-$(CONFIG_EXT4_FS_POSIX_ACL)	+= acl.o
ext4-$(CONFIG_EXT4_FS_SECURITY)		+= xattr_security.o
ext4-$(CONFIG_FS_VERITY)		+= verity.o
