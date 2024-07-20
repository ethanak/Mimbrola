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
fssize = 0
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

appsize=0
appv = False
voices={
    "de1":[ 5728225, 11456451 ],
    "de2":[ 5223621, 10447243 ],
    "de3":[ 5735053, 11470107 ],
    "en1":[ 3358140, 6716280 ],
    "es1":[ 1415568, 2831137 ],
    "es2":[ 2811152, 5622305 ],
    "es3":[ 978220, 1956441 ],
    "pl1":[ 2372775, 4745551 ],
    "us1":[ 3619047, 7238094 ],
    "us2":[ 3540125, 7080251 ],
    "us3":[ 3705126, 7410252 ]}

hdrsize=0
fitin = False
def list_voices():
    fv=[]
    for a in voices:
        fv.append(a)
    fv.sort()
    vstr = 'Known voices: '+','.join(fv)
    print(vstr)
    exit(0)
vox=None
for arg in sys.argv[1:]:
    if arg == "ota":
        ota=True
        continue
    if arg == "int":
        fitin = True
        continue
    if arg.startswith("app="):
        appsize = mkfssize(arg[4:])
        continue
    if arg == 'hdr':
        appv = True
        continue
    if arg.startswith("hdr="):
        ts = voices.get(arg[4:])
        if not ts:
            list_voices()
        hdrsize = ts[0]
        appv = True
        vox = arg[4:]
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

if (blob and hdrsize):
    print ("Usage: %s [parameters] [<filename.blob>]" % sys.argv[0])
    print ("""Parameters are:
  flash=<size> - flash size in MB. Acceptable are 4, 8 and 16. Default 4.
  ota - create partition for OTA. Default no OTA.
  spi - create partition for spiffs.
  fat - create partition for fatfs.
  hdr - no partition for blob data (header data will be compiled in)
  int - fit application in internal 4MB flash memory
  hdr=<voice> - get size from known voices
  app=<size> - size for app and ota partitions (without header data)
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
frp = flash - 0x10000

if not hdrsize and blob:
    f=open(blob,"rb")
    f.seek(0, os.SEEK_END)
    blobsize = f.tell()
    f.close()
    blobsize = (blobsize + 0xfff) & 0xffff000
    if appv:
        hdrsize = blobsize
        blobsize = 0
    else:
        frp -= blobsize
else:
    blobsize = 0
    hdrsize = (hdrsize + 0xfff) & 0xffff000
    
maxappsize = 0x3f0000 if fitin else 0x7f0000

if appsize:
    appsize += hdrsize
    if appsize > maxappsize:
        print ("Application 0x%x too big, maximum is 0x%x" % (appsize, maxappsize))
        exit(1)
    apmem = appsize
    if ota:
        apmem += appsize
    if frp < apmem + fssize:
        mxp = frp - fssize
        if ota:
            mxp //= 2
        print ("Application 0x%x does not fit in flash, maximum is 0x%x" % (appsize, mxp))
        exit(1)
    if fstype:
        fssize = frp - apmem
    
elif fssize:
    minappsize = 1024 * 1024
    if hdrsize:
        minappsize += hdrsize
    appmem = frp - fssize
    appsize = appmem
    if ota:
        appsize //= 2
    if appsize > maxappsize:
        appsize = maxappsize
    if appsize < minappsize:
        print ("Application 0x%x too small, minimum is 0x%x" % (appsize, minappsize))
        exit(1)
    appmem = appsize
    if ota:
        appmem += appsize
    fssize = frp - appmem
else:
    appsize = frp
    if fstype:
        fssize = 128 * 1024
        appsize -= fssize
    if ota:
        appsize //= 2
    if appsize > maxappsize:
        appsize = maxappsize
    if appsize < 1024 * 1024 + hdrsize:
        print ("Application 0x%x too small, minimum is 0x%x" % (appsize, 1024 * 1024 + hdrsize))
        exit(1)
    appmem = appsize
    if ota:
        appmem += appsize
    if fstype:
        fssize = frp - appmem

partitions[-1][4] = appsize
vptr = partitions[-1][4] + partitions[-1][3]

if ota:
    partitions.append(["app1","app","ota_1", vptr, appsize])
    vptr += appsize
if not appv:
    mbrolapos = vptr
    partitions.append(["mbrola","data", "0x40", vptr, blobsize])
vptr += blobsize
if fstype:
    fn = 'ffat' if fstype == 'fat' else 'spiffs'
    ft = 'fat' if fstype == 'fat' else 'spiffs'
    partitions.append([fn, "data", ft, vptr, fssize])
    vptr += fssize

sysnames= {
    'app0': 'App',
    'app1': 'OTA'
        }

def prsize(n):
    n = n // 1024
    if (n & 1023):
        return "%dk" % n
    return "%dM" % (n // 1024)

print ("=== Summary ===")
for n,p in enumerate(partitions):
    if n < 2:
        continue
    print ('%s:\t%s' % (sysnames.get(p[0], p[0]), prsize(p[4])))

print("")
    
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


if not name:
    voicename = None
    if blob:
         voicename = os.path.basename(os.path.dirname(blob))
    name=["%dM flash" % (flash // 0x100000),"App=%s" % prsize(appsize)]
    if ota:
        name.append("OTA")
    if voicename:
        name.append("Mbrola %s %s" % (voicename, prsize(blobsize)))
    elif blob:
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
print ("%s.menu.PartitionScheme.%s.upload.maximum_size=%d" % (board, pname, appsize))
print("")
if not vox:
    print("=== Example upload command: ===\nesptool.py -b 921600 write_flash 0x%x '%s'" % (mbrolapos, blob))
else:
    print("=== Set in cofig.h: ===\n#define _data_header(x) __data_header(data/%s_alaw_app/espola##x)" % vox)
exit(0)

