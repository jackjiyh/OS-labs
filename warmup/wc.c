#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <math.h>
#include "common.h"
#include "wc.h"
#define M 5000000

struct entry;
void word_copy(char * source, long begin, long len, struct entry * dest);

long hash_func(char * word, long begin, long len);
int insert_word(struct wc * wc, char * word_array, long hashValue, long begin, long len);
int check_equal(struct entry * entry, char * word_array, long begin, long len);

struct entry {
    char * word;
    long len;
    int count;
    //struct entry * next;
};

long hash_func(char * word, long begin, long len) {
    long value = 5381;
    int c;
    long i;
    for (i=begin; i < begin+len; i++) {
        //value += word[i] - '0' + 2;
        c = word[i];
        value = ( (value << 5) + value ) ^ c;
    }
    //return ((value/2000)*M) % M;
    return abs(value % M);
}


struct wc {
	/* you can define this struct to have whatever fields you want. */
        struct entry * wordCount[M];
};

void word_copy(char * source, long begin, long len, struct entry * dest) {
    dest->len = len;
    dest->word = (char *)malloc(sizeof(char)*(len+1));
    long i;
    for (i=begin; i<begin+len;i++) {
        dest->word[i-begin] = source[i];
    }
    dest->word[len] = 0;
    //dest->next = NULL;
    dest->count = 1;
}

int check_equal(struct entry * entry, char * word_array, long begin, long len) {
    if (entry->len != len) return 0;
    long i;
    for ( i=0;i<len;i++) {
        if ( entry->word[i] != word_array[begin+i]) return 0;
    }
    return 1;
}

int insert_word(struct wc * wc, char * word_array, long hashValue, long begin, long len) {
    if ( wc->wordCount[hashValue] == NULL) {
        wc->wordCount[hashValue] = (struct entry *)malloc(sizeof(struct entry));
        //copy word from word_array to entry
        word_copy(word_array, begin, len, wc->wordCount[hashValue]);
        return 1;
    } else if ( check_equal(wc->wordCount[hashValue], word_array, begin, len) ) {
        wc->wordCount[hashValue]->count++;
        return 1;
    } else return 0;

    /*
    struct entry * curEn = wc->wordCount[hashValue];
    struct entry * preEn = curEn;
    int state = 0;
    while (curEn != NULL) {
        if ( check_equal(curEn, word_array, begin, len) ) {
            curEn->count++;
            return;
        } else {
            preEn = curEn;
            curEn = curEn->next;
            state = 1;
        }
    }
    struct entry * newEn = (struct entry *)malloc(sizeof(struct entry));
    word_copy(word_array, begin, len, newEn);
    if (state == 1) preEn->next = newEn;
    else preEn = newEn;
    preEn->next->word = (char *)malloc(sizeof(char)*len);
    preEn->next->len = len;
    long i;
    for (i=begin; i<begin+len; i++) {
        preEn->next->word[i-begin] = word_array[i];       
    }
    preEn->next->count = 1;
    preEn->next->next = NULL;

     */ 
}

struct wc *
wc_init(char *word_array, long size)
{
	struct wc *wc;
	long i;

	wc = (struct wc *)malloc(sizeof(struct wc));
	assert(wc);
	for (i=0; i< M; i++) {
	    wc->wordCount[i] = NULL;
	}

	//TBD();
        long begin = 0, len = 0;
        long coll = 0;
        for (i=0; i<size; i++) {
            if ( !isspace(word_array[i]) ) {
                len++;
            } else {
                // hit a word, hash into the hash table
                //if (word_array[i] == '') continue;
                if (i==begin) {
                    begin++;
                    continue;
                }
                long hashValue = hash_func(word_array, begin, len);
                //insert_word(wc, word_array, hashValue, begin, len);
                while ( !insert_word(wc, word_array, hashValue, begin, len) ) {
                    if (hashValue < M -1) hashValue++;
                    else hashValue = 0;
                    coll++;
                }
                begin = begin + len + 1;
                len = 0;
            }
        }
        //printf("The # of collision: %ld\n", coll);
	return wc;
}

void
wc_output(struct wc *wc)
{
	//TBD();
	long i;
	long size = 0;
	for (i=0; i<M; i++) {
	    if (wc->wordCount[i] != NULL) {
	        //struct entry * curEn = wc->wordCount[i];
	        //while (curEn != NULL) {
	        printf("%s:%d\n", wc->wordCount[i]->word, wc->wordCount[i]->count);
	            //curEn = curEn->next;
	        size += wc->wordCount[i]->count;
	        //}
	    }
	}
	//printf("The total # of words is %ld.\n", size);
}

void
wc_destroy(struct wc *wc)
{
	//TBD();
	long i;
	for (i=0; i<M; i++) {
	    if (wc->wordCount[i] != NULL) {
	        /*struct entry * curEn = wc->wordCount[i];
	        while (curEn != NULL) {
	            struct entry * temp = curEn;
	            curEn = curEn->next;
	            wc->wordCount[i] = curEn;
	            free(temp->word);
	            free(temp);
	        }*/
                free(wc->wordCount[i]->word);
                free(wc->wordCount[i]);
	    }
	}
	free(wc);
}
