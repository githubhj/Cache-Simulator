
from __future__ import print_function
from lib2to3.fixer_util import String
import glob
import os


traces= ['0','0','0','0']

for trace_files in glob.glob("*.trace"):
    traces[i]=trace_files
    print(traces[i],"\n")
    i=i+1

for i in range(0,5):
    logfile = "log_astar_c15_s10_b5_k0_v" + str(i) +".txt"
    cmd = "Cache_Simulator.exe -c 15 -b 5 -s 10 -k 0 -v " + str(i) + " -i " + traces[0] + ">"+logfile
    os.system(cmd)

for i in range(0,5):
    logfile = "log_astar_c15_s10_b5_v0_k" + str(i) +".txt"
    cmd = "Cache_Simulator.exe -c 15 -b 5 -s 10 -v 0 -k " + str(i) + " -i " + traces[0] + ">"+logfile
    os.system(cmd)

for i in range(0,5):
    s = 9-i
    logfile = "log_astar_c15_b6_s1_v4_k" + str(i) +".txt"
    cmd = "Cache_Simulator.exe -c 15 -b 6 -s 1 -v 4 -k " + str(i) + " -i " + traces[0] + ">"+logfile
    os.system(cmd)
'''
'''
for i in range(0,10):
    s = 9-i
    logfile = "log_bzip2_c15_b6_k0_v0_s" + str(s) +".txt"
    cmd = "Cache_Simulator.exe -c 15 -b 6 -k 0 -v 0 -s " + str(s) + " -i " + traces[1] + ">"+logfile
    os.system(cmd)

for i in range(0,5):
    s = 9-i
    logfile = "log_bzip2_c15_b6_s1_k4_v" + str(i) +".txt"
    cmd = "Cache_Simulator.exe -c 15 -b 6 -s 1 -k 4 -v " + str(i) + " -i " + traces[1] + ">"+logfile
    os.system(cmd)

for i in range(0,10):
    s = 9-i
    logfile = "log_mcf_c15_b6_k0_v0_s" + str(s) +".txt"
    cmd = "Cache_Simulator.exe -c 15 -b 6 -k 0 -v 0 -s " + str(s) + " -i " + traces[2] + ">"+logfile
    os.system(cmd)
    
for i in range(0,5):
    s = 9-i
    logfile = "log_mcf_c15_b6_s2_v4_k" + str(i) +".txt"
    cmd = "Cache_Simulator.exe -c 15 -b 6 -s 2 -v 4 -k " + str(i) + " -i " + traces[2] + ">"+logfile
    os.system(cmd)

for i in range(0,5):
    s = 9-i
    logfile = "log_mcf_c15_b6_s2_k0_v" + str(i) +".txt"
    cmd = "Cache_Simulator.exe -c 15 -b 6 -s 2 -k 0 -v " + str(i) + " -i " + traces[2] + ">"+logfile
    os.system(cmd)


for i in range(0,10):
    s = 9-i
    logfile = "log_perlbench_c15_b6_k0_v0_s" + str(s) +".txt"
    cmd = "Cache_Simulator.exe -c 15 -b 6 -k 0 -v 0 -s " + str(s) + " -i " + traces[3] + ">"+logfile
    os.system(cmd)

for i in range(0,5):
    s = 9-i
    logfile = "log_perlbench_c15_b6_s3_v4_k" + str(i) +".txt"
    cmd = "Cache_Simulator.exe -c 15 -b 6 -s 3 -v 4 -k " + str(i) + " -i " + traces[3] + ">"+logfile
    os.system(cmd)

for i in range(0,5):
    s = 9-i
    logfile = "log_perlbench_c15_b6_s3_k4_v" + str(i) +".txt"
    cmd = "Cache_Simulator.exe -c 15 -b 6 -s 3 -k 4 -v " + str(i) + " -i " + traces[3] + ">"+logfile
    os.system(cmd)


print("I am here\n")