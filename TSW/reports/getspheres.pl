#!/usr/bin/perl
print "<HTML><HEAD><TITLE>Highest Sphere Totals</TITLE></HEAD><BODY>";
print "<TABLE>";
while (<>) {
	chomp;
	@spheres = split(/ /);
	$total = @spheres[2] + @spheres[3] + @spheres[4] + @spheres[5] + @spheres[6];
	if ($total > 0) 
        {
	   print "<TR>";
	   print "<TD>Tot:$total</TD><TD>Air:@spheres[2]</TD><TD>Earth:@spheres[3]</TD><TD>Fire:@spheres[4]</TD><TD>Spirit:@spheres[5]</TD><TD>Water:@spheres[6]</TD><TD>@spheres[0]</TD>";
	   print "</TR>\n";
	}
	
}
print "</TABLE></BODY></HTML>";
