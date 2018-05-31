import os
import sys
import time

mod      = 'mod_mtier.ko'
exp      = '../mtier_utils/stream'
numastr  = 'numactl --membind=0 --cpunodebind=1'
mtierstr = 'mtier3 --ftnode 1 --stnode 0 --verbose'
dmesg    = '/var/log/messages'

runs = 10
ft_sizes  = [64, 128, 256, 512, 1024]
ft_fracs  = [16, 8, 4, 2, 1]
ft_delays = [50, 100, 200, 500, 1000]
ft_bks    = [2, 5, 10, 25, 50]

'''
for ft_size in ft_sizes:
    for ft_frac in ft_fracs:
        for ft_delay in ft_delays:
            for ft_bk in ft_bks:
                for i in range(0, runs):
                    print('iter={} size={} fraction={} delay={} bk={}'.
                          format(i, ft_size, ft_frac, ft_delay, ft_bk))
                    d_out = 'results/{}-{}-{}-{}-{}-dmesg.out'.format(ft_size,
                                                                      ft_frac,
                                                                      ft_delay,
                                                                      ft_bk,
                                                                      i)
                    s_out = 'results/{}-{}-{}-{}-{}-str.out'.format(ft_size,
                                                                    ft_frac,
                                                                    ft_delay,
                                                                    ft_bk,
                                                                    i)
                    n_out = 'results/{}-{}-{}-{}-{}-nct.out'.format(ft_size,
                                                                    ft_frac,
                                                                    ft_delay,
                                                                    ft_bk,
                                                                    i)

                    varstr = 'ft_size_mb={}'.format(ft_size)
                    varstr = varstr + ' ft_fraction={}'.format(ft_frac)
                    varstr = varstr + ' ft_delay={}'.format(ft_delay)
                    varstr = varstr + ' ft_bk_iters={}'.format(ft_bk)

                    cmdstr = 'sudo tail -f {} > {} &'.format(dmesg, d_out)
                    print('executing: {}'.format(cmdstr))
                    os.system(cmdstr)
                    time.sleep(2)
                    cmdstr = 'sudo insmod {} {}'.format(mod, varstr)
                    print('executing: {}'.format(cmdstr))
                    os.system(cmdstr)
                    time.sleep(2)

                    cmdstr = 'numactl -H > {}'.format(n_out)
                    print('executing: {}'.format(cmdstr))
                    os.system(cmdstr)
                    time.sleep(2)


                    cmdstr = '{} {} {} > {}'.format(numastr, mtierstr,
                                                    exp, s_out)
                    print('executing: {}'.format(cmdstr))
                    os.system(cmdstr)
                    time.sleep(2)
                    cmdstr = 'sudo rmmod {}'.format(mod[0:-3])
                    print('executing: {}'.format(cmdstr))
                    os.system(cmdstr)
                    time.sleep(2)
                    cmdstr = 'sudo killall tail'
                    print('executing: {}'.format(cmdstr))
                    #os.system('sudo killall tail')
'''
varstr = 'ft_size_mb=64'
varstr = varstr + ' ft_fraction=1'
varstr = varstr + ' ft_delay=5'
varstr = varstr + ' ft_bk_iters=10'
os.system('sudo insmod mod_mtier.ko {}'.format(varstr))

cmdstr = '{} {} {}'.format(numastr, mtierstr, exp)

for i in range(50):
    print('starting iter {}'.format(i))
    os.system(cmdstr)
    print('ending iter {}'.format(i))
