#!/usr/bin/perl
my (@AoA, @tmp);
@AoA = ( );
opendir(DIR, "/home/tsw/TSW/player/");
while (defined ($file = readdir(DIR))) {
   open(INPUT,"<", "/home/tsw/TSW/player/$file");
   while (<INPUT>) 
   {
      if (/CSpher/)
      {
	chomp;
	@spheres = split(/ /);
	$total = $spheres[2] + $spheres[3] + $spheres[4] + $spheres[5] + $spheres[6];
	if ($total > 0) 
        {
	   #print "$file Tot:$total Air:$spheres[2] Earth:$spheres[3] Fire:$spheres[4] Spirit:$spheres[5] Water:$spheres[6]\n";
	   @tmp = (  $total, $spheres[2], $spheres[3], $spheres[4], $spheres[5], $spheres[6], $file );   
#	   print "$tmp[0][0] - $tmp[0][1] - $tmp[0][6]\n";
	   push @AoA, [ @tmp ];
	}
      }
    }
  }	


#foreach $aref ( @AoA) {
#   print "[ @$aref ]\n";
#}

print "Sorting\n\n";
@sorted = sort {$a->[0] <=> $b->[0] } @AoA;
foreach $row (@sorted)
{
        #print "<TD>Tot:@$row[0]</TD><TD>Air:@$row[1]</TD><TD>Earth:@$row[2]</TD><TD>Fire:@$row[3]</TD><TD>Spirit:@$row[4]</TD><TD>Water:@$row[5]</TD><TD>@$row[6]</TD>\n";
	print "[ @$row ]\n";
}

#for $i ( 0 .. $#AoA ) {
# $aref = $AoA[$i];
# $n = @$aref - 1;
# for $j (0 .. $n ) {
#   print "elt '$i' '$j' is $AoA[$i][$j]\n";
# }
#}

