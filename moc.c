#include "moc.h"


PG_FUNCTION_INFO_V1(smoc_in);
PG_FUNCTION_INFO_V1(smoc_out);

Datum parse_moc(PG_FUNCTION_ARGS){
  char* mocAscii = PG_GETARG_CSTRING(0);
  
  // Try to identify the maximum Healpix order:
  int maxOrder = fetchMaxOrder(mocAscii);
  
  // If none has been found, return the empty Moc:
  if (maxOrder == -1){
    ereport(ERROR,
            (errcode(ERRCODE_INVALID_TEXT_REPRESENTATION),
             errmsg("Incorrect MOC's STC-s syntax!"),
             errhint("Expected syntax: 'MOC {healpix_order}/{healpix_index}[,...] ...', where {healpix_order} is between 0 and 29. Example: '1/0 2/3,5-10'."))
    );
    PG_RETURN_NULL();
  }
  // If the Healpix order is incorrect (not between 0 and MOC_MAX_ORDER) print an error
  // and return an empty Moc:
  else if (maxOrder < 0 || maxOrder > MOC_MAX_ORDER){
    ereport(ERROR,
            (errcode(ERRCODE_NUMERIC_VALUE_OUT_OF_RANGE),
             errmsg("Incorrect maximum Healpix order: %d!", maxOrder),
             errhint("A valid Healpix order must be an integer between 0 and %d.", MOC_MAX_ORDER))
    );
    PG_RETURN_NULL();
  }
  // Otherwise, let's build the MOC:
  else{
    // create an empty Moc:
    Moc* moc;
    int ind = 0, success;
    long nb, nb2, order = -1;
    char c;
	char prefix[5];
    long nbcells = nbCells(maxOrder);

    // allocate the required amount of memory:
	moc = palloc(MOC_FIX_SIZE+nbcells*sizeof(byte));
    moc->size  = 0;
    moc->order = maxOrder;
    moc->maxIndex = (12*pow(4, maxOrder))-1;
    //moc->cells = ((byte*)moc) + MOC_FIX_SIZE - sizeof(byte*);
    memset(&(moc->cells), (byte)0, nbcells);
    
    // set the variable size information:
    SET_VARSIZE(moc, MOC_FIX_SIZE+nbcells*sizeof(byte));

	// read the STC-s prefix:
	readWord(mocAscii, &ind, prefix, 5);
	if (strcmp(prefix, "MOC") != 0){
		ereport(ERROR,
				(errcode(ERRCODE_INVALID_TEXT_REPRESENTATION),
				 errmsg("Incorrect STC-s prefix for a MOC: '%s'!", prefix),
				 errhint("Expected prefix: 'MOC '."))
		);
		PG_RETURN_NULL();
	}
    
    // then, read a last time the STC-s MOC in order to set the specified cells:
    do{
      nb = readNumber(mocAscii, &ind);
      c = readChar(mocAscii, &ind);
      
      // CASE: nb is an Healpix order
      if (c == '/'){
        if (nb == -1){
          ereport(ERROR,
                  (errcode(ERRCODE_INVALID_TEXT_REPRESENTATION),
                   errmsg("[c.%d] Incorrect MOC's STC-s syntax: an Healpix level is expected before a / character!", (ind-1)),
                   errhint("Expected syntax: 'MOC {healpix_order}/{healpix_index}[,...] ...', where {healpix_order} is between 0 and %d. Example: '1/0 2/3,5-10'.", MOC_MAX_ORDER))
          );
          PG_RETURN_NULL();
        }else if (nb < 0 || nb > MOC_MAX_ORDER){
          ereport(ERROR,
                  (errcode(ERRCODE_NUMERIC_VALUE_OUT_OF_RANGE),
                   errmsg("[c.%d] Incorrect Healpix order: %ld!", (ind-1), nb),
                   errhint("A valid Healpix order must be an integer between 0 and %d.", MOC_MAX_ORDER))
          );
          PG_RETURN_NULL();
        }else
          order = nb;
      }
      // CASE: nb is an Healpix index
      else if (c == ','){
        success = (order == maxOrder) ? setCell(moc, nb) : setUpperCell(moc, order, nb);
        if (!success){
          ereport(ERROR,
                  (errcode(ERRCODE_NUMERIC_VALUE_OUT_OF_RANGE),
                   errmsg("[c.%d] Incorrect Healpix index: %ld!", (ind-1), nb),
                   errhint("At order %ld, an Healpix index must be an integer between 0 and %d.", order, moc->maxIndex >> 2*(moc->order-order)))
          );
          PG_RETURN_NULL();
        }
      }
      else if (c == '-'){
        nb2 = readNumber(mocAscii, &ind);
        if (nb2 == -1){
          ereport(ERROR,
                  (errcode(ERRCODE_INVALID_TEXT_REPRESENTATION),
                   errmsg("[c.%d] Incorrect MOC's STC-s syntax: a second Healpix index is expected after a - character!", (ind-1)),
                   errhint("Expected syntax: 'MOC {healpix_order}/{healpix_index}[,...] ...', where {healpix_order} is between 0 and %d. Example: '1/0 2/3,5-10'.", MOC_MAX_ORDER))
          );
          PG_RETURN_NULL();
        }
        c = readChar(mocAscii, &ind);
        if (isdigit(c))
          ind--;
        success = setCellRange(moc, nb << 2*(maxOrder-order), ((nb2+1) << 2*(maxOrder-order))-1);
        if (!success){
          ereport(ERROR,
                  (errcode(ERRCODE_NUMERIC_VALUE_OUT_OF_RANGE),
                   errmsg("[c.%d] Incorrect Healpix range: %ld-%ld!", (ind-1), nb, nb2),
                   errhint("The first value of a range (here %ld) MUST be less than the second one (here %ld). Besides, at order %ld, an Healpix index must be an integer between 0 and %d.", nb, nb2, order, moc->maxIndex >> 2*(moc->order-order)))
          );
          PG_RETURN_NULL();
        }
      }
      // CASE: nb is the last Healpix index of this Healpix level
      else if (isdigit(c)){
        success = (order == maxOrder) ? setCell(moc, nb) : setUpperCell(moc, order, nb);
        if (!success){
          ereport(ERROR,
                  (errcode(ERRCODE_NUMERIC_VALUE_OUT_OF_RANGE),
                   errmsg("[c.%d] Incorrect Healpix index: %ld!", (ind-1), nb),
                   errhint("At order %ld, an Healpix index must be an integer between 0 and %d.", order, moc->maxIndex >> 2*(moc->order-order)))
          );
          PG_RETURN_NULL();
        }
        ind--; // Nothing else to do in this function!
      }
      // CASE: nb should be the last Healpix index
      else if (c == '\0'){
        if (order == -1){
          ereport(ERROR,
                  (errcode(ERRCODE_INVALID_TEXT_REPRESENTATION),
                   errmsg("[c.%d] Incorrect MOC's STC-s syntax: empty string!", (ind-1)),
                   errhint("The minimal expected syntax is: '{healpix_order}/', where {healpix_order} must be an integer between 0 and %d. This will create an empty MOC at the specified order. Example: '1/'.", MOC_MAX_ORDER))
          );
          PG_RETURN_NULL();
        }else if (nb != -1){
          success = (order == maxOrder) ? setCell(moc, nb) : setUpperCell(moc, order, nb);
          if (!success){
              ereport(ERROR,
                    (errcode(ERRCODE_NUMERIC_VALUE_OUT_OF_RANGE),
                     errmsg("[c.%d] Incorrect Healpix index: %ld!", (ind-1), nb),
                     errhint("At order %ld, an Healpix index must be an integer between 0 and %d.", order, moc->maxIndex >> 2*(moc->order-order)))
            );
            PG_RETURN_NULL();
          }
        }
      }
      else{
        ereport(ERROR,
                (errcode(ERRCODE_INVALID_TEXT_REPRESENTATION),
                 errmsg("[c.%d] Incorrect MOC's STC-s syntax: unsupported character: '%c'!", (ind-1), c),
                 errhint("Expected syntax: 'MOC {healpix_order}/{healpix_index}[,...] ...', where {healpix_order} is between 0 and %d. Example: '1/0 2/3,5-10'.", MOC_MAX_ORDER))
        );
        PG_RETURN_NULL();
      }
    }while(c != '\0');
    
    PG_RETURN_POINTER(moc);
  }
}


