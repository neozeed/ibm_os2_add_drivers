
ibm2flpy.sys : fl2segs.obj fl2headr.obj fl2math.obj fl2data.obj fl2entry.obj fl2intr.obj fl2trace.obj fl2init.obj fl2iorb.obj fl2geo.obj fl2io.obj fl2fmt.obj
   link /nod /map /exepack /packd /a:16 /far @ibm2flpy.lnk
   mapsym ibm2flpy.map

fl2segs.obj : fl2segs.asm
   masm fl2segs,fl2segs /L;

fl2headr.obj : fl2headr.asm
   masm fl2headr,fl2headr /L;

fl2math.obj : fl2math.asm
   masm fl2math,fl2math /L;

fl2data.obj : fl2data.c fl2data.h
   cl /c /Asnw /G2s /Zp /Fs /Fc fl2data.c

fl2entry.obj : fl2entry.c
   cl /c /Asnw /G2s /Zp /Fs /Fc /NT StaticCode fl2entry.c

fl2intr.obj : fl2intr.c
   cl /c /Asnw /G2s /Zp /Fs /Fc /NT StaticCode fl2intr.c

fl2trace.obj : fl2trace.c
   cl /c /Asnw /G2s /Zp /Fs /Fc /NT InitCode fl2trace.c

fl2init.obj : fl2init.c
   cl /c /Asnw /G2s /Zp /Fs /Fc /NT InitCode fl2init.c

fl2iorb.obj : fl2iorb.c
   cl /c /Asnw /G2s /Zp /Fs /Fc /NT SwapCode fl2iorb.c

fl2geo.obj : fl2geo.c
   cl /c /Asnw /G2s /Zp /Fs /Fc /NT SwapCode fl2geo.c

fl2io.obj : fl2io.c
   cl /c /Asnw /G2s /Zp /Fs /Fc /NT SwapCode fl2io.c

fl2fmt.obj : fl2fmt.c
   cl /c /Asnw /G2s /Zp /Fs /Fc /NT SwapCode fl2fmt.c

