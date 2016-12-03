#include "testfs.h"
#include "list.h"
#include "super.h"
#include "block.h"
#include "inode.h"

/* given logical block number, read the corresponding physical block into block.
 * return physical block number.
 * returns 0 if physical block does not exist.
 * returns negative value on other errors. */
    static int
testfs_read_block(struct inode *in, int log_block_nr, char *block)
{
    int phy_block_nr = 0;

    assert(log_block_nr >= 0);
    if (log_block_nr < NR_DIRECT_BLOCKS) {
        phy_block_nr = (int)in->in.i_block_nr[log_block_nr];
    } else {
        log_block_nr -= NR_DIRECT_BLOCKS;

        if (log_block_nr < NR_INDIRECT_BLOCKS) {
            // need to implement 
            //TBD();
            if (in->in.i_indirect > 0) {
                read_blocks(in->sb, block, in->in.i_indirect, 1);
                phy_block_nr = ((int *)block)[log_block_nr];
            }

        } else {
            log_block_nr -= NR_INDIRECT_BLOCKS;
            long dind_nr = log_block_nr / NR_INDIRECT_BLOCKS;
            long ind_nr = log_block_nr % NR_INDIRECT_BLOCKS;
            if (dind_nr + 1 > NR_INDIRECT_BLOCKS) 
                return -EFBIG;

            if (in->in.i_dindirect > 0) {
                read_blocks(in->sb, block, in->in.i_dindirect, 1);
                phy_block_nr = ((int *)block)[dind_nr];
                if (phy_block_nr > 0) {
                    read_blocks(in->sb, block, phy_block_nr, 1);
                    phy_block_nr = ((int *)block)[ind_nr];
                }
            }
        }
    }
    if (phy_block_nr > 0) {
        read_blocks(in->sb, block, phy_block_nr, 1);
    } else {
        /* we support sparse files by zeroing out a block that is not
         * allocated on disk. */
        bzero(block, BLOCK_SIZE);
    }
    return phy_block_nr;
}

    int
testfs_read_data(struct inode *in, char *buf, off_t start, size_t size)
{
    char block[BLOCK_SIZE];
    long block_nr = start / BLOCK_SIZE;
    long block_ix = start % BLOCK_SIZE;
    int ret, num_blocks = 1, temp_size = 0, i, buf_offset = 0;

    assert(buf);
    if (start + (off_t) size > in->in.i_size) {
        size = in->in.i_size - start;
    }
    if (block_ix + size > BLOCK_SIZE) {
        //TBD();
        num_blocks = (block_ix + size + BLOCK_SIZE - 1) / BLOCK_SIZE;
    }
    for (i=0; i<num_blocks; i++) {
        if ((ret = testfs_read_block(in, block_nr + i, block)) < 0)
            return ret;

        if (i==0) {
            if (num_blocks > 1) temp_size = BLOCK_SIZE - block_ix;
            else temp_size = size;
        } else if (i == num_blocks - 1) {
            temp_size = (start + size) % BLOCK_SIZE;
        } else {
            temp_size = BLOCK_SIZE;
        }

        memcpy(buf + buf_offset, block + block_ix, temp_size);
        block_ix = 0;
        buf_offset += temp_size;
    }
    /* return the number of bytes read or any error */
    assert(((size_t)buf_offset) == size);
    return size;
}

/* given logical block number, allocate a new physical block, if it does not
 * exist already, and return the physical block number that is allocated.
 * returns negative value on error. */
    static int
testfs_allocate_block(struct inode *in, int log_block_nr, char *block)
{
    int phy_block_nr, flag = 0;
    char indirect[BLOCK_SIZE];
    int indirect_allocated = 0;

    char dind[BLOCK_SIZE];
    int dind_allocated = 0, dind_ind_alloc = 0;
    long dind_nr, ind_nr;

    assert(log_block_nr >= 0);
    phy_block_nr = testfs_read_block(in, log_block_nr, block);

    /* phy_block_nr > 0: block exists, so we don't need to allocate it, 
       phy_block_nr < 0: some error */
    if (phy_block_nr != 0)
        return phy_block_nr;

    /* allocate a direct block */
    if (log_block_nr < NR_DIRECT_BLOCKS) {
        assert(in->in.i_block_nr[log_block_nr] == 0);
        phy_block_nr = testfs_alloc_block_for_inode(in);
        if (phy_block_nr >= 0) {
            in->in.i_block_nr[log_block_nr] = phy_block_nr;
        }
        return phy_block_nr;
    }

    log_block_nr -= NR_DIRECT_BLOCKS;

    if (log_block_nr < NR_INDIRECT_BLOCKS) {
        //TBD();
        if (in->in.i_indirect == 0) {	/* allocate an indirect block */
            bzero(indirect, BLOCK_SIZE);
            phy_block_nr = testfs_alloc_block_for_inode(in);
            if (phy_block_nr < 0)
                return phy_block_nr;
            indirect_allocated = 1;
            in->in.i_indirect = phy_block_nr;
        } else {	/* read indirect block */
            read_blocks(in->sb, indirect, in->in.i_indirect, 1);
        }

        /* allocate direct block */
        assert(((int *)indirect)[log_block_nr] == 0);	
        phy_block_nr = testfs_alloc_block_for_inode(in);
    } else {
        flag = 1;
        log_block_nr -= NR_INDIRECT_BLOCKS;
        dind_nr = log_block_nr / NR_INDIRECT_BLOCKS;
        ind_nr = log_block_nr % NR_INDIRECT_BLOCKS;

        if (dind_nr + 1 > NR_INDIRECT_BLOCKS)
            return -EFBIG;

        if (in->in.i_dindirect == 0) {
            bzero(dind, BLOCK_SIZE);
            phy_block_nr = testfs_alloc_block_for_inode(in);
            if (phy_block_nr < 0)
                return phy_block_nr;
            dind_allocated = 1;
            in->in.i_dindirect = phy_block_nr;
        } else {
            read_blocks(in->sb, dind, in->in.i_dindirect, 1);
        }
        if (((int *)dind)[dind_nr] == 0) {
            bzero(indirect, BLOCK_SIZE);
            phy_block_nr = testfs_alloc_block_for_inode(in);
            if (phy_block_nr < 0)
                return phy_block_nr;
            dind_ind_alloc = 1;
            ((int *)dind)[dind_nr] = phy_block_nr;
        } else {
            read_blocks(in->sb, indirect, ((int *)dind)[dind_nr], 1);
        }
        /* allocate direct block */
        assert(((int *)indirect)[ind_nr] == 0);
        phy_block_nr = testfs_alloc_block_for_inode(in);

        log_block_nr = ind_nr; 
    }

    if (phy_block_nr >= 0) {
        /* update indirect block */
        ((int *)indirect)[log_block_nr] = phy_block_nr;
        if (!flag) {
            write_blocks(in->sb, indirect, in->in.i_indirect, 1);
        } else {
            write_blocks(in->sb, dind, in->in.i_dindirect, 1);
            write_blocks(in->sb, indirect, ((int *)dind)[dind_nr], 1);
        }
    } else {

        if (indirect_allocated) {
            /* free the indirect block that was allocated */
            testfs_free_block_from_inode(in, in->in.i_indirect);
            in->in.i_indirect = 0;
        }

        if (dind_ind_alloc) {
            testfs_free_block_from_inode(in, ((int *)dind)[dind_nr]);
            ((int *)dind)[dind_nr] = 0;
        }

        if (dind_allocated) {
            testfs_free_block_from_inode(in, in->in.i_dindirect);
            in->in.i_dindirect = 0;
        } 
    }
    return phy_block_nr;
}

    int
testfs_write_data(struct inode *in, const char *buf, off_t start, size_t size)
{
    char block[BLOCK_SIZE];
    long block_nr = start / BLOCK_SIZE;
    long block_ix = start % BLOCK_SIZE;
    int ret, num_blocks = 1, i, buf_offset = 0, temp_size = 0;

    if (block_ix + size > BLOCK_SIZE) {
        //TBD();
        num_blocks = (block_ix + size + BLOCK_SIZE - 1) / BLOCK_SIZE;
    }
    /* ret is the newly allocated physical block number */
    for (i=0; i<num_blocks; i++) {
        ret = testfs_allocate_block(in, block_nr + i, block);
        if (ret < 0) {
            if ( buf_offset > 0 && ret != -ENOSPC) {
                in->in.i_size = MAX(in->in.i_size, start + (off_t)buf_offset);
                in->i_flags |= I_FLAGS_DIRTY;
            }
            return ret;
        }

        if (i==0) {
            if (num_blocks > 1) temp_size = BLOCK_SIZE - block_ix;
            else temp_size = size;
        } else if (i == num_blocks - 1) {
            temp_size = (start + size) % BLOCK_SIZE;
        } else {
            temp_size = BLOCK_SIZE;
        }

        memcpy(block + block_ix, buf + buf_offset, temp_size);
        write_blocks(in->sb, block, ret, 1);
        buf_offset += temp_size;
        block_ix = 0;
    }

    /* increment i_size by the number of bytes written. */
    if (size > 0)
        in->in.i_size = MAX(in->in.i_size, start + (off_t) size);
    in->i_flags |= I_FLAGS_DIRTY;
    /* return the number of bytes written or any error */
    return size;
}

    int
testfs_free_blocks(struct inode *in)
{
    int i, j;
    int e_block_nr;

    /* last block number */
    e_block_nr = DIVROUNDUP(in->in.i_size, BLOCK_SIZE);

    /* remove direct blocks */
    for (i = 0; i < e_block_nr && i < NR_DIRECT_BLOCKS; i++) {
        if (in->in.i_block_nr[i] == 0)
            continue;
        testfs_free_block_from_inode(in, in->in.i_block_nr[i]);
        in->in.i_block_nr[i] = 0;
    }
    e_block_nr -= NR_DIRECT_BLOCKS; 


    /* remove indirect blocks */
    if (in->in.i_indirect > 0) {
        char block[BLOCK_SIZE];
        read_blocks(in->sb, block, in->in.i_indirect, 1);
        for (i = 0; i < e_block_nr && i < NR_INDIRECT_BLOCKS; i++) {
            if ( ((int *)block)[i] == 0 )
                continue;
            testfs_free_block_from_inode(in, ((int *)block)[i]);
            ((int *)block)[i] = 0;
        }
        testfs_free_block_from_inode(in, in->in.i_indirect);
        in->in.i_indirect = 0;
    }

    e_block_nr -= NR_INDIRECT_BLOCKS;
    if (e_block_nr > 0) {
        //TBD();
        long dind_nr = e_block_nr / NR_INDIRECT_BLOCKS; 
        long ind_nr = e_block_nr % NR_INDIRECT_BLOCKS;
        char dind_block[BLOCK_SIZE];
        char block[BLOCK_SIZE]; 
        assert(in->in.i_dindirect != 0);

        //printf("before dind block_count = %d\n", in->in.block_count);
        read_blocks(in->sb, dind_block, in->in.i_dindirect, 1);
        for (i=0; i<dind_nr+1; i++) {
            if ( ((int *)dind_block)[i] == 0 ) 
                continue;
            read_blocks(in->sb, block, ((int *)dind_block)[i], 1);
            long temp = (i == dind_nr) ? (ind_nr+1) : NR_INDIRECT_BLOCKS;
            for (j=0; j < temp; j++) {
                if ( ((int *)block)[j] == 0)
                    continue;
                testfs_free_block_from_inode(in, ((int *)block)[j]);
                ((int *)block)[j] = 0;
            }
            testfs_free_block_from_inode(in, ((int *)dind_block)[i]);
            ((int *)dind_block)[i] = 0;
        }
        testfs_free_block_from_inode(in, in->in.i_dindirect);
        in->in.i_indirect = 0;
    }

    in->in.i_size = 0;
    in->i_flags |= I_FLAGS_DIRTY;
    return 0;
}

