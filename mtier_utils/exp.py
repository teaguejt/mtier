#!/usr/local/bin/python3

import subprocess
import sys
import signal
import time
import os
from operator import add

# Set up what we need for invoking numactl and mtier
moddir     = '/home/jteague6/projects/mtier/module/'
mod        = 'mod_mtier.ko'
progsdir   = '/home/jteague6/projects/mtier/mtier_utils/'
numactl    = 'numactl'
bwdir      = '/home/jteague6/IntelPerformanceCounterMonitorV2.7/'
bw         = 'pcm-memory.x'
localbind  = ['--membind=1', '--cpunodebind=1']
remotebind = ['--membind=0', '--cpunodebind=1']
mtierbind  = remotebind
mtier     = 'mtier3'
mtierargs = ['--ftnode', '1', '--stnode', '0', '--verbose']

progs = ['mm', 'stream']
mm_sizes = [1024, 2048, 4096]
exps  = ['baseline-local', 'baseline-remote']
analysis = ['time', 'bandwidth']
results_dict = {}

def run_mm():
    return 0

def run_stream(threads, cfg, is_baseline, mtier_params = None):
    if is_baseline == False and mtier_params == None:
        mtier_params = {'ft_size_mb':  '1000',
                        'ft_fraction': '1',
                        'ft_delay':    '100',
                        'ft_bk_iters': '20'}
    for i in range(0, 10):
        expfname = 'results/stream-' + cfg + '-' + str(threads) + '-stdout-' + str(i) + '.txt'
        bwfname  = 'results/stream-' + cfg + '-' + str(threads) + '-bw-' + str(i) + '.txt'
        print(expfname)
        print(bwfname)
        if is_baseline == True and cfg == 'baseline-local':
            invoke = [numactl] + localbind + [progsdir + 'stream']
            print(invoke)
        elif is_baseline == True and cfg == 'baseline-remote':
            invoke = [numactl] + remotebind + [progsdir + 'stream']
            print(invoke)
        elif is_baseline == False:
            invoke = [numactl] + remotebind + [progsdir + mtier]
            invoke = invoke + mtierargs
            invoke.append(progsdir + 'stream')
            print(invoke)
            
        
        expf = open(expfname, 'w')
        bwf  = open(bwfname, 'w')

        # Handle module setup and insertion if needed
        if is_baseline == False:
            module_invoke = ['insmod', moddir + mod]
            for k, v in mtier_params.items():
                module_invoke.append(k + '=' + v)
            print(module_invoke)
            modmon = subprocess.Popen(module_invoke)
            modmon.wait()
            time.sleep(5)

        bwmon = subprocess.Popen([bwdir + bw], stdout = bwf)
        time.sleep(1)
        exp = subprocess.Popen(invoke, stdout = expf)
        exp.wait()
        time.sleep(1)
        expf.close()
        bwf.close()
        bwmon.send_signal(signal.SIGTERM)

        if is_baseline == False:
            time.sleep(5)
            module_invoke = ['rmmod', 'mod_mtier']
            modmon = subprocess.Popen(module_invoke)
            modmon.wait()

def analyze_stream(cfg, is_baseline):
    #numthreads = [1]
    numthreads = [1, 2, 4, 6, 8, 12]
    print(cfg + ':')
    print('Threads | Channel | Node 0 Read | Node 0 Write | Node 1 Read | ' + \
          'Node 1 Write | Total Iters | Total Time | Time/Iter |')
    for threads in numthreads:
        last_channel = 1
        n0_channel_reads = {0: [], 1: [], 2: [], 3: []}
        n0_channel_writes = {0: [], 1: [], 2: [], 3: []}
        n1_channel_reads = {0: [], 1: [], 2: [], 3: []}
        n1_channel_writes = {0: [], 1: [], 2: [], 3: []}
        node_reads = {0: [], 1: []}
        node_writes = {0: [], 1: []}
        total_reads = []
        total_writes = []
        total_iters = 0
        total_time = 0.0
        for i in range(0, 10):
            bwfname = 'results/stream-' + cfg + '-' + str(threads) + '-bw-' + str(i) + '.txt'
            bwf = open(bwfname, 'r')
            for line in bwf:
                if 'Reads' in line:
                    linesplit = line.split()
                    last_channel = int(linesplit[3].strip(':'))
                    #print(linesplit[6] + ' ' + linesplit[13])
                    n0_channel_reads[last_channel].append(float(linesplit[6]))
                    n1_channel_reads[last_channel].append(float(linesplit[13]))
                    #print(linesplit)
                elif 'Writes' in line:
                    linesplit = line.split()
                    #print(str(linesplit))
                    #print(str(linesplit[2] + ' ' + str(linesplit[5])))
                    n0_channel_writes[last_channel].append(float(linesplit[2]))
                    n1_channel_writes[last_channel].append(float(linesplit[5]))
                elif 'NODE0 Mem Read' in line:
                    linesplit = line.split()
                    #print(linesplit)
                    node_reads[0].append(float(linesplit[5]))
                    node_reads[1].append(float(linesplit[11]))
                elif 'NODE0 Mem Write' in line:
                    linesplit = line.split()
                    node_writes[0].append(float(linesplit[5]))
                    node_writes[1].append(float(linesplit[11]))
                elif 'System Read Throughput' in line:
                    linesplit = line.split()
                    total_reads.append(float(linesplit[4]))
                elif 'System Write Throughput' in line:
                    linesplit = line.split()
                    total_writes.append(float(linesplit[4]))

            bwf.close()

            stdoutfname = 'results/stream-' + cfg + '-' + str(threads) + '-stdout-' + str(i) + '.txt'
            stdoutf = open(stdoutfname, 'r')
            for line in stdoutf:
                if 'stream: ' in line:
                    linesplit = line.split()
                    total_iters = total_iters + int(linesplit[2])
                    total_time = total_time + int(linesplit[5])

        # Why didn't I just loop this?
        # Channel 0
        n0_read_avg = sum(n0_channel_reads[0]) / len(n0_channel_reads[0])
        n0_write_avg = sum(n0_channel_writes[0]) / len(n0_channel_writes[0])
        n1_read_avg = sum(n1_channel_reads[0]) / len(n1_channel_reads[0])
        n1_write_avg = sum(n1_channel_writes[0]) / len(n1_channel_writes[0])
        print('{0:7d} | {1:7d} | '.format(threads, 0) +         \
              '{0:11.3f} | '.format(n0_read_avg) +              \
              '{0:12.3f} | '.format(n0_write_avg) +             \
              '{0:11.3f} | '.format(n1_read_avg) +              \
              '{0:12.3f} | '.format(n1_write_avg) +             \
              '        --- | ' +                                \
              '       --- | ' +                                 \
              '      --- |')
        
        # Channel 1
        n0_read_avg = sum(n0_channel_reads[1]) / len(n0_channel_reads[1])
        n0_write_avg = sum(n0_channel_writes[1]) / len(n0_channel_writes[1])
        n1_read_avg = sum(n1_channel_reads[1]) / len(n1_channel_reads[1])
        n1_write_avg = sum(n1_channel_writes[1]) / len(n1_channel_writes[1])
        print('{0:7d} | {1:7d} | '.format(threads, 1) +         \
              '{0:11.3f} | '.format(n0_read_avg) +              \
              '{0:12.3f} | '.format(n0_write_avg) +             \
              '{0:11.3f} | '.format(n1_read_avg) +              \
              '{0:12.3f} | '.format(n1_write_avg) +             \
              '        --- | '+                                 \
              '       --- | ' +                                 \
              '      --- |')
        
        # Channel 2
        n0_read_avg = sum(n0_channel_reads[2]) / len(n0_channel_reads[2])
        n0_write_avg = sum(n0_channel_writes[2]) / len(n0_channel_writes[2])
        n1_read_avg = sum(n1_channel_reads[2]) / len(n1_channel_reads[2])
        n1_write_avg = sum(n1_channel_writes[2]) / len(n1_channel_writes[2])
        print('{0:7d} | {1:7d} | '.format(threads, 2) +         \
              '{0:11.3f} | '.format(n0_read_avg) +              \
              '{0:12.3f} | '.format(n0_write_avg) +             \
              '{0:11.3f} | '.format(n1_read_avg) +              \
              '{0:12.3f} | '.format(n1_write_avg) +             \
              '        --- | ' +                                \
              '       --- | '+                                  \
              '      --- |')
        
        # Channel 3
        n0_read_avg = sum(n0_channel_reads[3]) / len(n0_channel_reads[3])
        n0_write_avg = sum(n0_channel_writes[3]) / len(n0_channel_writes[3])
        n1_read_avg = sum(n1_channel_reads[3]) / len(n1_channel_reads[3])
        n1_write_avg = sum(n1_channel_writes[3]) / len(n1_channel_writes[3])
        print('{0:7d} | {1:7d} | '.format(threads, 3) +         \
              '{0:11.3f} | '.format(n0_read_avg) +              \
              '{0:12.3f} | '.format(n0_write_avg) +             \
              '{0:11.3f} | '.format(n1_read_avg) +              \
              '{0:12.3f} | '.format(n1_write_avg) +             \
              '        --- | ' +                                \
              '       --- | '+                                  \
              '      --- |')
        
        # TOTAL
        n0_total_reads = [sum(x) for x in zip(n0_channel_reads[0],  \
                                              n0_channel_reads[1],  \
                                              n0_channel_reads[2],  \
                                              n0_channel_reads[3])]
        n0_total_writes = [sum(x) for x in zip(n0_channel_writes[0],    \
                                               n0_channel_writes[1],    \
                                               n0_channel_writes[2],    \
                                               n0_channel_writes[3])]
        n1_total_reads = [sum(x) for x in zip(n1_channel_reads[0],  \
                                              n1_channel_reads[1],  \
                                              n1_channel_reads[2],  \
                                              n1_channel_reads[3])]
        n1_total_writes = [sum(x) for x in zip(n1_channel_writes[0],    \
                                               n1_channel_writes[1],    \
                                               n1_channel_writes[2],    \
                                               n1_channel_writes[3])]
        n0_read_avg = sum(n0_total_reads) / len(n0_total_reads)
        n0_write_avg = sum(n0_total_writes) / len(n0_total_writes)
        n1_read_avg = sum(n1_total_reads) / len(n1_total_reads)
        n1_write_avg = sum(n1_total_writes) / len(n1_total_writes)
        print('{0:7d} |  TOTAL  | '.format(threads) +           \
              '{0:11.3f} | '.format(n0_read_avg) +              \
              '{0:12.3f} | '.format(n0_write_avg) +             \
              '{0:11.3f} | '.format(n1_read_avg) +              \
              '{0:12.3f} | '.format(n1_write_avg) +             \
              '{0:11d} | '.format(total_iters) +                \
              '{0:10.3f} | '.format(total_time / 1000000000) +  \
              '{0:9.2f} |'.format((total_time / 1000000000) / total_iters))
        print('')

all_stream_cfgs = ['baseline-local', 'baseline-remote', '1000-100-1-20']

def process_stream_runs(runs):
    threads = 12
    cfgs = []
    if 'all' in runs:
        cfgs = all_stream_cfgs
    else:
        for run in runs:
            cfgs.append(run)

    for cfg in cfgs:
        if 'baseline' in cfg:
            run_stream(threads, cfg, True)
        else:
            # Put together the mtier params dict
            cfgsplit = cfg.split('-')
            if len(cfgsplit) != 4:
                mtier_params = None
            else:
                mtier_params = {'ft_size_mb'  : cfgsplit[0],
                                'ft_delay'    : cfgsplit[1],
                                'ft_fraction' : cfgsplit[2],
                                'ft_bk_iters' : cfgsplit[3]}
            run_stream(threads, cfg, False, mtier_params)

def process_stream_analysis(runs):
    cfgs = []
    if 'all' in runs:
        cfgs = all_stream_cfgs
    else:
        for run in runs:
            cfgs.append(run)

    #print(cfgs)
    for cfg in cfgs:
        if 'baseline' in cfg:
            analyze_stream(cfg, True)
        else:
            analyze_stream(cfg, False)

#### START HERE ####
#==================#
stream_runs = []
stream_process = []

for arg in sys.argv:
    if 'run-stream-' in arg:
        stream_runs.append(arg.replace('run-stream-', ''))
    elif 'analyze-stream-' in arg:
        stream_process.append(arg.replace('analyze-stream-', ''))

process_stream_runs(stream_runs)
process_stream_analysis(stream_process)

'''
    if arg == 'baseline-local':
        print("Local Baseline")
        run_stream(6, 'baseline-local', True)
    elif arg == 'baseline-remote':
        print("Remote Baseline")
        run_stream(6, 'baseline-remote', True)
    elif arg == 'stream-both':
        run_stream(6, 'baseline-local', True)
        run_stream(6, 'baseline-remote', True)
    elif 'stream-' in arg:
        #Change this based on what has been run
        results_dict = {}
        #run = ['baseline-local', 'baseline-remote']
        print("### ALL VALUES ARE AVERAGED OVER TEN 100+ SECOND STREAM RUNS ###")
        for r in run:
            if r == 'baseline-local':
                #analyze_stream(r, True)
                print('analyze', r)
            elif r == 'baseline-remote':
                #analyze_stream(r, True)
                print('analyze', r)
            else:
                rsplit = r.split('-')
                print(rsplit)
                analyze_stream(r, False)
    else:
        print("Invalid Instruction")
'''
