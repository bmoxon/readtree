#!/usr/bin/perl
# benchsum.pl

# extract rto bench run results and put into a summary CSV file

if ($#ARGV < 0)
{
    print "Usage: benchsum.pl <scenario>\n";
    print " where <scenario> is a scenario name in the results dir\n";
    die;
}

$scen=@ARGV[0];
$scendir="results/" . $scen;
#print "$scendir\n";

opendir(SCENDIR, $scendir);
open(CSVOUT, ">$scendir/summary.csv");
printf (CSVOUT "test, target, nthreads, nobjs, nbytes, cumthreadt, elapsedt, minlat, maxlat, avglat, gt2mslat\n");

foreach my $run ( readdir(SCENDIR) ) {
  next if ($run eq ".." || $run eq "." || $run eq "summary.csv");
  #print ".$run\n";
  my (@fn) = split('\.', $run);
  $test = $fn[0];
  $target = $fn[1];
  $threads = $fn[2];
  #print "test $test, target $target, threads $threads\n";

  $nObjs = 0;
  $nBytes = 0;
  $cttime = 0;
  $etime = 0;
  $minlat = 0;
  $maxlat = 0;
  $avglat = 0;
  $gt2mslat = 0;

  open(FILE, "$scendir/$run");
  foreach my $line ( <FILE> ) {
    if ( $line =~ /^\w+tree: nFiles = (\d+), nBytes =(\d+)/ )
    {
      $nObjs = $1;
      $nBytes = $2;
    }
    if ( $line =~ /cumulative procTime =\s*([-+]?[0-9]*\.?[0-9]*)/ ) {
      $cttime = $1;
    }
    if ( $line =~ /elapsed time\(\):\s*([-+]?[0-9]*\.?[0-9]*)/ ) {
      $etime = $1;
    }
    if ( $line =~ /min latency =\s*([-+]?[0-9]*\.?[0-9]*)/ ) {
      $minlat = $1;
    }
    if ( $line =~ /max latency =\s*([-+]?[0-9]*\.?[0-9]*)/ ) {
      $maxlat = $1;
    }
    if ( $line =~ /avg latency =\s*([-+]?[0-9]*\.?[0-9]*)/ ) {
      $avglat = $1;
    }
    if ( $line =~ /2ms\+ latency count =\s*([-+]?[0-9]*\.?[0-9]*)/ ) {
      $gt2mslat = $1;
    }
  }
  close(FILE);

  printf (CSVOUT "%s, %s, %d", $test, $target, $threads);
  printf (CSVOUT ", %d, %d", $nObjs, $nBytes);
  printf (CSVOUT ", %.3f, %.3f", $cttime, $etime);
  printf (CSVOUT ", %d, %d, %d, %d", $minlat, $maxlat, $avglat, $gt2mslat);
  printf (CSVOUT "\n");
}

close (CSVOUT);

