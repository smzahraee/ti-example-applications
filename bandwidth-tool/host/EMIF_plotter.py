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

config = ConfigParser.ConfigParser()
config.read('config.ini')
print config.items('core')

IPADDRESS=config.get('core','ipaddress')
PATH=config.get('core','path')
DELAY=config.getint('core','refreshrate')

print "IPAddress is " + IPADDRESS
print "Path is      " + PATH
print DELAY

UNIQUE = datetime.datetime.now().strftime("%Y%m%d-%H%M%S") 
OUTFILE = UNIQUE + ".csv"
print OUTFILE

cmd = "scp root@" + IPADDRESS + ":" + PATH + " " + OUTFILE
print cmd
os.system(cmd)

if os.path.isfile(OUTFILE) == False:
        exit
else:
        print "File found"


pl.figure(figsize=(10, 10))
pl.ion()
pl.show()

while (True):
        ARRAY = []
        cols=0
        os.system(cmd);
        ifile  = open(OUTFILE, "rb")
        reader = csv.reader(ifile)
        for row in reader:
                totalcolumns = len(row) - 1
                break
        ifile.seek(0)


        gs = gridspec.GridSpec(4, 1)
        while (cols < totalcolumns):
                for row in reader:
                        ARRAY.append(row[cols].split('=')[1])

                title=row[cols].split('=')[0]
                pl.subplot(gs[cols, :])
                pl.ylim([0,100])
                pl.title(title, fontsize=10)
                pl.grid(b=None, which='major', axis='both', color='r')
                pl.plot(ARRAY,'g-')

                cols+=1
                ifile.seek(0)
                ARRAY = []

        pl.draw()
        time.sleep(DELAY)
        ifile.close()

exit

