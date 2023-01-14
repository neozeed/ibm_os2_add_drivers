This is file README.TXT which describes all the zipped files
    in the add toolkit

Notes on files: (Current files are for version 6.304e; Please
                 contact DAP for this latest beta version)

    ADDSPEC is the text of the ADD specification; it has
been updated since the workshop.  Please download for the most
recent additions/modifications/deletions/updates to the spec.
    SCSISPEC is the text which describes how to interface to the
OS2SCSI.DAD module.  (Make sure you read ADDSPEC first)

    Files DDOBJ and ADDOBJ.ZIP are IBM supplied Device Managers (DM)
Adapter Device Drivers (ADD) object code.  The latest versions will be
loaded onto this BBS as they come available.  Please check the dates
for version control.

    Files IBM2FLPY, IBM1S506, IBM2ADSK.ZIP are the zipped versions of
the entire sample files for ADDs.  You are permitted to use any part
of them; make sure you use a unique filename for your add (i.e. anything
other than IBM2ADSK .. IBM2FLPY, etc).  Files with the number 1 are
for ISA machines, and talk to the hardware at the register level.  Files
with the number 2 are for PS/2 machines, and talk to ABIOS.  It is
suggested to first review IBM2FLPY, since it the most easy to read,
and has much useful diagnostic code.

    Files LIBOBJ, XTRAHINC, DISKH, DISKINC.ZIP are support headers/libraries
for the ADDs and DMs above.

    DDMESSAG is sample code for using the SAVE_MESSAGE DevHlp in C.
it should help in permitting Base DDs to display messages to the
console.

DEVELOPMENT TECHNIQUE SUGGESTIONS:
    I suggest that you initially test your ADDs without a complete
device driver stack; i.e. only put the line BASEDEV=YOURADD.SYS
into config.sys.  You will be able to use the following
method to test basic IORB interface to your ADD:

    ADDTEST is the sample code of the ADDTEST that we used in LAB 1 in the
class.  I suggest that you use this facility to test each IORB with your ADD;
in order to do this, you must currently embed the IOCTL code from
IBM2FLPY\FL2ENTRY.C (found in IBM2FLPY.ZIP) into your ADD source code.

    After making sure that you have the IORB protocol operating
correctly, then install a full driver stack via CONFIG.SYS; the
'correct' way of doing this would be (in CONFIG.SYS):

BASEDEV=YOURADD.ADD
BASEDEV=BOOTADD.ADD (i.e. IBM2ADSK.ADD)
BASEDEV=OS2DASD.DMD


Extra goodies: (not necessarily related to ADDs, but interesting...)
    SHOWDEV is an application/device driver
team which searches the device driver stack, and displays the
attributes of each DD on a full screen session.  This may prove
useful in the field to diagnose customer installations.
SHOWDEV.SYS must be in config.sys (DEVICE = SHOWDEV.SYS) in order
for this DD search to take place.
    CSERVE is a summary of all the files on Compu Serve.  It is listed
here to give you an idea of the breadth of PD OS/2 software.  Feel
free to sign up to CServe and help yourself!
    PKUNZIP.EXE is a file de-compression utility.  You'll need it to
'unzip' compressed files on this BBS.


General comments:
    Feel free to modify this *sample* code...  if you come up with
a good idea you wish to share, please upload your code on this
BBS, along with a readme file which explains your improvements.
As an examples:
1. note that CDB passthru test code doesn't currently exist in ADDTEST.C;
if you find it useful, please add it.
2. turn FL2ENTRY.C into a filter ADD to facilitate testing multiple ADDS.

    The code posted on this BBS is Sample Code...  Source code
is not available. If you have any other Sample Code
requirements, please let me know.


Thanks,
Rudy
