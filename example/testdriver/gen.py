import sys
from random import getrandbits, randrange, choice

count = 1
depth = int(sys.argv[1])
indent = 4
var = "input"

NOP = "a = {};"
CRASH = 'printf("crash {} %lx\\n", *(char *)0x0);'


def gen(count, bits):
    if count >= depth:
        if randrange(20000) == 1:
            print(" " * indent * count + CRASH.format(getrandbits(32)))
        else:
            print(" " * indent * count + NOP.format(getrandbits(32)))

        return

    current = choice(bits)
    bits.remove(current)
    print(
        " " * indent * count
        + "if ((({} >> {}) & 1) == {}) {{".format(var, current, randrange(2))
    )
    gen(count + 1, list(bits))
    print(" " * indent * count + "} else {")
    gen(count + 1, list(bits))
    print(" " * indent * count + "}")


print("void generated(unsigned int input) {")
gen(count, list(range(32)))
print("}")
