#PROG
vnum 16005
code Mob echoat $n $I carefully examine $O that you have given $k and smiles, "Thank you very much..."
Mob echoaround $n $I carefully examine $O that $N has given $k and smiles, "Thank you very much..."
if carries $i 16042
Mob echo says, "It must be my lucky day for I have a new hat and a scarf! I have lost count of how many hats and scarves people keep on giving to me..."
mob junk all
mob oload 16044
give yarn $n
Mob echoat $n $I says to you, "You'd better run off now, before I change my mind and take that back."
Mob echoaround $n $I says to $N, "You'd better run off now, before I change my mind and take that back."
else
mob echo $I goes back to ignoring you whilst $j tinkers around with the contraption.
endif
~
#END

