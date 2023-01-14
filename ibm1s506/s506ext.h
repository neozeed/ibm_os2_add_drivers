/*static char *SCCSID = "@(#)s506ext.h	6.3 92/01/21";*/
/****************************************************************************
 *                                                                          *
 *                (c) Copyright  IBM Corporation  1981, 1990                *
 *                           All Rights Reserved                            *
 *                                                                          *
 ****************************************************************************/

/********************* Start of Specifications ******************************
 *                                                                          *
 *  Source File Name: S506EXT.H                                             *
 *                                                                          *
 *  Descriptive Name: Hard Disk 1 Data External References                  *
 *                                                                          *
 *  Copyright:                                                              *
 *                                                                          *
 *  Status:                                                                 *
 *                                                                          *
 *  Function: This file provides external references for global             *
 *            data contained in S506DATA.C                                  *
 *                                                                          *
 *  Notes:                                                                  *
 *    Dependencies:                                                         *
 *    Restrictions:                                                         *
 *                                                                          *
 *  Entry Points:                                                           *
 *                                                                          *
 *  External References:  See EXTRN statements below                        *
 *                                                                          *
 ********************** End of Specifications *******************************/

/****************************************************************************
 *                                                                          *
 *     Static Data                                                          *
 *                                                                          *
 *                                                                          *
 ****************************************************************************/

 extern ACB          acb;              // adapter control block
 extern  SEL         gdt_selector;     // space for two gdt selectors
 extern  SEL         gdt_selector_IO;  // gdt selector used for io operations
 extern struct InfoSegGDT FAR *pGlobalInfoSeg;  // ptr to global infoseg
 extern BYTE         NumFix;           // number of hard disk drives
 extern USHORT       ADDHandle;        // ADDHandle

 extern USHORT       MTick;            // MS/(System Tick)
 extern USHORT       UTick;            // US/(System Tick)
 extern USHORT       TimerConv;
 extern UCHAR        TimerActive;      // bit flags for active timers
 extern USHORT       LongTimeout;

 extern PFN          Device_Help;
 extern USHORT       Init_Complete;
 extern USHORT       LID;

 extern  UCHAR AdapterName[17];  // Adapter Name ASCIIZ string

/****************************************************************************
 *                                                                          *
 *     Area to build Control Blocks                                         *
 *                                                                          *
 *                                                                          *
 ****************************************************************************/

 extern ULONG SafeRBA;

 extern IORB_CONFIGURATION TestIORB;
 extern DEVICETABLE TestDeviceTable;
 extern IORB_UNIT_CONTROL TestIORB2;
 extern UNITINFO TestUnitInfo;
 extern IORB_GEOMETRY TestIORB3;
 extern GEOMETRY TestGeometry;
 extern IORB_UNIT_CONTROL TestIORB4;
 extern IORB_UNIT_STATUS TestIORB5;
 extern IORB_EXECUTEIO TestIORBRead;
 extern SCATGATENTRY TestSGList00;
 extern SCATGATENTRY TestSGList01;
 extern SCATGATENTRY TestSGList02;
 extern SCATGATENTRY TestSGList03;
 extern SCATGATENTRY TestSGList04;
 extern SCATGATENTRY TestSGList05;
 extern SCATGATENTRY TestSGList06;
 extern SCATGATENTRY TestSGList07;
 extern SCATGATENTRY TestSGList08;
 extern SCATGATENTRY TestSGList09;
 extern SCATGATENTRY TestSGList10;
 extern SCATGATENTRY TestSGList11;
 extern SCATGATENTRY TestSGList12;
 extern SCATGATENTRY TestSGList13;
 extern SCATGATENTRY TestSGList14;
 extern SCATGATENTRY TestSGList15;
 extern IORB_EXECUTEIO TestIORBVerify;
 extern SCATGATENTRY TestSGList2;
 extern IORB_EXECUTEIO TestIORBWrite;
 extern SCATGATENTRY TestSGList3;
 extern UCHAR            TestBuffer[];

/****************************************************************************
 *                                                                          *
 *     Initialization Data                                                  *
 *                                                                          *
 *                                                                          *
 ****************************************************************************/

