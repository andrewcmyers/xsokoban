while (<STDIN>) {
	if ($_ =~ m/^"40 40/) {
		$x = $_;
		$x =~ s/40 40/20 20/;
		print $x;
	} else { print $_; }
	if ($_ =~ /pixels/) { last; }
}

$line = 1;

while (<STDIN>) {
	if ($_ =~ /;$/) {
		print $_;
		exit;
	}
	$line++;
	if ($line % 2) {
		print '"';
		for ($i = 1; $i < 80; $i += 4) {
			print substr($_, $i, 2);
		}
		print substr($_, 81, 2)."\n";
	}
}
