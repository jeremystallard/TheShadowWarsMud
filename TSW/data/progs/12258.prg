#PROG
vnum 12258
code mob echoat $n The {cf{Cl{xuid that oozes from the skin of the {rst{Rrawber{rry {bpo{Bis{bon{x arrow {Bf{br{ro{Rg{x seeps into your body, killing you almost instantly.
if room $n == 12533
mob damage $n 10000 10000 slay
mob echo $n slowly sinks to the ground.
endif
 
 
~
#END

