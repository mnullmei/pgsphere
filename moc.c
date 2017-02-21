#include "pgs_healpix.h"
#include "pgs_moc.h"

PG_FUNCTION_INFO_V1(smoc_in);
PG_FUNCTION_INFO_V1(smoc_out);

static void
moc_error_out(const char *message, int type)
{
	ereport(ERROR, (errcode(ERRCODE_INTERNAL_ERROR),
			errmsg("MOC processing error: %s", message)));
}

static bool
index_invalid(hpint64 npix, long index)
{
	return index < 0 || index >= npix;
}

Datum
smoc_in(PG_FUNCTION_ARGS)
{
	char*	input_text = PG_GETARG_CSTRING(0);
	char	c;
	Smoc*	moc;
	void*	moc_context = create_moc_context(moc_error_out);
	int32	moc_size;

	long	order = -1;
	hpint64	npix = 0;
	hpint64	nb;
	hpint64	nb2;

	int ind = 0;
	do
	{
		nb = readNumber(input_text, &ind);
		c = readChar(input_text, &ind);

		if (c == '/') /* nb is a Healpix order */
		{
			if (nb == -1)
			{
				release_moc_context(moc_context, moc_error_out);
				ereport(ERROR, (errcode(ERRCODE_INVALID_TEXT_REPRESENTATION),
						errmsg("[c.%d] Incorrect MOC syntax: a Healpix level "
								"is expected before a / character!", ind - 1),
						errhint("Expected syntax: '{healpix_order}/"
								"{healpix_index}[,...] ...', where "
								"{healpix_order} is between 0 and 29. Example: "
								"'1/0 2/3,5-10'.")));
			}
			else if (order_invalid((int) nb))
			{
				release_moc_context(moc_context, moc_error_out);
				ereport(ERROR, (errcode(ERRCODE_NUMERIC_VALUE_OUT_OF_RANGE),
						errmsg("[c.%d] Incorrect Healpix order: %lld!", ind - 1,
																			nb),
						errhint("A valid Healpix order must be an integer "
								"between 0 and 29.")));
			}
			else
			{
				order = nb;
				npix = c_npix(order);
			}
		}
		else if (c == ',') /* nb is a Healpix index */
		{
			if (index_invalid(npix, nb))
			{
				release_moc_context(moc_context, moc_error_out);
				ereport(ERROR, (errcode(ERRCODE_NUMERIC_VALUE_OUT_OF_RANGE),
							errmsg("[c.%d] Incorrect Healpix index: %lld!",
																ind - 1, nb),
							errhint("At order %ld, a Healpix index must be "
									"an integer between 0 and %lld.", order,
									 								npix - 1)));
			}
			add_to_moc(moc_context, order, nb, nb + 1, moc_error_out);
		}
		else if (c == '-')  /* next Healpix number must follow */
		{
			nb2 = readNumber(input_text, &ind);
			if (nb2 == -1)
			{
				release_moc_context(moc_context, moc_error_out);
				ereport(ERROR, (errcode(ERRCODE_INVALID_TEXT_REPRESENTATION),
						errmsg("[c.%d] Incorrect MOC syntax: a second Healpix "
								"index is expected after a '-' character.",
								(ind - 1)),
						errhint("Expected syntax: 'MOC {healpix_order}/"
								"{healpix_index}[,...] ...', where "
								"{healpix_order} is between 0 and 29. Example: "
								"'1/0 2/3,5-10'.")));
			}

			c = readChar(input_text, &ind);
			if (isdigit(c))
			{
				ind--;
			}

			if (index_invalid(npix, nb2) || nb >= nb2)
			{
				release_moc_context(moc_context, moc_error_out);
				ereport(ERROR, (errcode(ERRCODE_NUMERIC_VALUE_OUT_OF_RANGE),
						errmsg("[c.%d] Incorrect Healpix range: %lld-%lld!",
															ind - 1 , nb, nb2),
						errhint("The first value of a range (here %lld) must be"
								" less than the second one (here %lld). At "
								"order %ld, a Healpix index must be an integer "
								"between 0 and 29.", nb, nb2, order)));

			}
			add_to_moc(moc_context, order, nb, nb2 + 1, moc_error_out);
		}
		else if (isdigit(c)) /* nb is the last Healpix index of this level */
		{
			if (index_invalid(npix, nb))
			{
				release_moc_context(moc_context, moc_error_out);
				ereport(ERROR, (errcode(ERRCODE_NUMERIC_VALUE_OUT_OF_RANGE),
							errmsg("[c.%d] Incorrect Healpix index: %lld!",
																ind - 1, nb),
							errhint("At order %ld, a Healpix index must be "
									"an integer between 0 and %lld.", order,
									 								npix - 1)));
			}
			ind--; /* Nothing else to do in this function */
			add_to_moc(moc_context, order, nb, nb + 1, moc_error_out);
		}
		else if (c == '\0') /* nb should be the last Healpix index */
		{
			if (order == -1)
			{
				release_moc_context(moc_context, moc_error_out);
				ereport(ERROR, (errcode(ERRCODE_INVALID_TEXT_REPRESENTATION),
				errmsg("[c.%d] Incorrect MOC syntax: empty string!", ind - 1),
				errhint("The minimal expected syntax is: '{healpix_order}/', "
						"where {healpix_order} must be an integer between 0 and"
						" 29. This will create an empty MOC. Example: '1/'.")));
			}
			else if (nb != -1 && index_invalid(npix, nb))
			{
				release_moc_context(moc_context, moc_error_out);
				ereport(ERROR, (errcode(ERRCODE_NUMERIC_VALUE_OUT_OF_RANGE),
							errmsg("[c.%d] Incorrect Healpix index: %lld!",
																ind - 1, nb),
							errhint("At order %ld, a Healpix index must be "
									"an integer between 0 and %lld.", order,
									 								npix - 1)));
			}
			else
			{
				add_to_moc(moc_context, order, nb, nb + 1, moc_error_out);
			}
		}
		else
		{
			release_moc_context(moc_context, moc_error_out);
			ereport(ERROR, (errcode(ERRCODE_INVALID_TEXT_REPRESENTATION),
					errmsg("[c.%d] Incorrect MOC syntax: unsupported character:"
							" '%c'!", ind - 1, c),
					errhint("Expected syntax: 'MOC {healpix_order}/"
							"{healpix_index}[,...] ...', where {healpix_order} "
							"is between 0 and 29. Example: '1/0 2/3,5-10'.")));
		}
	}
	while (c != '\0');

	moc_size = get_moc_size(moc_context, moc_error_out);
	moc = (Smoc*) palloc(moc_size);
	memset(moc, 0, MOC_HEADER_SIZE);
	SET_VARSIZE(moc, moc_size);

	if (create_moc_release_context(moc_context, moc, moc_error_out))
	{
		PG_RETURN_POINTER(moc);
	}
	else
	{
		ereport(ERROR, (errcode(ERRCODE_INTERNAL_ERROR),
				errmsg("Internal error in creation of MOC from text input.")));
		PG_RETURN_NULL();
	}
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
long readNumber(const char* mocAscii, int* start)
{
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
  while((wordLength < 0 || length < wordLength)
					&& mocAscii[*start] != '\0' && !isspace(mocAscii[*start]))
    word[length++] = mocAscii[(*start)++];

  // Finish the read word:
  word[length] = '\0';
}

/**
 * [readChar(char*, int*) --> char]
 * 
 * Read the next non-whitespace character.
 * 
 * All whitespaces from the given position are silently skipped.
 * 
 * The given index is incremented each time a character (whatever it is) is
 * skipped.
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
char readChar(const char* mocAscii, int* start)
{
  // Skip all space characters:
  while(mocAscii[*start] != '\0' && isspace(mocAscii[*start]))
    (*start)++;
  
  // Return the read character:
  if (mocAscii[*start] == '\0')
    return '\0';
  else
    return mocAscii[(*start)++];
}

Datum
smoc_out(PG_FUNCTION_ARGS)
{
	char	*buf = (char *) palloc(20);
	sprintf(buf, "[a MOC]");
	PG_RETURN_CSTRING(buf);
}
