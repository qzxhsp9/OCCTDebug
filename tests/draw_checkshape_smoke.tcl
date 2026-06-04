# Minimal geometry smoke script. It creates an in-memory box and validates it.
pload ALL
box b 0 0 0 1 1 1
checkshape b
puts "DRAW_CHECKSHAPE_OK"
exit
