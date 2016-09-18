#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include "common.h"
#include "point.h"
#include "sorted_points.h"

struct sorted_points {
	/* you can define this struct to have whatever fields you want. */
    struct point * coor;
    struct sorted_points * next;
};

struct sorted_points *
sp_init()
{
	struct sorted_points *sp;

	sp = (struct sorted_points *)malloc(sizeof(struct sorted_points));
	assert(sp);

	//TBD();
        sp->coor = NULL;
        sp->next = NULL;
	return sp;
}

void
sp_destroy(struct sorted_points *sp)
{
	//TBD();
	struct point p;
	while (sp->next != NULL) {
	    sp_remove_first(sp, &p);
	}
	free(sp);
	return;
	//free(sp);
}

int
sp_add_point(struct sorted_points *sp, double x, double y)
{
	//TBD();
	struct sorted_points * newPt = sp_init();
	struct point * p = (struct point*)malloc(sizeof(struct point));
	p->x = x;
	p->y = y;
	newPt->coor = p;
	newPt->next = NULL;
	
        if (sp->next == NULL) {
            sp->next = sp_init();
            sp->next->coor = (struct point*)malloc(sizeof(struct point));
            sp->next->coor->x = x;
            sp->next->coor->y = y;
            sp->next->next = NULL;
            free(newPt->coor);
            free(newPt);
            return 1;
        } else {
            struct sorted_points *curPt = sp->next;
            struct sorted_points *prePt = NULL;
            while (curPt != NULL) {
                if ( point_compare(newPt->coor, curPt->coor) == -1 ) {
                    newPt->next = curPt;
                    if (prePt == NULL) {
                        sp->next = newPt;
                    } else {
                        prePt->next = newPt;
                    }
                    //break;
                    return 1;
                } else if ( point_compare(newPt->coor, curPt->coor) == 1 ) {
                    if (curPt->next == NULL) {
                        curPt->next = newPt;
                        //break;
                        return 1;
                    } else {
                        prePt = curPt;
                        curPt = curPt->next;
                    }
                } else {
                    if (curPt->coor->x > newPt->coor->x) {
                        newPt->next = curPt;
                        if (prePt == NULL) {
                            sp->next = newPt;
                        } else {
                            prePt->next = newPt;
                        }
                        //break;
                        return 1;
                    } else if (curPt->coor->x == newPt->coor->x) {
                        if (newPt->coor->y < curPt->coor->y) {
                            newPt->next = curPt;
                            if (prePt == NULL) {
                                sp->next = newPt;
                            } else {
                                prePt->next = newPt;
                            }
                            //break;
                            return 1;
                        } else {
                            newPt->next = curPt->next;
                            curPt->next = newPt;
                            //break;
                            return 1;
                        }
                    } else {
                        newPt->next = curPt->next;
                        curPt->next = newPt;
                        //break;
                        return 1;
                    }
                }

            }
        }
        return 0;       
}

int
sp_remove_first(struct sorted_points *sp, struct point *ret)
{
	//TBD();
	if ( sp->next == NULL) {
	    return 0;
        } else {
            struct sorted_points *curPt = sp->next;
            sp->next = sp->next->next;
            ret->x = curPt->coor->x;
            ret->y = curPt->coor->y;
            curPt->next = NULL;
            free(curPt->coor);
            free(curPt);
            return 1;
        }
}

int
sp_remove_last(struct sorted_points *sp, struct point *ret)
{
	//TBD();
	if ( sp->next == NULL) {
	    return 0;
	} else {
	    struct sorted_points *curPt = sp->next;
	    if (curPt->next == NULL) {
	        ret->x = curPt->coor->x;
	        ret->y = curPt->coor->y;
	        sp->next = NULL;
	        free(curPt->coor);
	        free(curPt);
	        return 1;
	    }
	    while ( curPt->next != NULL) {
	        if ( curPt->next->next == NULL) {
	            ret->x = curPt->next->coor->x;
	            ret->y = curPt->next->coor->y;
	            free(curPt->next->coor);
	            free(curPt->next);
	            curPt->next = NULL;
    	            return 1;
	        }
	        curPt = curPt->next;
	    }
	    return 0;
	}
}

int
sp_remove_by_index(struct sorted_points *sp, int index, struct point *ret)
{
	//TBD();
	if (index < 0 || sp->next == NULL) {
	    return 0;
	} else {
	    int count = 0;
	    struct sorted_points *curPt = sp->next;
	    struct sorted_points *prePt = NULL;
            while (curPt != NULL) {
                if ( index == count ) {
                    //remove curPt
                    if ( count == 0 ) {
                        sp->next = sp->next->next;
                        //curPt->next = NULL;
                        ret->x = curPt->coor->x;
                        ret->y = curPt->coor->y;
                        free(curPt->coor);
                        free(curPt);
                        return 1;
                    } else {
                        prePt->next = curPt->next;
                        //curPt->next = NULL;
                        ret->x = curPt->coor->x;
                        ret->y = curPt->coor->y;
                        //printf("%d: (%f, %f)\n", count, ret->x, ret->y);
                        free(curPt->coor);
                        free(curPt);
                        return 1;
                    }
                } else if ( count < index ) {
                    //increment
                    if ( curPt->next == NULL ) {
                        //printf("%d\n", count);
                        return 0; 
                    } else {
                        prePt = curPt;
                        curPt = curPt->next;
                        count = count + 1;
                    }
                }
            }
            return 0;
	}
}

int
sp_delete_duplicates(struct sorted_points *sp)
{
	//TBD();
	int count = 0;
	if ( sp->next == NULL || sp->next->next == NULL ) return count;
	else {
	    struct sorted_points *curPt = sp->next->next;
	    struct sorted_points *prePt = sp->next;
	    while (curPt != NULL) {
	        if (curPt->coor->x == prePt->coor->x && curPt->coor->y == prePt->coor->y) {
    	            prePt->next = curPt->next;
    	            struct sorted_points *temp = curPt;
	            curPt = curPt->next;
	            temp->next = NULL;
	            free(temp->coor);
	            free(temp);
	            count++;
	        } else {
	            prePt = curPt;
	            curPt = curPt->next;
	        }
	    }
	    return count;
	}
}

