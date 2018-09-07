#!/usr/local/bin/python3

import subprocess
import sys
import signal
import time
import os
from operator import add

# Important vars
num_iters = 10

# Set up what we need for invoking numactl and mtier
moddir          = '/home/jteague6/projects/mtier/module/'
modheavydir     = '/home/jteague6/projects/mtier/module_heavy/'
modlightdir     = '/home/jteague6/projects/mtier/module_light/'
modmassivedir   = '/home/jteague6/projects/mtier/module_massive/'
mod             = 'mod_mtier.ko'
progsdir        = '/home/jteague6/projects/mtier/mtier_utils/'
numactl         = 'numactl'
bwdir           = '/home/jteague6/IntelPerformanceCounterMonitorV2.7/'
bw              = 'pcm-memory.x'
debugfsdir      = '/sys/kernel/debug/tracing/'
tracedir        = debugfsdir + 'trace'
intdir          = '/proc/interrupts'
localbind       = ['--membind=1', '--cpunodebind=1']
remotebind      = ['--membind=0', '--cpunodebind=1']
mtierbind       = remotebind
mtier           = 'mtier3'
mtierargs       = ['--ftnode', '1', '--stnode', '0', '--verbose']

module_variants = {
    'base'    : moddir,
    'heavy'   : modheavydir,
    'massive' : modmassivedir,
}

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

    for n, v in module_variants.items():
        res_dir = 'results_' + n + '/'
        l_moddir = v
        print(res_dir, l_moddir)
        for i in range(0, num_iters):
            expfname    = res_dir + cfg + '-' + str(threads) + '-stdout-' + str(i) + '.txt'
            bwfname     = res_dir + cfg + '-' + str(threads) + '-bw-' + str(i) + '.txt'
            dmfname     = res_dir + cfg + '-' + str(threads) + '-dmesg-' + str(i) + '.txt'
            tracename   = res_dir + cfg + '-' + str(threads) + '-tlb_trace-' + str(i) + '.txt'
            preintname  = res_dir + cfg + '-' + str(threads) + '-int_base-' + str(i) + '.txt'
            postintname = res_dir + cfg + '-' + str(threads) + '-int_res-' + str(i) + '.txt'
            print(expfname)
            print(bwfname)
            print(dmfname)
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
                dmf = open(dmfname, 'w')
                dm_invoke = ['tail', '-f', '/var/log/messages']
                tracef = open(tracename, 'w')
                trace_clear = 'echo "" > ' + tracedir
                trace_invoke = ['cat', tracedir]
                print(invoke)
            
        
            expf     = open(expfname, 'w')
            bwf      = open(bwfname, 'w')
            preintf  = open(preintname, 'w')
            postintf = open(postintname, 'w')

            # Handle module setup and insertion if needed
            if is_baseline == False:
                module_invoke = ['insmod', l_moddir + mod]
                for k, v in mtier_params.items():
                    module_invoke.append(k + '=' + v)
                print(module_invoke)
                dmmon = subprocess.Popen(dm_invoke, stdout = dmf)
                modmon = subprocess.Popen(module_invoke)
                modmon.wait()
                time.sleep(5)
                tclearmon = subprocess.Popen(trace_clear, shell=True)
                tclearmon.wait()

            bwmon = subprocess.Popen([bwdir + bw], stdout = bwf)
            time.sleep(1)
            intmon = subprocess.Popen(['cat', intdir], stdout = preintf)
            intmon.wait()
            preintf.close()
            exp = subprocess.Popen(invoke, stdout = expf)
            exp.wait()
            intmon = subprocess.Popen(['cat', intdir], stdout = postintf)
            intmon.wait()
            postintf.close()
            time.sleep(1)
            expf.close()
            bwf.close()
            bwmon.send_signal(signal.SIGTERM)

            if is_baseline == False:
                tracemon = subprocess.Popen(trace_invoke, stdout = tracef)
                tracef.close()
                time.sleep(5)
                module_invoke = ['rmmod', 'mod_mtier']
                modmon = subprocess.Popen(module_invoke)
                modmon.wait()
                dmmon.send_signal(signal.SIGTERM)
                dmf.close()
                os.system('sync')
                os.system('echo 1 > /proc/sys/vm/drop_caches')
                time.sleep(10)

def analyze_stream(cfg, path, is_baseline):
    #numthreads = [1]
    numthreads = [1, 2, 4, 6]
    print(path + cfg + ':')
    print('Bandwidth Table:')
    print('Threads | Node 0 Read | Node 0 Write | Node 0 Total |' + \
          ' Node 1 Read | Node 1 Write | Node 1 Total | Total Read |' + \
          ' Total Write | Grand Total | Node 0 % | Node 1 % |')
    # Get bandwidth info from pcmtools output
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

        for i in range(0, num_iters):
            bwfname = path + cfg + '-' + str(threads) + '-bw-' + str(i) + '.txt'
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

        # Why didn't I just loop this?
        # Channel 0
        '''
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
        '''
        
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
        n0_total_avg = n0_read_avg + n0_write_avg
        n1_total_avg = n1_read_avg + n1_write_avg
        total_read = n0_read_avg + n1_read_avg
        total_write = n0_write_avg + n1_write_avg
        grand_total = total_read + total_write
        n0_pct = n0_total_avg / (n0_total_avg + n1_total_avg) * 100
        n1_pct = n1_total_avg / (n0_total_avg + n1_total_avg) * 100
        print('{0:7d} | '.format(threads) +                     \
              '{0:11.3f} | '.format(n0_read_avg) +              \
              '{0:12.3f} | '.format(n0_write_avg) +             \
              '{0:12.3f} | '.format(n0_total_avg) +             \
              '{0:11.3f} | '.format(n1_read_avg) +              \
              '{0:12.3f} | '.format(n1_write_avg) +             \
              '{0:12.3f} | '.format(n1_total_avg) +             \
              '{0:10.3f} | '.format(total_read) +               \
              '{0:11.3f} | '.format(total_write) +              \
              '{0:11.3f} | '.format(grand_total) +              \
              '{0:8.2f} | '.format(n0_pct) +                   \
              '{0:8.2f} | '.format(n1_pct))
        #print('')


    print()
    print()

    # Get benchmark output from stdout
    print('STREAM Table (times in seconds):')
    print('Threads | Iterations | Total Time | Time / Iter |')
    for threads in numthreads:
        total_iters = 0
        total_time = 0.0
        
        for i in range(0, num_iters):
            stdoutfname = path + cfg + '-' + str(threads) + '-stdout-' + str(i) + '.txt'
            stdoutf = open(stdoutfname, 'r')
            for line in stdoutf:
                if 'stream: ' in line:
                    linesplit = line.split()
                    total_iters = total_iters + int(linesplit[2])
                    total_time = total_time + int(linesplit[5])
            stdoutf.close()

        time_secs = total_time / 1000000000
        secs_per_iter = time_secs / total_iters
        print('{0:7d} |'.format(threads),
              '{0:10d} |'.format(total_iters),
              '{0:10.3f} |'.format(time_secs),
              '{0:11.3f} |'.format(secs_per_iter))

    print()
    print()

    if is_baseline == True:
        print('No dmesg information for baseline runs.')
        print()
        print()
        print('#' * 50)
        return 0

    # Get behind-the-scenes information from dmesg output
    print('Kernel Output Table (time in microseconds):')
    print('Threads | Iterations | Heavy Copies | Heavy Copy Pages |',
          'Heavy Copy Time | Light Swaps | Light Swap Pages |',
          'Light Swap Time |')
    
    for threads in numthreads:
        mtier_iters = 0
        heavy_copies = 0
        light_copies = 0
        shortest_heavy_copy = 0.0
        shortest_light_copy = 0.0
        longest_heavy_copy = 0.0
        longest_light_copy = 0.0
        total_heavy_time = 0.0
        total_light_time = 0.0
        total_heavy_pages = 0
        total_light_pages = 0
        num_heavy_copies = 0.0
        num_light_copies = 0.0

        for i in range(0, 10):
            dmfname = path + cfg + '-' + str(threads) + '-dmesg-' + str(i) + '.txt'
            dmf = open(dmfname, 'r')
            for line in dmf:
                if 'mod_iter:' in line:
                    mtier_iters = mtier_iters + 1
                elif 'mtier duplicate heavy-copied' in line:
                    linesplit = line.split(']')
                    deetsplit = linesplit[1].split()
                    heavy_copies += 1
                    total_heavy_time += float(deetsplit[6])
                    total_heavy_pages += int(deetsplit[3])
                elif 'evict_unused_tier_structs' in line:
                    linesplit = line.split(']')
                    deetsplit = linesplit[1].split()
                    light_copies += 1
                    total_light_time += float(deetsplit[5])
                    total_light_pages += int(deetsplit[2])
            dmf.close()

        heavy_microtime = total_heavy_time / 1000
        light_microtime = total_light_time / 1000
        print('{0:7d} |'.format(threads),
              '{0:10d} |'.format(mtier_iters),
              '{0:12d} |'.format(heavy_copies),
              '{0:16d} |'.format(total_heavy_pages),
              '{0:15.2f} |'.format(heavy_microtime),
              '{0:11d} |'.format(light_copies),
              '{0:16d} |'.format(total_light_pages),
              '{0:15.2f} |'.format(light_microtime))

    # Get TLB flush information (from interrupts procfile)
    print()
    print('TLB Interrupt Information (total over {} runs):'.format(num_iters))
    print('{0:7} |'.format('Threads'), end = "")
    for i in range(0, 6):
        print('   Core {}   |'.format(i + 1), end = "")
    print('   Totals   |')
    for threads in numthreads:
        grand_total = 0
        totals = [0, 0, 0, 0, 0, 0]
        for i in range(0, num_iters):
            tmp_bases = [0, 0, 0, 0, 0, 0]
            tmp_finals = [0, 0, 0, 0, 0, 0]
            basename = path + cfg + '-' + str(threads) + '-int_base-' + str(i) + '.txt'
            basef = open(basename, 'r')
            resname = path + cfg + '-' + str(threads) + '-int_res-' + str(i) + '.txt'
            resf = open(resname, 'r')
            # Get base values
            for line in basef:
                if 'TLB' in line:
                    linesplit = line.split()
                    for j in range(7, 13):
                        tmp_bases[j - 7] += int(linesplit[j])
            for line in resf:
                if 'TLB' in line:
                    linesplit = line.split()
                    for j in range(7, 13):
                        tmp_finals[j - 7] += int(linesplit[j])
            resf.close()
            basef.close()
            for j in range(0, 6):
                totals[j] += (tmp_finals[j] - tmp_bases[j])
        print('{0:7d} |'.format(threads), end = "")
        for i in range(0, 6):
            print('{0:11d} |'.format(totals[i]), end="")
        grand_total = sum(totals)
        print('{0:11d} |'.format(grand_total))

    # Get TLB information from tracepoint dumps
    print()
    print('TLB Tracepoint Information (total over {} runs):'.format(num_iters))
    print('{0:7} |'.format('Threads'),
          '{0:15} |'.format('Total IPI Sends'),
          '{0:15} |'.format('Total MM Flushes'),
          '{0:12} |'.format('Range Flushes'),
          '{0:13} |'.format('Pages Flushed'),
          '{0:11} |'.format('All Sources'))
    for thread in numthreads:
        ipi_sends = 0
        mm_dumps = 0
        range_flushes = 0
        pages_flushed = 0
        all_sources = 0
        for i in range(0, num_iters):
            tlbname = path + cfg + '-' + str(thread) + '-tlb_trace-' + str(i) + '.txt'
            tlbf = open(tlbname, 'r')
            for line in tlbf:
                if line[0] == '#':
                    continue
                if 'mtier_worker-' in line or 'stream-' in line or '<...>' in line:
                    if 'remote ipi send' in line:
                        ipi_sends += 1
                    elif 'remote shootdown' in line:
                        linesplit = line.split()
                        for word in linesplit:
                            if 'pages:' in word:
                                parts = word.split(':')
                                num_pages = parts[1]
                                if num_pages == '-1':
                                    mm_dumps += 1
                                else:
                                    pages_flushed += int(num_pages)
                                    range_flushes += 1
                core = line[line.find("[") + 1:line.find("]")]
                if int(core) > 5 or int(core) < 12:
                    all_sources += 1

            tlbf.close()
        print('{0:7d} |'.format(thread),
              '{0:15d} |'.format(ipi_sends),
              '{0:15d} |'.format(mm_dumps),
              '{0:12d} |'.format(range_flushes),
              '{0:13d} |'.format(pages_flushed),
              '{0:11d} |'.format(all_sources))
    

    print()
    print()
    print('#' * 50)
    print()
    print()


stream_cfgs_base = ['baseline-local',
                    'baseline-remote']
stream_cfgs_1000 = ['1000-50-1-20',
                    '1000-100-1-20',
                    '1000-250-1-20',
                    '1000-500-1-20',
                    '1000-1000-1-20']
stream_cfgs_98 = [  #'98-50-1-20',
                    '98-100-1-20',]
                    #'98-250-1-20',
                    #'98-500-1-20',
                    #'98-1000-1-20']
stream_cfgs_196 = [ '196-50-1-20',
                    '196-100-1-20',
                    '196-500-1-20',
                    '196-1000-1-20']
stream_cfgs_392 = [ '392-50-1-20',
                    '392-100-1-20',
                    '392-500-1-20',
                    '392-1000-1-20']
stream_cfgs_1000 = ['1000-50-1-20',
                    '1000-100-1-20',
                    '1000-500-1-20',
                    '1000-1000-1-20']
#all_stream_cfgs = stream_cfgs_base
all_stream_cfgs = stream_cfgs_1000

def process_stream_runs(runs):
    threads = [1, 2, 4, 6]
    cfgs = []
    if 'all' in runs:
        cfgs = all_stream_cfgs
    else:
        for run in runs:
            cfgs.append(run)

    for cfg in cfgs:
        if 'baseline' in cfg:
            for thread in threads:
                os.environ['OMP_NUM_THREADS'] = str(thread)
                run_stream(thread, cfg, True)
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
            for thread in threads:
                os.environ['OMP_NUM_THREADS'] = str(thread)
                run_stream(thread, cfg, False, mtier_params)

def process_stream_analysis(runs):
    cfgs = []
    if 'all' in runs:
        cfgs = all_stream_cfgs
    else:
        for run in runs:
            cfgs.append(run)

    #print(cfgs)
    for key, path in module_variants.items():
        path = 'results_' + key + '/'
        for cfg in cfgs:
            if 'baseline' in cfg:
                analyze_stream(cfg, path, True)
            else:
                analyze_stream(cfg, path, False)

#==================#
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
