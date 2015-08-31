#include "gist.h"

/*!
  \file
  \brief Functions needed to build a GIST index 
*/


/*! \defgroup PGS_KEY_REL Key relationships */
/*!
  \addtogroup PGS_KEY_REL
  @{
*/
#define SCKEY_DISJ    0 //!< two keys are disjunct
#define SCKEY_OVERLAP 1 //!< two keys are overlapping
#define SCKEY_IN      2 //!< first key contains second key
#define SCKEY_SAME    3 //!< keys are equal
/* @} */

#ifndef DOXYGEN_SHOULD_SKIP_THIS

  PG_FUNCTION_INFO_V1(spherekey_in);
  PG_FUNCTION_INFO_V1(spherekey_out);
  PG_FUNCTION_INFO_V1(g_spherekey_decompress);
  PG_FUNCTION_INFO_V1(g_scircle_compress);
  PG_FUNCTION_INFO_V1(g_spoint_compress);
  PG_FUNCTION_INFO_V1(g_sline_compress);
  PG_FUNCTION_INFO_V1(g_spath_compress);
  PG_FUNCTION_INFO_V1(g_spoly_compress);
  PG_FUNCTION_INFO_V1(g_sellipse_compress);
  PG_FUNCTION_INFO_V1(g_sbox_compress);
  PG_FUNCTION_INFO_V1(g_spherekey_union );
  PG_FUNCTION_INFO_V1(g_spherekey_same);
  PG_FUNCTION_INFO_V1(g_spoint_consistent);
  PG_FUNCTION_INFO_V1(g_scircle_consistent);
  PG_FUNCTION_INFO_V1(g_sline_consistent);
  PG_FUNCTION_INFO_V1(g_spath_consistent);
  PG_FUNCTION_INFO_V1(g_spoly_consistent);
  PG_FUNCTION_INFO_V1(g_sellipse_consistent);
  PG_FUNCTION_INFO_V1(g_sbox_consistent);
  PG_FUNCTION_INFO_V1(g_spherekey_penalty);
  PG_FUNCTION_INFO_V1(g_spherekey_picksplit);

#endif


  /*!
    \brief Returns the \link  PGS_KEY_REL Relationship \endlink of two keys
    \param k1 pointer to first key
    \param k2 pointer to second key
    \return \link  PGS_KEY_REL Relationship \endlink
  */
  static uchar  spherekey_interleave ( const int32 * k1 ,  const int32 * k2 )
  {
    uchar          i ;
    static char    tb;

    // i represents x,y,z

    tb = 0;
    for ( i = 0 ; i<3 ; i++ ){
      tb |= ( ( k2[i] > k1[i+3] ) || ( k1[i] > k2[i+3] ) );
      if ( tb ) break;
    }
    if ( tb ){
     return SCKEY_DISJ;
    }
    tb = 1;
    for ( i = 0 ; i<3 ; i++ ){
      tb &= ( ( k1[i] == k2[i] ) && ( k1[i+3] == k2[i+3] ) );
      if ( ! tb ) break;
    }
    if ( tb ){
     return SCKEY_SAME;
    }
    tb = 1;
    for ( i = 0 ; i<3 ; i++ ){
      tb &= ( k1[i] <= k2[i] && k1[i+3] >= k2[i+3] );
      if ( ! tb ) break;
    }
    if ( tb ){
      // v2 in v1
      return SCKEY_IN;
    }
    return SCKEY_OVERLAP;
  }

  /* gives the cube size of spherekey */
  /*!
    \brief Returns the Volume of a key ( cube )
    \param v pointer to key
    \return Volume
  */
  static double spherekey_size ( const int32 * v )
  {
    static const int32  ks =  MAXCVALUE ;
    double d = ( ( double ) ( v[3] - v[0] ) )/ks 
             * ( ( double ) ( v[4] - v[1] ) )/ks
             * ( ( double ) ( v[5] - v[2] ) )/ks;
    return d;
  }

  Datum  spherekey_in(PG_FUNCTION_ARGS)
  {
    elog ( ERROR , "Not implemented!" );
    PG_RETURN_POINTER ( NULL );
  }

  Datum spherekey_out(PG_FUNCTION_ARGS)
  {

    static const float8  ks =  (float8) MAXCVALUE ;
    int32 * k     =  ( int32 * ) PG_GETARG_POINTER ( 0 ) ;
    char * buffer =  ( char  * ) MALLOC ( 1024 ) ;

    sprintf( 
      buffer,
      "(%.9f,%.9f,%.9f),(%.9f,%.9f,%.9f)",
      k[0]/ks,
      k[1]/ks,
      k[2]/ks,
      k[3]/ks,
      k[4]/ks,
      k[5]/ks
    );

    PG_RETURN_CSTRING ( buffer ) ;

  }

  Datum g_spherekey_decompress(PG_FUNCTION_ARGS)
  {
    PG_RETURN_DATUM(PG_GETARG_DATUM(0));
  }


/*!
  \brief general compress method for all data types
  \param type \link PGS_DATA_TYPES data type \endlink
  \param genkey function to generate the key value
  \see key.c
*/

#define PGS_COMPRESS( type, genkey, detoast )  do { \
    GISTENTRY  *entry = (GISTENTRY *) PG_GETARG_POINTER(0); \
    GISTENTRY  *retval; \
    if (entry->leafkey) \
    { \
      retval  =  MALLOC ( sizeof ( GISTENTRY ) ); \
      if ( DatumGetPointer(entry->key) != NULL ){ \
        int32 * k = ( int32 * ) MALLOC ( KEYSIZE ) ; \
        if( detoast ) \
        { \
          genkey ( k , ( type * )  DatumGetPointer( PG_DETOAST_DATUM( entry->key ) ) ) ; \
        } else { \
          genkey ( k , ( type * )  DatumGetPointer( entry->key ) ) ; \
        } \
        gistentryinit(*retval, PointerGetDatum(k) , \
          entry->rel, entry->page, \
          entry->offset, FALSE ); \
      } else { \
        gistentryinit(*retval, (Datum) 0, \
          entry->rel, entry->page, \
          entry->offset, FALSE ); \
      } \
    } else { \
      retval = entry; \
    } \
    PG_RETURN_POINTER(retval); \
  } while (0) ;

  Datum g_scircle_compress(PG_FUNCTION_ARGS)
  {
    PGS_COMPRESS( SCIRCLE , spherecircle_gen_key, 0 )
  }
  
  Datum g_spoint_compress(PG_FUNCTION_ARGS)
  {
    PGS_COMPRESS( SPoint , spherepoint_gen_key, 0 )
  }

  Datum g_sline_compress(PG_FUNCTION_ARGS)
  {
    PGS_COMPRESS( SLine , sphereline_gen_key, 0 )
  }

  Datum g_spath_compress(PG_FUNCTION_ARGS)
  {
    PGS_COMPRESS( SPATH , spherepath_gen_key, 1 )
  }

  Datum g_spoly_compress(PG_FUNCTION_ARGS)
  {
    PGS_COMPRESS( SPOLY , spherepoly_gen_key, 1)
  }

  Datum g_sellipse_compress(PG_FUNCTION_ARGS)
  {
    PGS_COMPRESS( SELLIPSE , sphereellipse_gen_key, 0)
  }

  Datum g_sbox_compress(PG_FUNCTION_ARGS)
  {
    PGS_COMPRESS( SBOX , spherebox_gen_key, 0)
  }

  Datum g_spherekey_union (PG_FUNCTION_ARGS)
  {
      GistEntryVector    *entryvec = ( GistEntryVector *) PG_GETARG_POINTER(0);
    int                   *sizep = (int *)   PG_GETARG_POINTER(1);
    int             numranges, i;
    int32                 * ret  = ( int32 * ) MALLOC ( KEYSIZE ) ;

      numranges = entryvec->n;
      memcpy( (void *) ret , (void *) DatumGetPointer(entryvec->vector[0].key) , KEYSIZE );

    for (i = 1; i < numranges; i++)
    {
        spherekey_union_two ( ret , ( int32 *) DatumGetPointer(entryvec->vector[i].key) );
    }
    *sizep = KEYSIZE;
    PG_RETURN_POINTER( ret );
  }

  Datum g_spherekey_same(PG_FUNCTION_ARGS)
  {
    int32          *c1 = ( int32 * ) PG_GETARG_POINTER(0);
    int32          *c2 = ( int32 * ) PG_GETARG_POINTER(1);
    bool       *result = ( bool  * ) PG_GETARG_POINTER(2);
    static int       i ;
 
    *result            = TRUE;

    if ( c1 && c2 ){
      for ( i=0; i<6; i++ ){
        *result &= ( c1[i] == c2[i] );
      }
    } else {
      *result = (c1 == NULL && c2 == NULL) ? TRUE : FALSE;
    }

    PG_RETURN_POINTER(result);
  }


/*!
  \brief general interleave method with query cache
  \param type \link PGS_DATA_TYPES data type \endlink
  \param genkey function to generate the key value
  \param dir for spherekey_interleave what value is the first
          - 0 : the query key
          - not 0 : the key entry
  \see key.c gq_cache.c
*/
#define SCK_INTERLEAVE( type , genkey , dir ) do { \
  int32 * q = NULL ; \
  if ( ! gq_cache_get_value ( PGS_TYPE_##type , query, &q ) ){ \
    q = ( int32 *) malloc ( KEYSIZE ); \
    genkey ( q, ( type * ) query ); \
    gq_cache_set_value ( PGS_TYPE_##type , query, q ) ; \
    free ( q ); \
    gq_cache_get_value ( PGS_TYPE_##type , query, &q ) ; \
  } \
  if ( dir ){ \
    i = spherekey_interleave ( ent, q  ); \
  } else { \
    i = spherekey_interleave ( q , ent ); \
  } \
} while (0);


  Datum g_spoint_consistent(PG_FUNCTION_ARGS)
  { 
    GISTENTRY          *entry = (GISTENTRY *) PG_GETARG_POINTER(0);
    void               *query = ( void * ) PG_GETARG_POINTER(1) ;
    StrategyNumber strategy   = (StrategyNumber) PG_GETARG_UINT16(2);
    bool             result   = FALSE ;

    if ( DatumGetPointer(entry->key) == NULL || ! query ) {
      PG_RETURN_BOOL(FALSE);
    } else {

      bool           *recheck = (bool *) PG_GETARG_POINTER(4);
      int32            *ent   = ( int32 * ) DatumGetPointer( entry->key ) ;
      int i = SCKEY_DISJ ;
      *recheck = true;

      switch ( strategy ) {
        case  1 : SCK_INTERLEAVE ( SPoint   , spherepoint_gen_key   , 1 ); break;
        case 11 : SCK_INTERLEAVE ( SCIRCLE  , spherecircle_gen_key  , 0 ); break;
        case 12 : SCK_INTERLEAVE ( SLine    , sphereline_gen_key    , 0 ); break;
        case 13 : SCK_INTERLEAVE ( SPATH    , spherepath_gen_key    , 0 ); break;
        case 14 : SCK_INTERLEAVE ( SPOLY    , spherepoly_gen_key    , 0 ); break;
        case 15 : SCK_INTERLEAVE ( SELLIPSE , sphereellipse_gen_key , 0 ); break;
        case 16 : SCK_INTERLEAVE ( SBOX     , spherebox_gen_key     , 0 ); break;
        case 37 : SCK_INTERLEAVE ( SCIRCLE  , spherecircle_gen_key  , 0 ); break;
        case 38 : SCK_INTERLEAVE ( SLine    , sphereline_gen_key    , 0 ); break;
        case 39 : SCK_INTERLEAVE ( SPATH    , spherepath_gen_key    , 0 ); break;
        case 40 : SCK_INTERLEAVE ( SPOLY    , spherepoly_gen_key    , 0 ); break;
        case 41 : SCK_INTERLEAVE ( SELLIPSE , sphereellipse_gen_key , 0 ); break;
        case 42 : SCK_INTERLEAVE ( SBOX     , spherebox_gen_key     , 0 ); break;
      }

      if (GIST_LEAF(entry)) {
        switch ( strategy ) {
          case  1 : if ( i == SCKEY_SAME     )  result = TRUE; break;
          default : if ( i >  SCKEY_OVERLAP  )  result = TRUE; break;
        }
      } else {

        switch ( strategy ) {
          case  1 : if ( i > SCKEY_OVERLAP )  result = TRUE; break;
          default : if ( i > SCKEY_DISJ    )  result = TRUE; break;
        }

      }

      PG_RETURN_BOOL( result );

    }
    PG_RETURN_BOOL(FALSE);
  }
  

  Datum g_scircle_consistent(PG_FUNCTION_ARGS)
  { 
    GISTENTRY          *entry = (GISTENTRY *) PG_GETARG_POINTER(0);
    void               *query = ( void * ) PG_GETARG_POINTER(1) ;
    StrategyNumber strategy   = (StrategyNumber) PG_GETARG_UINT16(2);
    bool             result   = FALSE ;

    if ( DatumGetPointer(entry->key) == NULL || ! query ) {
      PG_RETURN_BOOL(FALSE);
    } else {

      bool           *recheck = (bool *) PG_GETARG_POINTER(4);
      int32            *ent   = ( int32 * ) DatumGetPointer( entry->key ) ;
      int i = SCKEY_DISJ ;
      *recheck = true;

      switch ( strategy ) {
        case  1 : SCK_INTERLEAVE ( SCIRCLE  , spherecircle_gen_key  , 1 ); break;
        case 11 : SCK_INTERLEAVE ( SCIRCLE  , spherecircle_gen_key  , 0 ); break;
        case 12 : SCK_INTERLEAVE ( SPOLY    , spherepoly_gen_key    , 0 ); break;
        case 13 : SCK_INTERLEAVE ( SELLIPSE , sphereellipse_gen_key , 0 ); break;
        case 14 : SCK_INTERLEAVE ( SBOX     , spherebox_gen_key     , 0 ); break;
        case 21 : SCK_INTERLEAVE ( SPoint   , spherepoint_gen_key   , 1 ); break;
        case 22 : SCK_INTERLEAVE ( SCIRCLE  , spherecircle_gen_key  , 1 ); break;
        case 23 : SCK_INTERLEAVE ( SLine    , sphereline_gen_key    , 1 ); break;
        case 24 : SCK_INTERLEAVE ( SPATH    , spherepath_gen_key    , 1 ); break;
        case 25 : SCK_INTERLEAVE ( SPOLY    , spherepoly_gen_key    , 1 ); break;
        case 26 : SCK_INTERLEAVE ( SELLIPSE , sphereellipse_gen_key , 1 ); break;
        case 27 : SCK_INTERLEAVE ( SBOX     , spherebox_gen_key     , 1 ); break;
        case 31 : SCK_INTERLEAVE ( SCIRCLE  , spherecircle_gen_key  , 0 ); break;
        case 32 : SCK_INTERLEAVE ( SLine    , sphereline_gen_key    , 0 ); break;
        case 33 : SCK_INTERLEAVE ( SPATH    , spherepath_gen_key    , 0 ); break;
        case 34 : SCK_INTERLEAVE ( SPOLY    , spherepoly_gen_key    , 0 ); break;
        case 35 : SCK_INTERLEAVE ( SELLIPSE , sphereellipse_gen_key , 0 ); break;
        case 36 : SCK_INTERLEAVE ( SBOX     , spherebox_gen_key     , 0 ); break;
        case 37 : SCK_INTERLEAVE ( SCIRCLE  , spherecircle_gen_key  , 0 ); break;
        case 38 : SCK_INTERLEAVE ( SPOLY    , spherepoly_gen_key    , 0 ); break;
        case 39 : SCK_INTERLEAVE ( SELLIPSE , sphereellipse_gen_key , 0 ); break;
        case 40 : SCK_INTERLEAVE ( SBOX     , spherebox_gen_key     , 0 ); break;
        case 43 : SCK_INTERLEAVE ( SPoint   , spherepoint_gen_key   , 1 ); break;
        case 44 : SCK_INTERLEAVE ( SCIRCLE  , spherecircle_gen_key  , 1 ); break;
        case 45 : SCK_INTERLEAVE ( SLine    , sphereline_gen_key    , 1 ); break;
        case 46 : SCK_INTERLEAVE ( SPATH    , spherepath_gen_key    , 1 ); break;
        case 47 : SCK_INTERLEAVE ( SPOLY    , spherepoly_gen_key    , 1 ); break;
        case 48 : SCK_INTERLEAVE ( SELLIPSE , sphereellipse_gen_key , 1 ); break;
        case 49 : SCK_INTERLEAVE ( SBOX     , spherebox_gen_key     , 1 ); break;
      }

      if (GIST_LEAF(entry)) {

        switch ( strategy ) {
          case  1 : if ( i == SCKEY_SAME     )  result = TRUE; break;
          default : if ( i >  SCKEY_DISJ     )  result = TRUE; break;
        }
      } else {
        switch ( strategy ) {
          case  1 : if ( i >  SCKEY_OVERLAP  )  result = TRUE; break;
          default : if ( i >  SCKEY_DISJ     )  result = TRUE; break;
        }
      }
      PG_RETURN_BOOL( result );
    }
    PG_RETURN_BOOL(FALSE);
  }
  


  Datum g_sline_consistent(PG_FUNCTION_ARGS)
  { 
    GISTENTRY          *entry = (GISTENTRY *) PG_GETARG_POINTER(0);
    void               *query = ( void * ) PG_GETARG_POINTER(1) ;
    StrategyNumber strategy   = (StrategyNumber) PG_GETARG_UINT16(2);
    bool             result   = FALSE ;

    if ( DatumGetPointer(entry->key) == NULL || ! query ) {
      PG_RETURN_BOOL(FALSE);
    } else {

      bool           *recheck = (bool *) PG_GETARG_POINTER(4);
      int32            *ent   = ( int32 * ) DatumGetPointer( entry->key ) ;
      int i = SCKEY_DISJ ;
      *recheck = true;

      switch ( strategy ) {
        case  1 : 
        case  2 : SCK_INTERLEAVE ( SLine    , sphereline_gen_key    , 1 ); break;
        case 11 : SCK_INTERLEAVE ( SCIRCLE  , spherecircle_gen_key  , 0 ); break;
        case 12 : SCK_INTERLEAVE ( SPOLY    , spherepoly_gen_key    , 0 ); break;
        case 13 : SCK_INTERLEAVE ( SELLIPSE , sphereellipse_gen_key , 0 ); break;
        case 14 : SCK_INTERLEAVE ( SBOX     , spherebox_gen_key     , 0 ); break;
        case 21 : SCK_INTERLEAVE ( SPoint   , spherepoint_gen_key   , 1 ); break;
        case 31 : SCK_INTERLEAVE ( SCIRCLE  , spherecircle_gen_key  , 1 ); break;
        case 32 : SCK_INTERLEAVE ( SLine    , sphereline_gen_key    , 1 ); break;
        case 33 : SCK_INTERLEAVE ( SPATH    , spherepath_gen_key    , 1 ); break;
        case 34 : SCK_INTERLEAVE ( SPOLY    , spherepoly_gen_key    , 1 ); break;
        case 35 : SCK_INTERLEAVE ( SELLIPSE , sphereellipse_gen_key , 1 ); break;
        case 36 : SCK_INTERLEAVE ( SBOX     , spherebox_gen_key     , 1 ); break;
        case 37 : SCK_INTERLEAVE ( SCIRCLE  , spherecircle_gen_key  , 0 ); break;
        case 38 : SCK_INTERLEAVE ( SPOLY    , spherepoly_gen_key    , 0 ); break;
        case 39 : SCK_INTERLEAVE ( SELLIPSE , sphereellipse_gen_key , 0 ); break;
        case 40 : SCK_INTERLEAVE ( SBOX     , spherebox_gen_key     , 0 ); break;
        case 43 : SCK_INTERLEAVE ( SPoint   , spherepoint_gen_key   , 1 ); break;
      }

      if (GIST_LEAF(entry)) {
        switch ( strategy ) {
          case  1 : if ( i == SCKEY_SAME     )  result = TRUE; break;
          default : if ( i >  SCKEY_DISJ     )  result = TRUE; break;
        }
      } else {
        switch ( strategy ) {
          case  1 : if ( i >  SCKEY_OVERLAP  )  result = TRUE; break;
          default : if ( i >  SCKEY_DISJ     )  result = TRUE; break;
        }
      }
      PG_RETURN_BOOL( result );
    }
    PG_RETURN_BOOL(FALSE);
  }
  


  Datum g_spath_consistent(PG_FUNCTION_ARGS)
  { 
    GISTENTRY          *entry = (GISTENTRY *) PG_GETARG_POINTER(0);
    void               *query = ( void * ) PG_GETARG_POINTER(1) ;
    StrategyNumber strategy   = (StrategyNumber) PG_GETARG_UINT16(2);
    bool             result   = FALSE ;

    if ( DatumGetPointer(entry->key) == NULL || ! query ) {
      PG_RETURN_BOOL(FALSE);
    } else {

      bool           *recheck = (bool *) PG_GETARG_POINTER(4);
      int32            *ent   = ( int32 * ) DatumGetPointer( entry->key ) ;
      int i = SCKEY_DISJ ;
      *recheck = true;

      switch ( strategy ) {
        case  1 : SCK_INTERLEAVE ( SPATH    , spherepath_gen_key    , 1 ); break;
        case 11 : SCK_INTERLEAVE ( SCIRCLE  , spherecircle_gen_key  , 0 ); break;
        case 12 : SCK_INTERLEAVE ( SPOLY    , spherepoly_gen_key    , 0 ); break;
        case 13 : SCK_INTERLEAVE ( SELLIPSE , sphereellipse_gen_key , 0 ); break;
        case 14 : SCK_INTERLEAVE ( SBOX     , spherebox_gen_key     , 0 ); break;
        case 21 : SCK_INTERLEAVE ( SPoint   , spherepoint_gen_key   , 1 ); break;
        case 31 : SCK_INTERLEAVE ( SCIRCLE  , spherecircle_gen_key  , 1 ); break;
        case 32 : SCK_INTERLEAVE ( SLine    , sphereline_gen_key    , 1 ); break;
        case 33 : SCK_INTERLEAVE ( SPATH    , spherepath_gen_key    , 1 ); break;
        case 34 : SCK_INTERLEAVE ( SPOLY    , spherepoly_gen_key    , 1 ); break;
        case 35 : SCK_INTERLEAVE ( SELLIPSE , sphereellipse_gen_key , 1 ); break;
        case 36 : SCK_INTERLEAVE ( SBOX     , spherebox_gen_key     , 1 ); break;
        case 37 : SCK_INTERLEAVE ( SCIRCLE  , spherecircle_gen_key  , 0 ); break;
        case 38 : SCK_INTERLEAVE ( SPOLY    , spherepoly_gen_key    , 0 ); break;
        case 39 : SCK_INTERLEAVE ( SELLIPSE , sphereellipse_gen_key , 0 ); break;
        case 40 : SCK_INTERLEAVE ( SBOX     , spherebox_gen_key     , 0 ); break;
        case 43 : SCK_INTERLEAVE ( SPoint   , spherepoint_gen_key   , 1 ); break;
      }

      if (GIST_LEAF(entry)) {
        switch ( strategy ) {
          case  1 : if ( i == SCKEY_SAME     )  result = TRUE; break;
          default : if ( i >  SCKEY_DISJ     )  result = TRUE; break;
        }
      } else {
        switch ( strategy ) {
          case  1 : if ( i >  SCKEY_OVERLAP  )  result = TRUE; break;
          default : if ( i >  SCKEY_DISJ     )  result = TRUE; break;
        }
      }
      PG_RETURN_BOOL( result );
    }
    PG_RETURN_BOOL(FALSE);
  }
  

  Datum g_spoly_consistent(PG_FUNCTION_ARGS)
  { 
    GISTENTRY          *entry = (GISTENTRY *) PG_GETARG_POINTER(0);
    void               *query = ( void * ) PG_GETARG_POINTER(1) ;
    StrategyNumber strategy   = (StrategyNumber) PG_GETARG_UINT16(2);
    bool             result   = FALSE ;

    if ( DatumGetPointer(entry->key) == NULL || ! query ) {
      PG_RETURN_BOOL(FALSE);
    } else {

      bool           *recheck = (bool *) PG_GETARG_POINTER(4);
      int32            *ent   = ( int32 * ) DatumGetPointer( entry->key ) ;
      int i = SCKEY_DISJ ;
      *recheck = true;

      switch ( strategy ) {
        case  1 : SCK_INTERLEAVE ( SPATH    , spherepath_gen_key    , 1 ); break;
        case 11 : SCK_INTERLEAVE ( SCIRCLE  , spherecircle_gen_key  , 0 ); break;
        case 12 : SCK_INTERLEAVE ( SPOLY    , spherepoly_gen_key    , 0 ); break;
        case 13 : SCK_INTERLEAVE ( SELLIPSE , sphereellipse_gen_key , 0 ); break;
        case 14 : SCK_INTERLEAVE ( SBOX     , spherebox_gen_key     , 0 ); break;
        case 21 : SCK_INTERLEAVE ( SPoint   , spherepoint_gen_key   , 1 ); break;
        case 22 : SCK_INTERLEAVE ( SCIRCLE  , spherecircle_gen_key  , 1 ); break;
        case 23 : SCK_INTERLEAVE ( SLine    , sphereline_gen_key    , 1 ); break;
        case 24 : SCK_INTERLEAVE ( SPATH    , spherepath_gen_key    , 1 ); break;
        case 25 : SCK_INTERLEAVE ( SPOLY    , spherepoly_gen_key    , 1 ); break;
        case 26 : SCK_INTERLEAVE ( SELLIPSE , sphereellipse_gen_key , 1 ); break;
        case 27 : SCK_INTERLEAVE ( SBOX     , spherebox_gen_key     , 1 ); break;
        case 31 : SCK_INTERLEAVE ( SCIRCLE  , spherecircle_gen_key  , 0 ); break;
        case 32 : SCK_INTERLEAVE ( SLine    , sphereline_gen_key    , 0 ); break;
        case 33 : SCK_INTERLEAVE ( SPATH    , spherepath_gen_key    , 0 ); break;
        case 34 : SCK_INTERLEAVE ( SPOLY    , spherepoly_gen_key    , 0 ); break;
        case 35 : SCK_INTERLEAVE ( SELLIPSE , sphereellipse_gen_key , 0 ); break;
        case 36 : SCK_INTERLEAVE ( SBOX     , spherebox_gen_key     , 0 ); break;
        case 37 : SCK_INTERLEAVE ( SCIRCLE  , spherecircle_gen_key  , 0 ); break;
        case 38 : SCK_INTERLEAVE ( SPOLY    , spherepoly_gen_key    , 0 ); break;
        case 39 : SCK_INTERLEAVE ( SELLIPSE , sphereellipse_gen_key , 0 ); break;
        case 40 : SCK_INTERLEAVE ( SBOX     , spherebox_gen_key     , 0 ); break;
        case 43 : SCK_INTERLEAVE ( SPoint   , spherepoint_gen_key   , 1 ); break;
        case 44 : SCK_INTERLEAVE ( SCIRCLE  , spherecircle_gen_key  , 1 ); break;
        case 45 : SCK_INTERLEAVE ( SLine    , sphereline_gen_key    , 1 ); break;
        case 46 : SCK_INTERLEAVE ( SPATH    , spherepath_gen_key    , 1 ); break;
        case 47 : SCK_INTERLEAVE ( SPOLY    , spherepoly_gen_key    , 1 ); break;
        case 48 : SCK_INTERLEAVE ( SELLIPSE , sphereellipse_gen_key , 1 ); break;
        case 49 : SCK_INTERLEAVE ( SBOX     , spherebox_gen_key     , 1 ); break;
      }

      if (GIST_LEAF(entry)) {
        switch ( strategy ) {
          case  1 : if ( i == SCKEY_SAME     )  result = TRUE; break;
          default : if ( i >  SCKEY_DISJ     )  result = TRUE; break;
        }
      } else {
        switch ( strategy ) {
          case  1 : if ( i >  SCKEY_OVERLAP  )  result = TRUE; break;
          default : if ( i >  SCKEY_DISJ     )  result = TRUE; break;
        }
      }
      PG_RETURN_BOOL( result );
    }
    PG_RETURN_BOOL(FALSE);
  }


  Datum g_sellipse_consistent(PG_FUNCTION_ARGS)
  { 
    GISTENTRY          *entry = (GISTENTRY *) PG_GETARG_POINTER(0);
    void               *query = ( void * ) PG_GETARG_POINTER(1) ;
    StrategyNumber strategy   = (StrategyNumber) PG_GETARG_UINT16(2);
    bool             result   = FALSE ;

    if ( DatumGetPointer(entry->key) == NULL || ! query ) {
      PG_RETURN_BOOL(FALSE);
    } else {

      bool           *recheck = (bool *) PG_GETARG_POINTER(4);
      int32            *ent   = ( int32 * ) DatumGetPointer( entry->key ) ;
      int i = SCKEY_DISJ ;
      *recheck = true;

      switch ( strategy ) {
        case  1 : SCK_INTERLEAVE ( SELLIPSE , sphereellipse_gen_key , 1 ); break;
        case 11 : SCK_INTERLEAVE ( SCIRCLE  , spherecircle_gen_key  , 0 ); break;
        case 12 : SCK_INTERLEAVE ( SPOLY    , spherepoly_gen_key    , 0 ); break;
        case 13 : SCK_INTERLEAVE ( SELLIPSE , sphereellipse_gen_key , 0 ); break;
        case 14 : SCK_INTERLEAVE ( SBOX     , spherebox_gen_key     , 0 ); break;
        case 21 : SCK_INTERLEAVE ( SPoint   , spherepoint_gen_key   , 1 ); break;
        case 22 : SCK_INTERLEAVE ( SCIRCLE  , spherecircle_gen_key  , 1 ); break;
        case 23 : SCK_INTERLEAVE ( SLine    , sphereline_gen_key    , 1 ); break;
        case 24 : SCK_INTERLEAVE ( SPATH    , spherepath_gen_key    , 1 ); break;
        case 25 : SCK_INTERLEAVE ( SPOLY    , spherepoly_gen_key    , 1 ); break;
        case 26 : SCK_INTERLEAVE ( SELLIPSE , sphereellipse_gen_key , 1 ); break;
        case 27 : SCK_INTERLEAVE ( SBOX     , spherebox_gen_key     , 1 ); break;
        case 31 : SCK_INTERLEAVE ( SCIRCLE  , spherecircle_gen_key  , 0 ); break;
        case 32 : SCK_INTERLEAVE ( SLine    , sphereline_gen_key    , 0 ); break;
        case 33 : SCK_INTERLEAVE ( SPATH    , spherepath_gen_key    , 0 ); break;
        case 34 : SCK_INTERLEAVE ( SPOLY    , spherepoly_gen_key    , 0 ); break;
        case 35 : SCK_INTERLEAVE ( SELLIPSE , sphereellipse_gen_key , 0 ); break;
        case 36 : SCK_INTERLEAVE ( SBOX     , spherebox_gen_key     , 0 ); break;
        case 37 : SCK_INTERLEAVE ( SCIRCLE  , spherecircle_gen_key  , 0 ); break;
        case 38 : SCK_INTERLEAVE ( SPOLY    , spherepoly_gen_key    , 0 ); break;
        case 39 : SCK_INTERLEAVE ( SELLIPSE , sphereellipse_gen_key , 0 ); break;
        case 40 : SCK_INTERLEAVE ( SBOX     , spherebox_gen_key     , 0 ); break;
        case 43 : SCK_INTERLEAVE ( SPoint   , spherepoint_gen_key   , 1 ); break;
        case 44 : SCK_INTERLEAVE ( SCIRCLE  , spherecircle_gen_key  , 1 ); break;
        case 45 : SCK_INTERLEAVE ( SLine    , sphereline_gen_key    , 1 ); break;
        case 46 : SCK_INTERLEAVE ( SPATH    , spherepath_gen_key    , 1 ); break;
        case 47 : SCK_INTERLEAVE ( SPOLY    , spherepoly_gen_key    , 1 ); break;
        case 48 : SCK_INTERLEAVE ( SELLIPSE , sphereellipse_gen_key , 1 ); break;
        case 49 : SCK_INTERLEAVE ( SBOX     , spherebox_gen_key     , 1 ); break;
      }

      if (GIST_LEAF(entry)) {
        switch ( strategy ) {
          case  1 : if ( i == SCKEY_SAME     )  result = TRUE; break;
          default : if ( i >  SCKEY_DISJ     )  result = TRUE; break;
        }
      } else {
        switch ( strategy ) {
          case  1 : if ( i >  SCKEY_OVERLAP  )  result = TRUE; break;
          default : if ( i >  SCKEY_DISJ     )  result = TRUE; break;
        }
      }
      PG_RETURN_BOOL( result );
    }
    PG_RETURN_BOOL(FALSE);
  }


  Datum g_sbox_consistent(PG_FUNCTION_ARGS)
  { 
    GISTENTRY          *entry = (GISTENTRY *) PG_GETARG_POINTER(0);
    void               *query = ( void * ) PG_GETARG_POINTER(1) ;
    StrategyNumber strategy   = (StrategyNumber) PG_GETARG_UINT16(2);
    bool             result   = FALSE ;

    if ( DatumGetPointer(entry->key) == NULL || ! query ) {
      PG_RETURN_BOOL(FALSE);
    } else {

      bool           *recheck = (bool *) PG_GETARG_POINTER(4);
      int32            *ent   = ( int32 * ) DatumGetPointer( entry->key ) ;
      int i = SCKEY_DISJ ;
      *recheck = true;

      switch ( strategy ) {
        case  1 : SCK_INTERLEAVE ( SBOX     , spherebox_gen_key     , 1 ); break;
        case 11 : SCK_INTERLEAVE ( SCIRCLE  , spherecircle_gen_key  , 0 ); break;
        case 12 : SCK_INTERLEAVE ( SPOLY    , spherepoly_gen_key    , 0 ); break;
        case 13 : SCK_INTERLEAVE ( SELLIPSE , sphereellipse_gen_key , 0 ); break;
        case 14 : SCK_INTERLEAVE ( SBOX     , spherebox_gen_key     , 0 ); break;
        case 21 : SCK_INTERLEAVE ( SPoint   , spherepoint_gen_key   , 1 ); break;
        case 22 : SCK_INTERLEAVE ( SCIRCLE  , spherecircle_gen_key  , 1 ); break;
        case 23 : SCK_INTERLEAVE ( SLine    , sphereline_gen_key    , 1 ); break;
        case 24 : SCK_INTERLEAVE ( SPATH    , spherepath_gen_key    , 1 ); break;
        case 25 : SCK_INTERLEAVE ( SPOLY    , spherepoly_gen_key    , 1 ); break;
        case 26 : SCK_INTERLEAVE ( SELLIPSE , sphereellipse_gen_key , 1 ); break;
        case 27 : SCK_INTERLEAVE ( SBOX     , spherebox_gen_key     , 1 ); break;
        case 31 : SCK_INTERLEAVE ( SCIRCLE  , spherecircle_gen_key  , 0 ); break;
        case 32 : SCK_INTERLEAVE ( SLine    , sphereline_gen_key    , 0 ); break;
        case 33 : SCK_INTERLEAVE ( SPATH    , spherepath_gen_key    , 0 ); break;
        case 34 : SCK_INTERLEAVE ( SPOLY    , spherepoly_gen_key    , 0 ); break;
        case 35 : SCK_INTERLEAVE ( SELLIPSE , sphereellipse_gen_key , 0 ); break;
        case 36 : SCK_INTERLEAVE ( SBOX     , spherebox_gen_key     , 0 ); break;
        case 37 : SCK_INTERLEAVE ( SCIRCLE  , spherecircle_gen_key  , 0 ); break;
        case 38 : SCK_INTERLEAVE ( SPOLY    , spherepoly_gen_key    , 0 ); break;
        case 39 : SCK_INTERLEAVE ( SELLIPSE , sphereellipse_gen_key , 0 ); break;
        case 40 : SCK_INTERLEAVE ( SBOX     , spherebox_gen_key     , 0 ); break;
        case 43 : SCK_INTERLEAVE ( SPoint   , spherepoint_gen_key   , 1 ); break;
        case 44 : SCK_INTERLEAVE ( SCIRCLE  , spherecircle_gen_key  , 1 ); break;
        case 45 : SCK_INTERLEAVE ( SLine    , sphereline_gen_key    , 1 ); break;
        case 46 : SCK_INTERLEAVE ( SPATH    , spherepath_gen_key    , 1 ); break;
        case 47 : SCK_INTERLEAVE ( SPOLY    , spherepoly_gen_key    , 1 ); break;
        case 48 : SCK_INTERLEAVE ( SELLIPSE , sphereellipse_gen_key , 1 ); break;
        case 49 : SCK_INTERLEAVE ( SBOX     , spherebox_gen_key     , 1 ); break;
      }

      if (GIST_LEAF(entry)) {
        switch ( strategy ) {
          case  1 : if ( i == SCKEY_SAME     )  result = TRUE; break;
          default : if ( i >  SCKEY_DISJ     )  result = TRUE; break;
        }
      } else {
        switch ( strategy ) {
          case  1 : if ( i >  SCKEY_OVERLAP  )  result = TRUE; break;
          default : if ( i >  SCKEY_DISJ     )  result = TRUE; break;
        }
      }
      PG_RETURN_BOOL( result );
    }
    PG_RETURN_BOOL(FALSE);
  }


  /* The GiST Penalty method for boxes.
     We have to make panalty as fast as possible ( offen called ! ) 
  */
  Datum g_spherekey_penalty(PG_FUNCTION_ARGS)
  {
    GISTENTRY  *origentry = (GISTENTRY *) PG_GETARG_POINTER(0);
    GISTENTRY  *newentry  = (GISTENTRY *) PG_GETARG_POINTER(1);
    float      *result    = (float *) PG_GETARG_POINTER(2);
    int32      *o         = (int32 *) DatumGetPointer( origentry->key );
    static int32 n[6]     ;

    if ( newentry == NULL ){
      PG_RETURN_POINTER( NULL );
    } else {
      double osize = spherekey_size( o );
      memcpy( (void *) &n[0], (void *) DatumGetPointer( newentry->key ), KEYSIZE );
      spherekey_union_two ( &n[0] ,  o );
      *result = ( float ) ( spherekey_size( &n[0] ) - osize );
      if( FPzero( *result ) ){
    	  if( FPzero( osize ) ){
    		*result = 0.0;
    	  } else {
    	    *result = 1.0 - ( 1.0 / ( 1.0 + osize ) );
    	  }
      } else {
    	  *result += 1.0;
      }
    }

    PG_RETURN_POINTER(result);
  }


#define WISH_F(a,b,c) (double)( -(double)(((a)-(b))*((a)-(b))*((a)-(b)))*(c) )

  typedef struct
  {
        double      cost;
        OffsetNumber pos;
  } SPLITCOST;

  static int
  comparecost(const void *a, const void *b)
  {
    if (((SPLITCOST *) a)->cost == ((SPLITCOST *) b)->cost)
         return 0;
     else
         return (((SPLITCOST *) a)->cost > ((SPLITCOST *) b)->cost) ? 1 : -1;
  }

  Datum g_spherekey_picksplit(PG_FUNCTION_ARGS)
  {
      GistEntryVector    *entryvec = ( GistEntryVector *) PG_GETARG_POINTER(0);
    GIST_SPLITVEC  *v = (GIST_SPLITVEC *) PG_GETARG_POINTER(1);
    OffsetNumber  i,j;
    int32	*datum_alpha, *datum_beta;
    int32	datum_l[6], datum_r[6];
    int32	union_dl[6],union_dr[6];
    int32	union_d[6], inter_d[6];
    double	size_alpha, size_beta;
    double	size_waste, waste=-1.0;
    double      size_l, size_r;
    int                     nbytes;
    OffsetNumber	seed_1 = 0, seed_2 = 0;
    OffsetNumber *left, *right;
    OffsetNumber maxoff;
    SPLITCOST  *costvector;

     maxoff  = entryvec->n - 1;
    nbytes = (maxoff + 2) * sizeof(OffsetNumber);
    v->spl_left  = (OffsetNumber *) MALLOC(nbytes);
    v->spl_right = (OffsetNumber *) MALLOC(nbytes);

    for (i = FirstOffsetNumber; i < maxoff; i = OffsetNumberNext(i)) {
	 	datum_alpha = (int32 *) DatumGetPointer(entryvec->vector[i].key);
	for (j = OffsetNumberNext(i); j <= maxoff; j = OffsetNumberNext(j)) {
		 	datum_beta = (int32 *) DatumGetPointer(entryvec->vector[j].key);
		memcpy( (void*)union_d, (void*)datum_alpha, KEYSIZE );
		memcpy( (void*)inter_d, (void*)datum_alpha, KEYSIZE );
		size_waste = spherekey_size( spherekey_union_two(union_d, datum_beta) ) - 
			( (spherekey_inter_two( inter_d, datum_beta )) ? spherekey_size(inter_d) : 0 );
		if (size_waste > waste) {
			waste = size_waste;
			seed_1 = i;
			seed_2 = j;
		}
	}
   }
		 
   left = v->spl_left;
   v->spl_nleft = 0;
   right = v->spl_right;
   v->spl_nright = 0;
   if (seed_1 == 0 || seed_2 == 0) {
	seed_1 = 1;
	seed_2 = 2;
   }

     memcpy( (void*)datum_l, (void *) DatumGetPointer(entryvec->vector[seed_1].key), KEYSIZE );
     memcpy( (void*)datum_r, (void *) DatumGetPointer(entryvec->vector[seed_2].key), KEYSIZE );
   size_l = spherekey_size(datum_l);
   size_r = spherekey_size(datum_r);

   costvector = (SPLITCOST *) MALLOC(sizeof(SPLITCOST) * maxoff);
   for (i = FirstOffsetNumber; i <= maxoff; i = OffsetNumberNext(i)) {
	costvector[i - 1].pos = i;
	 	datum_alpha = (int32 *) DatumGetPointer(entryvec->vector[i].key);
	memcpy( (void*)union_dl, (void*)datum_l, KEYSIZE );
	spherekey_union_two(union_dl, datum_alpha );
	memcpy( (void*)union_dr, (void*)datum_r, KEYSIZE );
	spherekey_union_two(union_dr, datum_alpha );
	costvector[i - 1].cost = pgs_abs( (spherekey_size(union_dl) - size_l) - (spherekey_size(union_dr) - size_r) );
   }
   qsort((void *) costvector, maxoff, sizeof(SPLITCOST), comparecost);

   for (j = 0; j < maxoff; j++) {
	i = costvector[j].pos;

	if (i == seed_1) {
		*left++ = i;
		v->spl_nleft++;
		continue;
	} else if (i == seed_2) {
		*right++ = i;
		v->spl_nright++;
		continue;
	}

	 	datum_alpha = (int32 *) DatumGetPointer(entryvec->vector[i].key);
	memcpy( (void*)union_dl, (void*)datum_l, KEYSIZE );
	memcpy( (void*)union_dr, (void*)datum_r, KEYSIZE );

	spherekey_union_two(union_dl, datum_alpha );
	spherekey_union_two(union_dr, datum_alpha );
	
	size_alpha = spherekey_size( union_dl );
	size_beta  = spherekey_size( union_dr );

	/* pick which page to add it to */
	if (size_alpha - size_l < size_beta - size_r + WISH_F(v->spl_nleft, v->spl_nright, 1.0e-9) ) {
		memcpy( (void*)datum_l, (void*)union_dl, KEYSIZE );
		size_l = size_alpha;
		*left++ = i;
		v->spl_nleft++;
	} else {
		memcpy( (void*)datum_r, (void*)union_dr, KEYSIZE );
		size_r = size_beta;
		*right++ = i;
		v->spl_nright++;
	}
   }
   FREE(costvector);

   *right = *left = FirstOffsetNumber;

   datum_alpha = (int32*)MALLOC(KEYSIZE);
   memcpy( (void*)datum_alpha, (void*)datum_l, KEYSIZE ); 
   datum_beta  = (int32*)MALLOC(KEYSIZE); 
   memcpy( (void*)datum_beta, (void*)datum_r, KEYSIZE );

   v->spl_ldatum = PointerGetDatum(datum_alpha);
   v->spl_rdatum = PointerGetDatum(datum_beta);

   PG_RETURN_POINTER(v);
}

