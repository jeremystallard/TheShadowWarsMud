#PROG
vnum 31268
code sleep 5
mob echoat $n {RF{Yl{ram{Ye{Rs{x leap through the empty {cair{x in front of you forming a {Dd{re{Dmon{ri{Dc{x looking face with two glowing {Re{ry{Res{x.
sleep 5
mob force $n drop paperquest
get paperquest
sleep 5
if carries $i 31229
mob junk all
mob echoat $n The voice of the dark lord rings in your head, "{RYou shall recieve favor for your choice.  Make sure to use it wisely.{x"
mob oload 31268
give token $n
else
emote laughs at you, "{RYou are not ready for Dark Lords Favor, Go away.{x"
~
#END

