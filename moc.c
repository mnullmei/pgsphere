#include "moc.h"


PG_FUNCTION_INFO_V1(smoc_in);
PG_FUNCTION_INFO_V1(smoc_out);

Datum
moc_in(PG_FUNCTION_ARGS)
{
	char*	input_text = PG_GETARG_CSTRING(0);
	char	c;
	Smoc*	moc;
	int32	moc_size;
	hpint64	area; /* number of covered Healpix cells */

int ind = 0, success;
long nb, nb2, order = -1;
char prefix[5];
long nbcells = nbCells(maxOrder);

	moc_size = MOC_HEADER_SIZE + ...;
	moc = palloc(moc_size);
	memset(moc, 0, MOC_HEADER_SIZE);
	SET_VARSIZE(moc, MOC_FIX_SIZE+nbcells*sizeof(byte));
	moc->version	= 0;
	moc->order		= ...;
	moc->first		= ...;	/* first Healpix index in set */
	moc->start		= ...;	/* 1 + (last Healpix index in set) */
	moc->area		= ...;
	moc->end		= ...;	/* 1 + (offset of last interval) */

	do
	{
		nb = readNumber(input_text, &ind);
		c = readChar(input_text, &ind);

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
		nb2 = readNumber(input_text, &ind);
		if (nb2 == -1){
		  ereport(ERROR,
        		  (errcode(ERRCODE_INVALID_TEXT_REPRESENTATION),
        		   errmsg("[c.%d] Incorrect MOC's STC-s syntax: a second Healpix index is expected after a - character!", (ind-1)),
        		   errhint("Expected syntax: 'MOC {healpix_order}/{healpix_index}[,...] ...', where {healpix_order} is between 0 and %d. Example: '1/0 2/3,5-10'.", MOC_MAX_ORDER))
		  );
		  PG_RETURN_NULL();
		}
		c = readChar(input_text, &ind);
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
	}
	while(c != '\0');

	PG_RETURN_POINTER(moc);
}

/**
 * [readNumber(char*, int*) --> long]
 * 
 * Read the next character as a number.
 * 
 * All whitespaces from the given position to the first digit are silently
 * skiped.
 * 
 * If the first non-whitespace character is not a digit, this function returns
 * immediately -1. Otherwise, all successive digits are read and gather to build
 * the corresponding number.
 * 
 * The given index is incremented each time a character (whatever it is) is
 * skiped.
 * 
 * @param mocAscii  The string in which the number must be read.
 * @param start     Index from which the number must be read.
 *                  This value is incremented at each read character.
 * 
 * @return  The read number,
 *          or -1 if no digit has been found.
 */
long readNumber(const char* mocAscii, int* start){
  long nb = -1;
  
  // Skip all space characters:
  while(mocAscii[*start] != '\0' && isspace(mocAscii[*start]))
    (*start)++;
  
  /* If the current character is a digit,
   * read all other possible successive digits: */
  if (mocAscii[*start] != '\0' && isdigit(mocAscii[*start])){
    nb = 0;
    do{
      nb = nb*10 + (long)(mocAscii[*start]-48);
      (*start)++;
    }while(isdigit(mocAscii[*start]) && mocAscii[*start] != '\0');
  }
  
  // Return the read number:
  return nb;
}

/**
 * [readWord(char*, int*, char*, int)]
 * 
 * Read the next word (string of non-whitespace characters).
 * 
 * All whitespaces from the given position and before the first non-whitespace
 * character are silently skiped.
 * 
 * The given index is incremented each time a character (whatever it is) is
 * skiped.
 * 
 * When the end of the given string is reached, this function stops immediately
 * and returns '\0'. The given index is then set to the corresponding position.
 * 
 * @param mocAscii    The string in which the next word must be read.
 * @param start       Index from which the word must be read.
 *                    This value is incremented at each read character.
 * @param word        The read word, or '\0' if the end of the given string is
 *                    empty.
 * @param wordLength  Maximum number of characters to read.
 *                    If negative, characters will be read until the end of the
 *                    string or a whitespace is encountered.
 */
void readWord(const char* mocAscii, int* start, char* word, int wordLength) {
  int length = 0;
  word[0] = '\0';
  
  // Skip all space characters:
  while(mocAscii[*start] != '\0' && isspace(mocAscii[*start]))
    (*start)++;
  
  // Read all characters until the end or a whitespace is reached:
  while((wordLength < 0 || length < wordLength) && mocAscii[*start] != '\0' && !isspace(mocAscii[*start]))
    word[length++] = mocAscii[(*start)++];

  // Finish the read word:
  word[length] = '\0';
}

/**
 * [readChar(char*, int*) --> char]
 * 
 * Read the next non-whitespace character.
 * 
 * All whitespaces from the given position are silently skiped.
 * 
 * The given index is incremented each time a character (whatever it is) is
 * skiped.
 * 
 * When the end of the given string is reached, this function stops immediately
 * and returns '\0'. The given index is then set to the corresponding position.
 * 
 * @param mocAscii  The string in which the next non-whitespace character must
 *                  be read.
 * @param start     Index from which the character must be read.
 *                  This value is incremented at each read character.
 * 
 * @return  The read non-whitespace character,
 *          or '\0' if the end has been reached.
 */
char readChar(const char* mocAscii, int* start){
  // Skip all space characters:
  while(mocAscii[*start] != '\0' && isspace(mocAscii[*start]))
    (*start)++;
  
  // Return the read character:
  if (mocAscii[*start] == '\0')
    return '\0';
  else
    return mocAscii[(*start)++];
}

