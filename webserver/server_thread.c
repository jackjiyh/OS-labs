#include "request.h"
#include "server_thread.h"
#include "common.h"
#include <pthread.h>


static void * process_request(void *argv);

struct request_buffer {
    int * buffer;
    int begin;
    int end;
};


struct server {
	int nr_threads;
	int max_requests;
	int max_cache_size;
	/* add any other parameters you need */
        struct request_buffer request_buffer;
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

static void
do_server_request(struct server *sv, int connfd)
{
	int ret;
	struct request *rq;
	struct file_data *data;

	data = file_data_init();

	/* fills data->file_name with name of the file being requested */
	rq = request_init(connfd, data);
	if (!rq) {
		file_data_free(data);
		return;
	}
	/* reads file, 
	 * fills data->file_buf with the file contents,
	 * data->file_size with file size. */
	ret = request_readfile(rq);
	if (!ret)
		goto out;
	/* sends file to client */
	request_sendfile(rq);
out:
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
        do_server_request(sv, connfd);
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
		do_server_request(sv, connfd);
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
