CREATE OR REPLACE FUNCTION mocd(bytea) RETURNS text AS $$

	($in) = @_;
	$moc = pack("H*", substr($in, 2));
	$len = length($moc);
	
	($version, $order, $depth, $first_moc, $last_moc, $area, $tree_begin,
					 $data) = unpack("SCCQQQLa*", $moc);

	($dstr) = unpack("Z*", $data);
	$len_dstr = length($dstr);

	$tree_begin_32 = $tree_begin - 32;
	$gap = unpack("H*", substr($data, $len_dstr, $tree_begin_32 - $len_dstr));

	$tree = substr($data, $tree_begin_32);
	$tree_hex = unpack("H*", $tree);

	$level_ends_size = 4 * $depth;
	@level_ends = unpack("L" . $depth, substr($tree, 0, $level_ends_size));

	$pages_start = $tree_begin + $level_ends_size;
	
	$node_size = 12;
	$last_end = $pages_start;
	$out_str = "\n";

	for ($i = $0; $i < $depth; ++$i)
	{
		$out_str .= sprintf("%u:: ", $i);
		for ($j = $last_end; $j < $level_ends[$i]; $j += $node_size)
		{
			# insert page bump code here
			$node = substr($moc, $j, $node_size);
			($subnode, $start) = unpack("LQ", $node);
			$out_str .= sprintf("%u:{%llu -> %u} ", $j, $start, $subnode);
		}
		$out_str .= "\n";
		$last_end = $level_ends[$i];
	}
	# must read the following from the root node:
	($interval_begin) = unpack("L",
							substr($moc, $tree_begin + $level_ends_size, 4));

	$interval_size = 16;
	for ($j = $interval_begin; $j < $len; $j += $interval_size)
	{
		$interval = substr($moc, $j, $interval_size);
		($first, $second) = unpack("QQ", $interval);
		$out_str .= sprintf("%u:[%llu, %u) ", $j, $first, $second);
	}

#$gap=""; # $tree_hex="";

#@level_ends = (2204, 1103, 1234); $depth = 3;

	return sprintf( "len = %d, version = %u, order = %u, " .
					"depth = %u, first = %llu, last = %llu, area = %llu, " .
					"tree_begin = %u\n%s\n%s\n%s\npages_start = %u, " .
					"level_ends: ",
					$len, $version, $order, $depth,
					$first_moc, $last_moc, $area, $tree_begin,
					$dstr, $gap, $tree_hex,
					$pages_start)
				. sprintf("%d " x $depth, @level_ends) . $out_str;
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
