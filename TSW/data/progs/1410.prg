#PROG
vnum 1410
code if ispc $n
    mob echo $I tears up $O...
    mob junk slip
    mob echo $I says, "{GThank you very much for helping me, $n. Here is your reward...{x"
    mob oload 1403 0 room
    mob echo $I rumages through $l pockets and places some coins upon the shop counter.
    mob force $n get coins
endif
~
#END

