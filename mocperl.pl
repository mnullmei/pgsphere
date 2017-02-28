CREATE OR REPLACE FUNCTION mocd(bytea) RETURNS text AS $$

	(my $in) = @_;
	my $len = (length($in) - 2) / 2;
	($var_len, $version, $order, $depth, $first, $last, $area, $tree_begin,
					$data) = unpack("LSCCQQQLC*", pack("H*", substr($_[0], 2)));
	return sprintf( "len = %d, var_len = %d, version = %u, order = %u, " .
					"depth = %u, first = %llu, last = %llu, area = %llu, " .
					"tree_begin = %u",
					$len, $var_len, $version, $order, $depth,
					$first, $last, $area, $tree_begin);
$$ LANGUAGE plperlu;


-- # typedef struct
-- # {
-- # 	char		vl_len_[4];	/* size of PostgreSQL variable-length data */
-- # 	uint16		version;	/* version of the 'toasty' MOC data structure */
-- # 	uint8		order;		/* actual MOC order */
-- # 	uint8		depth;		/* depth of B+-tree */
-- # 	hpint64		first;		/* first Healpix index in set */
-- # 	hpint64		last;		/* 1 + (last Healpix index in set) */
-- # 	hpint64		area;		/* number of covered Healpix cells */
-- # 	int32		tree_begin;	/* start of B+ tree, past the options block */
-- # 	int32		data[1];	/* no need to optimise for empty MOCs */
-- # } Smoc;
