#!/usr/local/bin/python3

files = ['01_fresh', '02_base', '03_heavy', '04_massive', '05_userspace_barrier', '06_userspace']
raw = {}

def compare_and_print(end, start):
    bname = end.split('_')[-1]
    n0 = raw[end][-2] - raw[start][-2]
    n1 = raw[end][-1] - raw[start][-1]
    print('{:<9} | {:10} | {:10}'.format(bname, n0, n1))

for f in files:
    raw[f] = []
    file = open(f, 'r')
    for line in file:
        if 'TLB:' in line:
            linesplit = line.split()
            for item in linesplit[1:25]:
                raw[f].append(int(item))
            n0_total = sum(raw[f][0:6]) + sum(raw[f][12:18])
            n1_total = sum(raw[f][6:12]) + sum(raw[f][18:24])
            raw[f].append(n0_total)
            raw[f].append(n1_total)

print('{:<9} | {:>10} | {:>10}'.format('Bench', 'n0 Total', 'n1 Total'))
compare_and_print('02_base', '01_fresh')
compare_and_print('03_heavy', '02_base')
compare_and_print('04_massive', '03_heavy')
compare_and_print('06_userspace', '05_userspace_barrier')
