#!/usr/bin/python

import os,re

partitions=[
    ["nvs","data", "nvs", 0x9000, 0x5000],
    ["otadata","data","ota",0xe000, 0x2000],
    ["app0","app","ota_0",0x10000,0]]


import sys

flash = 4
ota = False
fstype = None
fssize = 32768
#blob = "/home/ethanak/Arduino/libraries/Mimbrola/data/pl1_full/espola.blob"
blob = None
def mkfssize(s):
    r=re.match(r'((?:0x(?:[0-9a-f]+))|(?:[1-9][0-9]*))([km]?)$',s.lower())
    if not r:
        print("Bad size: %s" % s)
        exit(1)
    size = int(r.group(1), 0)
    
    if r.group(2) == 'k':
        mpx = 1024
    else:
        mpx = 1024 * 1024
    size *= mpx
    return size
    
name = ''
board = '<board>'
csv = None
        
for arg in sys.argv[1:]:
    if arg == "ota":
        ota=True
        continue
        
    if arg in ("spi","fat"):
        fstype=arg
        continue
        
    if arg[:4] in ("spi=","fat="):
        fstype = arg[:3]
        fssize = mkfssize(arg[4:])
        continue
        
    if arg.startswith("flash="):
        flash = int(arg[6:])
        if flash not in (4,8,16):
            printf("Bad flash size: only 4, 8 or 16 are accepted")
            exit(1)
        continue
        
    if arg.startswith("menu="):
        name = arg[5:]
        continue
        
    if arg.startswith("board="):
        board=arg[6:]
        continue
        
    if arg.startswith("csv="):
        csv = arg[4:]
        if '.' not in csv:
            csv += '.csv'
        elif not csv.endswith('.csv'):
            print("Bad csv file name: %s" % csv)
            exit(1)
        continue
    
    blob = arg
    break

if not blob:
    print ("Usage: %s [parameters] <filename.blob>" % sys.argv[0])
    print ("""Parameters are:
  flash=<size> - flash size in MB. Acceptable are 4, 8 and 16. Default 4.
  ota - create partition for OTA. Default no OTA.
  spi - create partition for spiffs.
  fat - create partition for fatfs.
  spi=<size> - create partition for spiffs with given size
  fat=<size> - create partition for fatfs woth given size
  menu=<name> - name for position in Arduino IDE menu
  board=<name> - board name from boards.txt
  csv=<name> - name of csv file (without path, csv extension will be added)
Position name will be created from parameters if not given.
CSV content will be dumped to stdout if name not given.
SPIFFS or FATFS partition will be extended to fit flash size.
Extra lines to put into boards.txt and example blob upload command
will be printed.
""")
    exit(0)
    
if not fstype:
    fssize = 0
flash *= 0x100000

f=open(blob,"rb")
f.seek(0, os.SEEK_END)
blobsize = f.tell()
f.close()
blobsize = (blobsize + 0xfff) & 0xffff000
frp = flash - (0x10000 + blobsize + fssize)
if frp < 0:
    print ("Does not fit in given flash size")
    exit(1)
aps = frp

if ota:
    aps = frp // 2
if aps < 512 * 1024:
    print ("Application partition too small (0x%x bytes)" % aps)
    exit(1)
aps = aps & 0xffff000
if aps > 0x3f0000:
    aps = 0x3f0000
frp = flash - (blobsize + 0x10000 + aps)
if ota:
    frp -= aps
if fstype:
    fssize =  frp
partitions[-1][4] = aps
vptr = partitions[-1][4] + partitions[-1][3]
mbrolapos = vptr
if ota:
    partitions.append(["app1","app","ota_1", vptr, aps])
    vptr += aps
partitions.append(["mbrola","data", "0x40", vptr, blobsize])
vptr += blobsize
if fstype:
    fn = 'ffat' if fstype == 'fat' else 'spiffs'
    ft = 'fat' if fstype == 'fat' else 'spiffs'
    partitions.append([ft, "data", ft, vptr, fssize])
    vptr += fssize


ps = "# Name,   Type, SubType, Offset,  Size, Flags\n"
ps += ''.join(list("%-10s%-6s%-9s%-9s0x%x,\n" % (
        p[0]+',',
        p[1]+',',
        p[2]+',',
        ("0x%x"%p[3])+',',
        p[4]) for p in partitions))

if csv:
    f=open(csv,"w")
    f.write(ps)
    f.close()
    print("=== Partitions saved to %s ===" % csv)
else:
    print(ps+"\n")

def prsize(n):
    n = n // 1024
    if (n & 1023):
        return "%dk" % n
    return "%dM" % (n // 1024)

if not name:
    voicename = os.path.basename(os.path.dirname(blob))
    name=["%dM flash" % (flash // 0x100000),"App=%s" % prsize(aps)]
    if ota:
        name.append("OTA")
    if voicename:
        name.append("Mbrola %s %s" % (voicename, prsize(blobsize)))
    else:
        name.append("Mbrola %s" % prsize(blobsize))
    if fstype:
        name.append("%s %s" % (ft.upper(),prsize(fssize)))
    name = ", ".join(name)
    
if not csv:
    pname = '<name>'
else:
    pname = csv[:-4]

print ("\n=== Add these (or similar) lines to your boards.txt ===\n")
print ("%s.menu.PartitionScheme.%s=%s" % (board, pname, name))
print ("%s.menu.PartitionScheme.%s.build.partitions=%s" % (board, pname, pname))
print ("%s.menu.PartitionScheme.%s.upload_maximum_size=%d" % (board, pname, aps))
print("")
print("=== Example upload command: ===\nesptool.py -b 921600 write_flash 0x%x '%s'" % (mbrolapos, blob))
exit(0)

