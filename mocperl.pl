CREATE OR REPLACE FUNCTION mocd(bytea) RETURNS text AS $$

	($in) = @_;
	$len = (length($in) - 2) / 2;
	($version, $order, $depth, $first, $last, $area, $tree_begin,
					 $data) = unpack("SCCQQQLa*", pack("H*", substr($in, 2)));

	($dstr) = unpack("Z*", $data);
	$len_dstr = length($dstr);

	$tree_begin_32 = $tree_begin - 32;
	$gap = unpack("H*", substr($data, $len_dstr, $tree_begin_32 - $len_dstr));

	$tree = substr($data, $tree_begin_32 - $len_dstr);
	$level_ends_size = 4 * $depth;
	@level_ends = unpack("L" . $lv, substr($tree, $level_ends_size));

	$pages_start = $tree_begin + $level_ends_size;

#	for ($x in 
#@level_ends = (2204, 1103, 1234); $depth = 3;

	return sprintf( "len = %d, version = %u, order = %u, " .
					"depth = %u, first = %llu, last = %llu, area = %llu, " .
					"tree_begin = %u\n%s\n%s\nlevel_ends: ",
					$len, $version, $order, $depth,
					$first, $last, $area, $tree_begin,
					$dstr, $gap)
				. sprintf("%d " x $depth, @level_ends);
$$ LANGUAGE plperlu;

-- # "SCCQQQL" =^= 32 bytes.

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
