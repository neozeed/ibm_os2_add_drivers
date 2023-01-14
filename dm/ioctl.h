/*static char *SCCSID = "@(#)ioctl.h	6.1 92/01/08";*/
/*static char *SCCSID = "@(#)ioctl.h    6.1 90/11/15";*/
/*static char *SCCSID = "@(#)ioctl.h    13.2 89/02/01";*/
/*
 * ioctl.h      MT-DOS ioctl codes
 */


/**     Major and Minor Codes
 *      Category(Major) and Function(Minor) Codes
 *      -----------------------------------------

 *      The Major and Minor values are each contained in a byte.

 *      Major Code:

 *              0... ....               - Microsoft Defined
 *              1... ....               - Oem/User Defined
 *              .xxx xxxx               - Code


 *      Minor Code:

 *              0... ....               - Return error if unsupported
 *              1... ....               - Ignore if unsupported
 *              .0.. ....               - Intercepted by DOS
 *              .1.. ....               - Passed to driver
 *              ..0. ....               - Function sends data/commands to device
 *              ..1. ....               - Function querys data/info from device
 *              ...x xxxx               - Subfunction


 *      Note that the "sends/querys" data bit is intended only to
 *      regularize the function set.  It plays no critical role; some
 *      functions may contain elements of both command and query.  The
 *      convention is that such commands are defined as "sends data".
 */


/**     Major and Minor Codes           **/


#define IOC_SE   1              /*  serial device control */
#define IOSW_BR ((IOC_SW<<8) | 0x41)            /*  set baud rate */
#define IOSR_BR ((IOC_SR<<8) | 0x61)            /*  return baud rate */
#define IOSW_FC ((IOC_SW<<8) | 0x42)            /*  set frame control (stop bits, parity) */
#define IOSR_FC ((IOC_SR<<8) | 0x62)            /*  return frame control (stop bits, parity) */
#define IOSW_FI ((IOC_SW<<8) | 0x03)            /*  flush input side (seen by DOS and driver) */
#define IOSW_FO ((IOC_SW<<8) | 0x04)            /*  flush output           (seen by DOS and driver) */

#define IOC_TC   2              /*  Terminal Control */
#define IOTW_EC ((IOC_TC<<8) | 0x41)            /*  set echo control */
#define IOTR_EC ((IOC_TC<<8) | 0x61)            /*  get echo control */
#define     TTECHO      02
#define IOTW_EM ((IOC_TC<<8) | 0x42)            /*  set edit mode (raw, cooked) */
#define IOTR_EM ((IOC_TC<<8) | 0x62)            /*  get edit mode (raw, cooked) */
#define     TTRAW       04
#define IOTW_KI ((IOC_TC<<8) | 0x43)            /*  set keyboard intercept characters */
#define IOTR_KI ((IOC_TC<<8) | 0x63)            /*  get keyboard intercept characters */

#define IOC_SC   3              /*  Screen Control */
#define IOSC_LS ((IOC_SC<<8) | 0x41)            /*  Locate SIB */
#define IOSC_SS ((IOC_SC<<8) | 0x42)            /*  save segment */
#define IOSC_RS ((IOC_SC<<8) | 0x43)            /*  restore segment */
#define IOSC_EI ((IOC_SC<<8) | 0x44)            /*  re-enable I/O */
#define IOSC_IS ((IOC_SC<<8) | 0x45)            /*  initialize screen */

#define IOC_KC   4              /*  Keyboard Control */
#define IOKC_LK ((IOC_KC<<8) | 0x41)            /*  Locate KIB */
#define IOKC_SS ((IOC_KC<<8) | 0x42)            /*  save segment */
#define IOKC_RS ((IOC_KC<<8) | 0x43)            /*  restore segment */
#define IOKC_CK ((IOC_KC<<8) | 0x44)            /*  change keyboard images */
#define IOKC_IK ((IOC_KC<<8) | 0x45)            /*  initialize keyboard */
#define IOKC_SL ((IOC_KC<<8) | 0x06)            /*  set console locus */
#define IOKC_RL ((IOC_KC<<8) | 0x07)            /*  reset console locus */


#define IOC_PC   5              /*  printer Control */


#define IOC_LP   6              /*  light pen */


#define IOC_MC   7              /*  mouse Control */


                                // IOCTL Control
#define FT_IOCTL_Cat    0x88
#define FT_IOCTL_Func   0x51


/* Logical Disk Control IOCTL added 08/29/91 DCL */
#define IOC_DC   0x08             // disk control
#define IODC_LK  0x00             // Lock drive
#define IODC_UL  0x01             // unlock drive
#define IODC_RM  0x02             // redetermine media
#define IODC_SL  0x03             // set logical map
#define IODC_BF  0x04             // begin format
#define IODC_BR  0x20             // block removable
#define IODC_GL  0x21             // get logical map
#define IODC_SP  0x43             // set device parameters
#define IODC_WT  0x44             // write track
#define IODC_FT  0x45             // format track
#define IODC_QD  0x5d             // quiesce/restart diskette
#define IODC_MS  0x60             // Media Sense
#define IODC_GP  0x63             // get device parameters
#define IODC_RT  0x64             // READ TRACK
#define IODC_VT  0x65             // verify track

/* Physical Disk Control IOCTL added 08/29/91 DCL */
#define IOC_PD   0x09             // physical disk control
#define IOPD_LK  0x00             // lock physical drive
#define IOPD_UL  0x01             // unlock physical drive
#define IOPD_ED  0x22             // install BDS functions
#define IOPD_RB  0x5e             // readback function
#define IOPD_DO  0x5f             // DMA overrun function


