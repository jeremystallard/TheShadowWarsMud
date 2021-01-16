#PROG
vnum 16008
code mob echo $I immediately pounces on $O, almost falling off of the shelf...
mob echo $I clumsily attacks $O again, knocking one of the stoneware jars off of the shelf...
mob junk all
if objhere 16045
else
mob oload 16045 25 R
endif
if objhere 16036
else
mob oload 16036 25 R
endif
mob force $n look
~
#END

