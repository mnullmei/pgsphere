#include <postgres.h>
#include <fmgr.h>
#include <catalog/pg_type.h>

#include <point.h> /* SPoint */

#include <chealpix.h>
#include <math.h>

static int ilog2(hpint64 x)
{
	int log = 0;
	unsigned w;
	for (w = 32; w; w >>= 1)
	{
		hpint64 y = x >> w;
		if (y)
		{
			log += w;
			x = y;
		}
	}
	return log;
}

static int order_invalid(int order)
{
	return (order < 0 || order > 29);
}

static int nside_invalid(hpint64 nside)
{
	return (nside <= 0 || nside - 1 & nside || order_invalid(ilog2(nside)));
}

static hpint64 c_nside(int order)
{
	hpint64 one_bit = 1;
	return one_bit << order;
}

PG_FUNCTION_INFO_V1(pg_nside2order);
Datum pg_nside2order(PG_FUNCTION_ARGS)
{
	hpint64 nside = PG_GETARG_INT64(0);
	if (nside_invalid(nside))
		PG_RETURN_NULL();
	PG_RETURN_INT32(ilog2(nside));
}

PG_FUNCTION_INFO_V1(pg_order2nside);
Datum pg_order2nside(PG_FUNCTION_ARGS)
{
	int32 order = PG_GETARG_INT32(0);
	if (order_invalid(order))
		PG_RETURN_NULL();

elog(LOG, "order = %d", order);
hpint64 goof = c_nside(order);
elog(LOG, "goof = %lld", goof);

	PG_RETURN_INT64(c_nside(order));
}

PG_FUNCTION_INFO_V1(pg_nside2npix);
Datum pg_nside2npix(PG_FUNCTION_ARGS)
{
	hpint64 nside = PG_GETARG_INT64(0);
	if (nside_invalid(nside))
		PG_RETURN_NULL();
	PG_RETURN_INT64(12 * nside * nside);
}

PG_FUNCTION_INFO_V1(pg_npix2nside);
Datum pg_npix2nside(PG_FUNCTION_ARGS)
{
	hpint64 npix = PG_GETARG_INT64(0);
	hpint64 nside;
	if (npix < 12)
		PG_RETURN_NULL();
	nside = floor(sqrt(npix * (1.0 / 12)) + 0.5);
	if (nside_invalid(nside))
		PG_RETURN_NULL();
	PG_RETURN_INT64(nside);
}


PG_FUNCTION_INFO_V1(healpix_nest);
Datum healpix_nest(PG_FUNCTION_ARGS)
{
	int32 order = PG_GETARG_INT32(0);
	SPoint* p = (SPoint*) PG_GETARG_POINTER(1);
	hpint64 i;
	if (order_invalid(order))
		PG_RETURN_NULL();
	ang2pix_nest64(c_nside(order), PIH - p->lat, p->lng, &i);
	PG_RETURN_INT64(i);
}

PG_FUNCTION_INFO_V1(healpix_ring);
Datum healpix_ring(PG_FUNCTION_ARGS)
{
	int32 order = PG_GETARG_INT32(0);
	SPoint* p = (SPoint*) PG_GETARG_POINTER(1);
	hpint64 i;
	if (order_invalid(order))
		PG_RETURN_NULL();
	ang2pix_ring64(c_nside(order), PIH - p->lat, p->lng, &i);
	PG_RETURN_INT64(i);
}


PG_FUNCTION_INFO_V1(inv_healpix_nest);
Datum inv_healpix_nest(PG_FUNCTION_ARGS)
{
	int32 order = PG_GETARG_INT32(0);
	hpint64 i = PG_GETARG_INT64(1);
	double theta = 0;
	SPoint* p = (SPoint*) palloc(sizeof(SPoint));
	if (order_invalid(order))
		PG_RETURN_NULL();
	pix2ang_nest64(c_nside(order), i, &theta, &p->lng);
	p->lat = PIH - theta;
	PG_RETURN_POINTER(p);
}

PG_FUNCTION_INFO_V1(inv_healpix_ring);
Datum inv_healpix_ring(PG_FUNCTION_ARGS)
{
	int32 order = PG_GETARG_INT32(0);
	hpint64 i = PG_GETARG_INT64(1);
	double theta = 0;
	SPoint* p = (SPoint*) palloc(sizeof(SPoint));
	if (order_invalid(order))
		PG_RETURN_NULL();
	pix2ang_ring64(c_nside(order), i, &theta, &p->lng);
	p->lat = PIH - theta;
	PG_RETURN_POINTER(p);
}
