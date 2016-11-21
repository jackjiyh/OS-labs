#include "request.h"
#include "server_thread.h"
#include "common.h"
#include <pthread.h>
#include <math.h>


static void * process_request(void *argv);
int cache_lookup(struct server * sv, char * file_name);
int hash_func(char * file_name, int size);
int cache_insert(struct server *sv, struct file_data *data);
int cache_evict(struct server *sv); 
static void do_server_request_no_cache(struct server *sv, int connfd);

struct request_buffer {
    int * buffer;
    int begin;
    int end;
};

struct hash_entry {
    char * file_name;
    char * file_buf;
    int evicted;
    int rc;
    int file_size;
    //pthread_mutex_t lock_h;
};

struct LF_entry {
    char * file_name;
    int file_size;
    int hash_value;
    struct LF_entry *next;
};


void LF_list_insert(struct server *sv, struct file_data *data, int hash_value); 
struct server {
	int nr_threads;
	int max_requests;
	int max_cache_size;
	/* add any other parameters you need */
        struct request_buffer request_buffer;
        struct hash_entry ** cache;
        int cache_size;
        int cache_left;
        struct LF_entry * LF_list; // a link list sorted in terms of file_size
        pthread_t * threads;
};

/* static functions */

/* initialize file data */
static struct file_data *
file_data_init(void)
{
	struct file_data *data;

	data = Malloc(sizeof(struct file_data));
	data->file_name = NULL;
	data->file_buf = NULL;
	data->file_size = 0;
	return data;
}

/* free all file data */
static void
file_data_free(struct file_data *data)
{
	free(data->file_name);
	free(data->file_buf);
	free(data);
}

int hash_func(char * file_name, int size) {
    int value = 5381;
    int c, i, len = strlen(file_name);

    for (i=0; i<len; i++) {
        c = file_name[i];
        value = ( (value << 5) + value ) ^ c;
    }

    return abs(value % size);
}

pthread_mutex_t cache_lock = PTHREAD_MUTEX_INITIALIZER;

/* search the cache to find the file
 * if found, return the hash_value of the file
 * if not found, return -1 */
int
cache_lookup(struct server * sv, char * file_name) {
    int size = sv->cache_size;
    int hash_value = hash_func(file_name, size);
    struct hash_entry ** cache = sv->cache;
    while (cache[hash_value]->file_name != NULL || cache[hash_value]->evicted != 0) {
        if (cache[hash_value]->evicted == 1) {
            hash_value = (hash_value + 1) % size;
            continue;
        }
        
        if (strcmp(cache[hash_value]->file_name, file_name) != 0) {
            hash_value = (hash_value + 1) % size;
        } else {
            cache[hash_value]->rc += 1;
            return hash_value;
        }
    }
    return -1;
    
}

/* insert the new cache entry to LF_list
 * nodes are inserted in descending orders in terms of file_size */
void LF_list_insert(struct server *sv, struct file_data *data, int hash_value) {
    struct LF_entry * newEn = Malloc(sizeof(struct LF_entry));
    int len = strlen(data->file_name);
   
    newEn->file_name = Malloc(len+1);
    strncpy(newEn->file_name, data->file_name, len);
    newEn->file_name[len] = '\0';

    newEn->file_size = data->file_size;
    newEn->hash_value = hash_value;
    newEn->next = NULL;
    
    if (sv->LF_list == NULL) {
        sv->LF_list = newEn;
    } else {
        struct LF_entry *curEn = sv->LF_list;
        struct LF_entry *preEn = NULL;
        while (curEn != NULL) {
            if ( newEn->file_size >= curEn->file_size) {
                newEn->next = curEn;
                if (preEn == NULL) sv->LF_list = newEn;
                else preEn->next = newEn;
                return;
            } else {
                if (curEn->next == NULL) {
                    curEn->next = newEn;
                    return;
                } else {
                    preEn = curEn;
                    curEn = curEn->next;
                }
            }
        }
    }
    assert(sv->LF_list);
}

/* this function will remove node with largest file_size
 * and that node needs to be removeable, i.e. rc == 0*/
struct LF_entry *
LF_list_remove(struct server *sv) {
    assert(sv->LF_list != NULL);
    struct LF_entry *curEn = sv->LF_list;
    struct LF_entry *preEn = NULL;
    while (curEn != NULL) {
        if (sv->cache[curEn->hash_value]->rc == 0) {
            if (preEn == NULL) {
                sv->LF_list = curEn->next;
                curEn->next = NULL;
            } else {
                preEn->next = curEn->next;
                curEn->next = NULL;
            }
            return curEn;
        } else {
            preEn = curEn;
            curEn = curEn->next;
        }
    }
    return NULL;
}

/* insert the new file into the cache
 * update the LF_list for eviction
 * return hash_value, the location in cache 
 * that the file is inserted to */
int
cache_insert(struct server *sv, struct file_data *data) {
    struct hash_entry ** cache = sv->cache;
    int hash_value = hash_func(data->file_name, sv->cache_size);
    while (cache[hash_value]->file_name != NULL) {
        hash_value = (hash_value + 1) % sv->cache_size;
    }
    
    int len1 = strlen(data->file_name);
    int len2 = data->file_size;
    
    cache[hash_value]->file_name = Malloc(len1+1);
    strncpy(cache[hash_value]->file_name, data->file_name, len1);
    cache[hash_value]->file_name[len1] = '\0';

    cache[hash_value]->file_buf = Malloc(len2+1);
    strncpy(cache[hash_value]->file_buf, data->file_buf, len2);
    cache[hash_value]->file_buf[len2] = '\0';
    
    //printf("%s diff: %d, len2:%d\n", data->file_name, strcmp(data->file_buf, cache[hash_value]->file_buf), len2);
    /*int i;
    for (i=0;i<len2+1;i++) {
        if (data->file_buf[i] != cache[hash_value]->file_buf[i]){
            printf("%d\n", i);
        }
    }*/
    
    cache[hash_value]->rc = 1;
    cache[hash_value]->evicted = 0;
    cache[hash_value]->file_size = data->file_size;
    sv->cache_left -= data->file_size;

    LF_list_insert(sv, data, hash_value);
    return hash_value;
}

/* the eviction algorithm
 * will evict the largest evictable file 
 * return 1 upon success
 * return 0 if there is no evictable file*/
int
cache_evict(struct server *sv) {
    struct LF_entry *removed = LF_list_remove(sv);
    if (removed == NULL) return 0;
    int hash_value = removed->hash_value;
    assert(hash_value >= 0);
    
    struct hash_entry *entry = sv->cache[hash_value];
    assert(entry->rc == 0);
    assert(!strcmp(entry->file_name, removed->file_name));
    assert(entry->file_size == removed->file_size);
    
    free(entry->file_name);
    entry->file_name = NULL;
    
    free(entry->file_buf);
    entry->file_buf = NULL;
    
    entry->evicted = 1;
    entry->file_size = 0;

    sv->cache_left += removed->file_size;
    
    free(removed->file_name);
    free(removed);

    return 1;
}


/* cache miss handler
 * return hash_value upon success
 * return -1 when failure */
int 
cache_miss_handler(struct server *sv, struct request *rq, struct file_data *data) {
    int ret;
    int hash_value;

    ret = request_readfile(rq);
    if (!ret) return -1;
    pthread_mutex_lock(&cache_lock);
    hash_value = cache_lookup(sv, data->file_name);
    if (data->file_size <= sv->cache_left) {
        if (hash_value == -1) {
            // insert the file and update LF_list
            hash_value = cache_insert(sv, data);
        }
    } else {
        if (hash_value == -1) {
            if (data->file_size >= 0.5*(sv->max_cache_size)){
                pthread_mutex_unlock(&cache_lock);
                return -2;
            }
            //evict
            while (data->file_size > sv->cache_left) {
                ret = cache_evict(sv);
                if (!ret) {
                    pthread_mutex_unlock(&cache_lock);
                    return -2;
                }
            }
            //insert
            hash_value = cache_insert(sv, data);     
            //printf("eviction at %s: %d\n", data->file_name, hash_value);
        }
    }
    pthread_mutex_unlock(&cache_lock);
    return hash_value;
}

static void
do_server_request(struct server *sv, int connfd)
{
	//int ret;
	struct request *rq;
	struct file_data *data;

	data = file_data_init();

	/* fills data->file_name with name of the file being requested */
	rq = request_init(connfd, data);
	if (!rq) {
		file_data_free(data);
		return;
	}

	/* first, find file in the cache
	 * if miss, call miss handler
	 * if success, send the file. */
        pthread_mutex_lock(&cache_lock);
        int hash_value = cache_lookup(sv, data->file_name);
        //printf("%s, %d\n",data->file_name, hash_value);
        pthread_mutex_unlock(&cache_lock);
        if (hash_value == -1) {
            hash_value = cache_miss_handler(sv, rq, data);
            if (hash_value == -1)
                goto out;
            if (hash_value == -2) {
                request_sendfile(rq);
                goto out;
            }
        } else {
            int len = sv->cache[hash_value]->file_size;
            data->file_buf = Malloc(len+1);
            strncpy(data->file_buf, sv->cache[hash_value]->file_buf, len);
            data->file_buf[len] = '\0';
            data->file_size = len;
            //request_readfile(rq);
            //printf("COPY: %d\n", strcmp(data->file_buf, sv->cache[hash_value]->file_buf));
          
            
            // need to update LRU_list later
        }
    
	/* sends file to client */
	request_sendfile(rq);
        sv->cache[hash_value]->rc -= 1;
out:
	request_destroy(rq);
	file_data_free(data);
}


static void do_server_request_no_cache(struct server *sv, int connfd) {
    int ret;
    struct request *rq;
    struct file_data *data;

    data = file_data_init();

    rq = request_init(connfd, data);
    if (!rq) {
        file_data_free(data);
        return;
    }

    ret = request_readfile(rq);
    if (!ret)
        goto out1;
    request_sendfile(rq);
out1: 
    request_destroy(rq);
    file_data_free(data);
}

/* entry point functions */
pthread_cond_t empty = PTHREAD_COND_INITIALIZER;
pthread_cond_t full = PTHREAD_COND_INITIALIZER;
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

static void * process_request(void *argv) {
    
    while (1) {
        pthread_mutex_lock(&lock);
        struct server * sv = argv;
        while (sv->request_buffer.begin == sv->request_buffer.end) { // empty
            pthread_cond_wait(&empty, &lock);
        }
        int connfd = sv->request_buffer.buffer[sv->request_buffer.begin];
        if ( (sv->request_buffer.end + 1) % (sv->max_requests + 1) == sv->request_buffer.begin ) {
            pthread_cond_signal(&full);
        }
        sv->request_buffer.begin = (sv->request_buffer.begin + 1) % (sv->max_requests + 1);
        pthread_mutex_unlock(&lock);
        if (sv->max_cache_size > 0) do_server_request(sv, connfd);
        else {
            do_server_request_no_cache(sv, connfd);
        }
    }
    return NULL;
}


struct server *
server_init(int nr_threads, int max_requests, int max_cache_size)
{
	struct server *sv;
        int i;
	sv = Malloc(sizeof(struct server));
	sv->nr_threads = nr_threads;
	sv->max_requests = max_requests;
	sv->max_cache_size = max_cache_size;

	if (nr_threads > 0 || max_requests > 0 || max_cache_size > 0) {
	
	    if (max_requests > 0) {
	        sv->request_buffer.buffer = (int *)malloc((max_requests+1)*sizeof(int));
	        sv->request_buffer.begin = 0;
	        sv->request_buffer.end = 0;
	    }
	    
	    if (nr_threads > 0) {
	        sv->threads = (pthread_t *)malloc((nr_threads)*sizeof(pthread_t));
                
                for (i=0; i<nr_threads; i++) {
                    pthread_create(&sv->threads[i], NULL, &process_request, sv);
                }
	    }

	    if (max_cache_size > 0) {
	        sv->cache_size = 10000;
	        sv->cache = Malloc(sizeof(struct hash_entry *)*(sv->cache_size));
	        sv->cache_left = max_cache_size;
	        for (i=0; i < sv->cache_size; i++) {
	            sv->cache[i] = Malloc(sizeof(struct hash_entry));
	            sv->cache[i]->file_name = NULL;
	            sv->cache[i]->file_buf = NULL;
	            sv->cache[i]->evicted = 0;
	            sv->cache[i]->rc = 0;
	            sv->cache[i]->file_size = 0;
	        }
	        // need to initialize LF_list later
	        sv->LF_list = NULL;
	        //printf("Done initializing server\n");
	    }
	}

	/* Lab 4: create queue of max_request size when max_requests > 0 */

	/* Lab 5: init server cache and limit its size to max_cache_size */

	/* Lab 4: create worker threads when nr_threads > 0 */

	return sv;
}

void
server_request(struct server *sv, int connfd)
{
	if (sv->nr_threads == 0) { /* no worker threads */
		do_server_request_no_cache(sv, connfd);
	} else {
		/*  Save the relevant info in a buffer and have one of the
		 *  worker threads do the work. */
		//TBD();
		pthread_mutex_lock(&lock);
		while ((sv->request_buffer.end + 1) % (sv->max_requests + 1) == sv->request_buffer.begin) {
		    pthread_cond_wait(&full, &lock);
		}
		sv->request_buffer.buffer[sv->request_buffer.end] = connfd;
		if ( sv->request_buffer.begin == sv->request_buffer.end ) {
		    pthread_cond_signal(&empty);
		}
		sv->request_buffer.end = (sv->request_buffer.end + 1) % (sv->max_requests + 1);
		pthread_mutex_unlock(&lock);
	}
}
