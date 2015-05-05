#! /usr/bin/perl -w

use Cwd;
$obj=shift;
$outpath=shift;
if( $outpath ) {
    unlink($outpath);
}
$cwd=cwd();
while(<>) {
    next unless m{^\#(line)?\s*[0-9]*\s+"(\w.*)"}; 
    next if m{vs.net}; 
    next if m{/usr/include};
    $_=$2; 
    s{\\\\}{/}g; 
    s{$cwd/}{}; 
    $h{$_}=1;
}
if( $outpath ) {
    open(STDOUT, ">$outpath") or die("open $outpath: $!");
}
print("$obj: \\\n",
      join(" \\\n", sort(keys(%h)), 
	   "makefile", "makefile.common", "makefile.win32"), 
      "\n\n");
