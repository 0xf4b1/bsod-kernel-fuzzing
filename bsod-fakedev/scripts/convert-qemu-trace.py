import sys

def replace_multiple(str, old, new):
    for c in old:
        str = str.replace(c, new)
    return str

regions = {}
time_intr_host = 0

with open(sys.argv[1]) as file:
    for line in file:
        if line.startswith(' '):
            continue

        first, second = line[:-1].split(' ', 1)
        first = replace_multiple(first, ['@', ':'], ',')
        try:
            pid, time, type = first.split(',')
        except:
            print(first)
            continue

        time = int(time.replace('.', ''))
        if type in ["vfio_intx_interrupt", "vfio_msi_interrupt"]:
            diff = time - time_intr_host
            time_intr_host = 0
            regions['region0'].write('INT,{}\n'.format(diff))
            continue

        elif type == ['vfio_dma_add_watchlist', 'vfio_dma_changed']:
            second = second.split(' ')
            page = second[1]
            size = second[3]
            regions[region].write('{},{},{}\n'.format(type, page, size))
            continue

        elif type in ["vfio_pci_read_config", "vfio_pci_write_config"]:
            second = replace_multiple(second, [',', '(', ')', '@'], '')
            second = replace_multiple(second, ['='], ' ').split(' ')

            region = 'region0'
            addr = second[2]

            if type == "vfio_pci_read_config":
                type = 'PCIR'
                size = second[4]
                value = second[5] # panda needs prefix '0x'
            elif type == "vfio_pci_write_config":
                type = 'PCIW'
                size = second[5]
                value = second[3]

        elif type in ["vfio_region_read", "vfio_region_write"]:

            second = replace_multiple(second, [' ', '(', ')'], '')
            second = replace_multiple(second, ['+', '=', ':', '.'], ',').split(',')

            region = second[4]
            addr = second[5]

            # The PCI config space is mapped in MMIO space at 0x88000-0x88fff.
            # Discard read/write events there, since they are caught by
            # vfio_pci_read_config and vfio_pci_write_config events already.
            if region == 'region0' and 0x88000 <= int(addr, 16) <= 0x88fff:
                continue

            if type == "vfio_region_read":
                type = 'R'
                size = second[6]
                value = second[7]

            elif type == "vfio_region_write":
                type = 'W'
                size = second[7]
                value = second[6]

        else:
            continue

        if region not in regions:
            regions[region] = open(region, 'w')

        regions[region].write("{},{},{},{},{},{},{}\n".format(type, time, 0, pid, size, addr, value))
        time_intr_host = time
