import sys

def parse(line):
    split = line.split(',')
    event = split[0]

    if event not in ['R', 'W', 'PCIR', 'PCIW']:
        return None, None, None, None

    size = int(split[4], 16)
    addr = int(split[5], 16)
    value = int(split[6], 16)

    if event == 'PCIR':
        event = 'R'
        addr += 0x88000
    elif event == 'PCIW':
        event = 'W'
        addr += 0x88000

    return event, addr, size, value

with open(sys.argv[1]) as file:
    mem = bytearray(int(sys.argv[2], 16))
    initialized = set()
    mmio = set()

    for line in file:
        event, addr, size, value = parse(line)
        if event == None:
            continue

        for i in range(size):
            byte = (value >> i * 8) & 0xFF
            if event == 'W' or addr + i not in initialized:
                mem[addr + i] = byte
                initialized.add(addr + i)
            elif mem[addr + i] != byte:
                mmio.add(addr)

    mem = bytearray(int(sys.argv[2], 16))
    initialized = set()
    readonly = set(mmio)

    file.seek(0)
    for line in file:
        event, addr, size, value = parse(line)
        if event == None or event == 'W' or addr not in mmio:
            continue

        for i in range(size):
            byte = (value >> i * 8) & 0xFF
            if addr + i not in initialized:
                mem[addr + i] = byte
                initialized.add(addr + i)
            elif mem[addr + i] != byte and addr in readonly:
                readonly.remove(addr)

    out = open(sys.argv[1] + '.seq', 'w')
    for addr in readonly:
        out.write('RO,{}\n'.format(hex(addr)))

    for addr in readonly:
        mmio.remove(addr)

    interrupt = False
    file.seek(0)
    for line in file:
        if 'INT' in line and not interrupt:
            interrupt = True
            out.write(line)
            continue

        event, addr, size, value = parse(line)
        if event == None or (addr not in mmio and addr != 0x10a1c0 and addr != 0x10a1c4):
            continue

        out.write("{},{},{}\n".format(event, hex(addr), hex(value)))