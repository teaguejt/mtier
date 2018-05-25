#! /usr/bin/env python

import sys

def lock_stats(events, start_event, end_event, caption):
    print('lock stats not implemented')

def function_stats(events, start_event, end_event, caption):
    calls = 0
    shortest_call = 0
    longest_call = 0
    total_time   = 0
    scheduler_events = []

    for i in range(0, len(events)):
        if start_event not in events[i]:
            continue

        words = events[i].split()
        for word in words:
            if 'time' in word:
                start = int(word.split('=')[1])

        i += 1
        while(end_event not in events[i]):
            if schedule_event in events[i]:
                scheduler_events.append(events[i])
            i += 1

        # Now we should be looking at the end event
        words = events[i].split()
        for word in words:
            if 'time' in word:
                end = int(word.split('=')[1])
        
        duration = end - start
        if calls == 0:
            shortest_call = duration
            longest_call = duration
        elif duration > longest_call:
            longest_call = duration
        elif duration < shortest_call:
            shortest_call = duration

        total_time += duration
        calls += 1

        ''' start = end = 0
        words = events[i].split()
        for word in words:
            if 'time' in word:
                start = long(word.split('=')[1])
        words = events[i + 1].split()
        for word in words:
            if 'time' in word:
                end = long(word.split('=')[1])

        duration = end - start
        if calls == 0:
            shortest_call = duration
            longest_call = duration
        elif duration > longest_call:
            longest_call = duration
        elif duration < shortest_call:
            shortest_call = duration

        total_time += duration
        calls += 1'''

    print(caption.upper())
    print('=' * len(caption))
    print('total_time    %10ld ns' % (total_time))
    print('total_calls   %10d'     % (calls))
    print('shortest_call %10ld ns' % (shortest_call))
    print('longest_call  %10ld ns' % (longest_call))
    if(calls != 0):
        print('avg_call_time %10ld ns' % (total_time / calls))
    print ''

    # Ignore partial scheduler events
    i = 0
    while 'pid=' + str(pid) not in scheduler_events[i]:
        i += 1
    if 'prev_pid=' + str(pid) in scheduler_events[i]:
        del scheduler_events[i]

    i = len(scheduler_events) - 1
    while 'pid=' + str(pid) not in scheduler_events[i]:
        i -= 1
    if 'next_pid=' + str(pid) in scheduler_events[i]:
        del scheduler_events[i]
    scheduler_stats(scheduler_events, caption.replace('stats', '') + ' scheduler stats')

def scheduler_stats(events, caption):
    sched_events = 0
    shortest_sched = 0
    longest_sched = 0
    avg_sched = 0
    stime = 0
    etime = 0
    ttime = 0
    gtime = 0   # grand total

    print('examining %d scheduler events' % (len(events)))
    for i in range(0, len(events)):
        if 'next_pid=' + str(pid) in events[i]:
            captures = events[i].split()
            for capture in captures:
                if 'time=' in capture:
                    timevals = capture.split('=')
                    stime = int(timevals[1])
        if 'prev_pid=' + str(pid) in events[i]:
            captures = events[i].split()
            for capture in captures:
                if 'time=' in capture:
                    timevals = capture.split('=')
                    etime = int(timevals[1])
            # Now that we have the end event, we can piece together some statistics
            ttime = etime - stime
            if sched_events == 0:
                shortest_sched = ttime
                longest_sched  = ttime
            elif ttime > longest_sched:
                longest_sched = ttime
            elif ttime < shortest_sched:
                shortest_sched = ttime
            gtime += ttime
            sched_events += 1

    avg_sched = float(gtime) / sched_events
    
    print(caption.upper())
    print('=' * len(caption))
    print 'total_time   %15ld ns' % (gtime)
    print 'sched_events %15d'     % (sched_events)
    print 'shortest     %15d ns'  % (shortest_sched)
    print 'longest      %15d ns'  % (longest_sched)
    print 'avg          %15d ns'  % (avg_sched)
    print ''

schedule_event            = 'mtier_sched_switch'
do_mtier_management_start = 'entry_do_mtier_management'
do_mtier_management_end   = 'exit_do_mtier_management'
queue_pages_range_start   = 'enter_mtier_queue_pages_range'
queue_pages_range_end     = 'exit_mtier_queue_pages_range'
shrink_pagelist_start     = 'enter_mtier_shrink_pagelist'
shrink_pagelist_end       = 'exit_mtier_shrink_pagelist'
migrate_pages_start       = 'enter_migrate_pages'
migrate_pages_end         = 'exit_migrate_pages'
schedule_events   = []
mtier_management  = []
queue_pages_range = []
shrink_pagelist   = []
migrate_pages     = []
events = []


if len(sys.argv) != 3:
    print('usage: %s pid <trace file>' % (sys.argv[0]))

try:
    f = open(sys.argv[2], 'r')
except:
    print('error opening file %s.' % (sys.argv[0]))
    sys.exit(1)

try:
    pid = int(sys.argv[1])
except:
    print('your pid has to be an integer!')
    sys.exit(1)

print('statistics for pid %d contained in tracefile %s' % (pid, sys.argv[2]))

for line in f:
    events.append(line)
    #if schedule_event in line:
    #    schedule_events.append(line)
    #if do_mtier_management_start in line or do_mtier_management_end in line:
    #    mtier_management.append(line)
    #if queue_pages_range_start in line or queue_pages_range_end in line:
    #    queue_pages_range.append(line)
    #if shrink_pagelist_start in line or shrink_pagelist_end in line:
    #    shrink_pagelist.append(line)
    #if migrate_pages_start in line or migrate_pages_end in line:
    #    migrate_pages.append(line)

scheduler_stats(events, 'general scheduler stats')

function_stats(events, do_mtier_management_start, do_mtier_management_end, 'do_mtier_management stats')
function_stats(events, queue_pages_range_start, queue_pages_range_end, 'queue_pages_range stats')
function_stats(events, shrink_pagelist_start, shrink_pagelist_end, 'shrink_pagelist stats')

f.close()
