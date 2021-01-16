#PROG
vnum 13301
code if race $n Trolloc
say Shadowspawn!
mob kill $n
else
if name $n Kamaryn
say Lights Blessing $n
else
say Walk in the Light
endif
endif
~
#END

