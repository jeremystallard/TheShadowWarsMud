#PROG
vnum 401
code if hastarget $i
mob transfer monstrous 410
if carries $q 405
mob force monstrous emoteto $q {Wglares{w at $Q just before it attacks.
pause 10
mob force monstrous murder $q
else
mob force monstrous emote glances toward its nest before returning to flight.
mob transfer monstrous 468
mob transfer $q 409
endif
else
mob transfer $q 409
endif
mob forget
~
#END

