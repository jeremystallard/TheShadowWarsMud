#PROG
vnum 31267
code sleep 10
mob echoat $n The Guards head spins around a few times as he moves to block the door, "{RYou will never enter the haunting place without the favor of the Dark Lord{x."
sleep 5
mob echoat $n The guards head then spins around again as he begins to laugh at you.
sleep 30
mob force $n give tokenfavor guard
if carries $i 31268
mob echoat $n The guard peers down at you as he sees you have the favor you have recieved and takes it from you as payment.  "{RYou have earned his favor, I guess you can enter.{x"
mob junk all
mob transfer $n 31009
else
mob echoat $n The Guards head stops breifly, and points at you, "{RYou will never find it puny mortal{x!"
mob oload 31229
~
#END

