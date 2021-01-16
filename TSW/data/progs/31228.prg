#PROG
vnum 31228
code sleep 5
emote grins wickedly, "So you want to test fate do you."
sleep 5
mob force $n give pumpkinkey gatekeeper
sleep 5
if carries $i 31216
emote nods slowly, "You are prepared, so please keep your hands and feet inside the vehicle at all times.  Well, unless you want to loose them." then lets out a mean cackle.
mob transfer $n 31252
mob junk all
else
emote shakes his head, "You have not prepared yourself. You do not have the key to make it through."
sleep 5
mob transfer $n 31200
mob junk all
~
#END

