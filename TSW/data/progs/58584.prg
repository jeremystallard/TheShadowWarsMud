#PROG
vnum -6952
code if objhere figure
emote snarls a little and gives $n one of his toys in a begrudged manner, '{GHere, take this. Just 'cause you said the magic word.{x'
give figure $n
else
emote shakes his head, showing $n his empty hands, '{GSorry pal, but that's all I had.{x'
if objhere token
emote says suddenly, '{GWait!{x' He stuffs his hands inside his pockets, searching for something. Taking a small object from one of the pockets, he hands it out to $n, '{GThere you go. I don't know what that is, it isn't even fun enough to play with.{x'
give token $n
endif
endif
~
#END

