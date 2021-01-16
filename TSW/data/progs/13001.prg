#PROG
vnum 13001
code if rand 75
mob pause 100
mob echo A rickety wagon suddenly barrels toward you, just barely missing.
else
if rand 50
mob pause 100
mob echo A rickety wagon suddenly barrels toward you, {Yclipping you{x as it passes
mob damage $n 50 250
else
if rand 25
mob pause 100
mob echo A rickety wagon suddenly barrels toward you, {Rstriking you{x as it passes.
mob damage $n 250 500
endif
endif
endif
~
#END

