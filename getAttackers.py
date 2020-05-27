#!/usr/bin/env python3
import random
import sys

total = sys.argv[1] 
l = list(range(int(total)))
random.shuffle(l)
for i in l:
	print (str(i))
