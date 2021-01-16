#!/usr/bin/perl
my (@AoA, @tmp);
@AoA = ( );
opendir(DIR, "/home/tsw/TSW/player/");
while (defined ($file = readdir(DIR))) {
   open(INPUT,"<", "/home/tsw/TSW/player/$file");
   while (<INPUT>) 
   {
      if (/'duelling'/)
      {
	chomp;
	@line = split(/ /);
	if ($line[1] > 0) 
        {
	   @tmp = (  $file, $line[1] );   
	   push @AoA, [ @tmp ];
	}
      }
    }
  }	


print "Sorting\n\n";
@sorted = sort {$a->[1] <=> $b->[1] } @AoA;
foreach $row (@sorted)
{
        #print "<TD>Tot:@$row[0]</TD><TD>Air:@$row[1]</TD><TD>Earth:@$row[2]</TD><TD>Fire:@$row[3]</TD><TD>Spirit:@$row[4]</TD><TD>Water:@$row[5]</TD><TD>@$row[6]</TD>\n";
	print "[ @$row ]\n";
}

