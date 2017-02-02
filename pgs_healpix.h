#ifndef __PGS_HEALPIX_H__
#define __PGS_HEALPIX_H__

/* function prototypes for the Healpix support functions */

Datum pg_nside2order(PG_FUNCTION_ARGS);
Datum pg_order2nside(PG_FUNCTION_ARGS);
Datum pg_nside2npix(PG_FUNCTION_ARGS);
Datum pg_npix2nside(PG_FUNCTION_ARGS);
Datum healpix_nest(PG_FUNCTION_ARGS);
Datum healpix_ring(PG_FUNCTION_ARGS);
Datum inv_healpix_nest(PG_FUNCTION_ARGS);
Datum inv_healpix_ring(PG_FUNCTION_ARGS);

#endif
