# Script to plot the EMIF bandwidth in a continous basis
#
# Author: Karthik Ramanan (karthik.ramanan@ti.com)
#
# License: BSD
#

import ConfigParser
import os
import datetime
import time
import csv
from pylab import *
import pylab as pl
import matplotlib.gridspec as gridspec

IPADDRESS="dummy"
PATH="default"
DELAY=5
OUTFILE="default.csv"
OPTION="-x"
INTERVAL_US=30000

def parse_arguments():
        global IPADDRESS, PATH, DELAY, OUTFILE, OPTION

        config = ConfigParser.ConfigParser()
        config.read('configstat.ini')
        print config.items('core')

        IPADDRESS=config.get('core','ipaddress')
        PATH=config.get('core','path')
        DELAY=config.getint('core','refreshrate')

        print "IPAddress is " + IPADDRESS
        print "Path is      " + PATH
        print DELAY

        if OPTION != "-f":
                unique = datetime.datetime.now().strftime("%Y%m%d-%H%M%S") 
                OUTFILE = unique + ".csv"
                print OUTFILE

                cmd = "scp root@" + IPADDRESS + ":" + PATH + " " + OUTFILE
                print cmd
                os.system(cmd)

        if os.path.isfile(OUTFILE) == False:
                exit
        else:
                print "File found"

def plot_graphs():
        pl.figure()

        ARRAY = []
        cols=0
        ifile  = open(OUTFILE, "rb")
        reader = csv.reader(ifile)
        for row in reader:
                totalcolumns = len(row) - 1
                break
        ifile.seek(0)


        EMIF_SYS1 = []
        EMIF_SYS2 = []
        gs = gridspec.GridSpec(1, 1)
        while (cols < totalcolumns):
                #print 'Reading column number: ' + str(cols)
                for row in reader:
                        ARRAY.append(int(row[cols].split('=')[1]))

                title=row[cols].split('=')[0]
                if title == 'STATCOL_EMIF1_SYS ':
                        print "Ignoring " + title
                        EMIF_SYS1 = list(ARRAY)
                        cols+=1
                        ifile.seek(0)
                        ARRAY = []
                        continue
                elif title == 'STATCOL_EMIF2_SYS ':
                        print "Ignoring " + title
                        EMIF_SYS2 = list(ARRAY)
                        cols+=1
                        ifile.seek(0)
                        ARRAY = []
                        continue
        
                if ARRAY.count(0) == (len(ARRAY)): 
                        print "All elements are zero for " + title
                        cols+=1
                        ifile.seek(0)
                        ARRAY = []
                        continue
                else:
                        print "Plotting graph for " + title
                
                pl.plot(ARRAY, label=title)

                cols+=1
                ifile.seek(0)
                ARRAY = []

        TOTAL=[]
        counter=0
        for item in EMIF_SYS1:
                TOTAL.append(int(EMIF_SYS1[counter]) + int(EMIF_SYS2[counter]))
                counter+=1

        pl.title("J6 L3 Bandwidth stats plot", fontsize=10)
        pl.plot(TOTAL, label='TOTAL')

        legend = pl.legend()
        pl.xlabel("Sample no")
        pl.ylabel("Bytes per sample")
        ifile.close()

        pl.show()

def display_average():
        ARRAY = []
        cols=0
        ifile  = open(OUTFILE, "rb")
        reader = csv.reader(ifile)
        for row in reader:
                totalcolumns = len(row) - 1
                break
        ifile.seek(0)


        EMIF_SYS1 = []
        EMIF_SYS2 = []
        print "-------------------------------------------------------------------"
        print " Initiator                   Average      Peak        Average(active)"
        print "-------------------------------------------------------------------"
        while (cols < totalcolumns):
                for row in reader:
                        ARRAY.append(int(row[cols].split('=')[1]))

                title=row[cols].split('=')[0]
                cols+=1
                ifile.seek(0)
                #print ARRAY
                summ=0
                for items in ARRAY:
                        summ += (items * 1000/30)
                total=len(ARRAY)
                #print "Sum is " + str(summ)
                #print "Total is " + str(total)
                #print title + " Average is " + str((summ/total)/1000000) + " Mbps"
                #print str.ljust(title,20," ") + str.rjust(str(round((summ/total)/1000000, 2)), 30, " ")
                print str.ljust(title,25," ") + str.rjust(str(round((summ/total)/1000000, 2)), 12, " ") + str.rjust(str(round(max(ARRAY)/INTERVAL_US, 2)), 10," ") + str.rjust(str(round((summ/(len([x for x in ARRAY if x > 0])+1)/1000000), 2)),10, " ") + " (" + str(len([x for x in ARRAY if x > 0])) + ")"
                #print "Non zero elements in list is " + str(len([x for x in ARRAY if x > 0]))
                
                ARRAY = []

        ifile.close()
        print "-------------------------------------------------------------------"
        print "-------------------------------------------------------------------\n"

#main
print "Number of arguments " + str(len(sys.argv))
print "Arguments = " + str(sys.argv)

if len(sys.argv) == 1: 
        print "Only one argument. Will parse all arguments from the config file"
else:
        OUTFILE = str(sys.argv[2])
        OPTION = str(sys.argv[1])
        print "New command" + str(sys.argv[2])
        print "File is " + OUTFILE
        print "option is " + OPTION 
        if OPTION == "-f":
                print "Local file option provided.. "

        if os.path.isfile(OUTFILE) == False:
                print "File not found"
                sys.exit()
        else:
                print "Local file found.. "

parse_arguments()
display_average()
plot_graphs()
