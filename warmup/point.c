#include <assert.h>
#include "common.h"
#include "point.h"
#include <math.h>

void
point_translate(struct point *p, double x, double y)
{
	//TBD();
	p->x += x;
	p->y += y;
}

double
point_distance(const struct point *p1, const struct point *p2)
{
	//TBD();
	double res = 0.0;
        res = sqrt( (p1->x - p2->x)*(p1->x - p2->x) + (p1->y - p2->y)*(p1->y - p2->y) );

	return res;
}

int
point_compare(const struct point *p1, const struct point *p2)
{
	//TBD();
        double dis1, dis2;
        struct point origin;

        point_set(&origin, 0, 0);

        dis1 = point_distance(p1, &origin);
        dis2 = point_distance(p2, &origin);

        if (dis1 == dis2) return 0;
        else if (dis1 > dis2) return 1;
        else return -1;

}
