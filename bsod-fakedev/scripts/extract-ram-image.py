import sys

mem = bytearray(int(sys.argv[2], 16))
initialized = set()

with open(sys.argv[1]) as file:
    for line in file:
        split = line.split(',')
        event = split[0]

        if event not in ['R', 'PCIR']:
            continue

        size = int(split[4], 16)
        addr = int(split[5], 16)
        value = int(split[6], 16)

        if event == 'PCIR':
            event = 'R'
            addr += 0x88000

        for i in range(size):
            byte = (value >> i * 8) & 0xFF
            if addr + i not in initialized:
                mem[addr + i] = byte
                initialized.add(addr + i)

with open(sys.argv[1] + '.ram', 'wb') as file:
    file.write(mem)
