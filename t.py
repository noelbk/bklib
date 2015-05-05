from random import randint

rand_max = 100
trials   = 100

hits = [0] * (rand_max+1)
collisions = 0
for i in xrange(trials):
    x = randint(0,rand_max)
    hits[x] += 1
    if hits[x] > 1:
        collisions += 1
    else:
        print("rand #%d = %d" % (i, x))

print("collisions = %s" % collisions)

for i in xrange(len(hits)):
    if hits[i] <= 1: continue
    print("hits[%d]=%d" % (i, hits[i]))

