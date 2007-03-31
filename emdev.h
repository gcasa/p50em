/* emdev.h, Jim Wilcoxson (prirun@gmail.com), April 17, 2005
   Device handlers for pio instructions.  Use devnull as a template for
   new handlers.

   NOTES:

   OCP instructions never skip
   SKS instructions skip on specified conditions
   INA/OTA instructions skip if they succeed (data was read/written)

   Device numbers:
   '00 = polling (?)
   '01 = paper tape reader
   '02 = paper tape punch
   '03 = #1 MPC/URC (line printer/card reader/card punch)
   '04 = SOC/Option A/VCP board (system console/user terminal)
   '05 = #2 MPC/URC (line printer/card reader/card punch)
   '06 = card punch? (RTOS User Guide, A-1) / IPC (Engr Handbook p.101)
   '06 = Interproc. Channel (IPC) (R20 Hacker's Guide)
   '07 = #1 PNC
   '10 = ICS2 #1 or ICS1
   '11 = ICS2 #2 or ICS1
   '12 = floppy disk/diskette (magtape controller #3 at rev 22.1?)
   '13 = #2 magtape controller
   '14 = #1 magtape controller
   '15 = #5 AMLC or ICS1
   '16 = #6 AMLC or ICS1
   '17 = #7 AMLC or ICS1
   '20 = control panel / real-time clock
   '21 = 1st 4002 (Option B') disk controller
   '22 = disk #3
   '23 = disk #4
   '24 = disk (was Writable Control Store)
   '25 = disk (was 4000 disk controller)
   '26 = #1 disk controller
   '27 = #2 disk controller
   '30-32 = BPIOC #1-3 (RTOS User Guide, A-1)
   '32 = AMLC #8 or ICS1
   '33 = #1 Versatec
   '34 = #2 Versatec
   '35 = #4 AMLC or ICS1
   '36-37 = ELFBUS #1 & 2 (ICS1 #1, ICS1 #2)
   '40 = A/D converter type 6000
   '41 = digital input type 6020
   '42 = digital input #2
   '43 = digital output type 6040
   '44 = digital output #2
   '45 = disk (was D/A converter type 6060 (analog output) - obsolete)
   '46 = disk
   '47 = #2 PNC
   '50 = #1 HSSMLC/MDLC (synchronous comm)
   '51 = #2 HSSMLC/MDLC
   '52 = #3 AMLC or ICS1
   '53 = #2 AMLC 
   '54 = #1 AMLC
   '55 = MACI autocall unit
   '56 = old SMLC (RTOS User Guide, A-1 & Hacker's Guide)
   '60-67 = reserved for user devices (GPIB)
   '70-'73 = Megatek graphics terminals
   '75-'76 = T$GPPI

   Devices emulated by Primos in ks/ptrap.ftn for I/O instructions in Ring 3:
     '01 = paper tape reader
     '02 = paper tape punch
     '04 = console
     '20 = control panel lights & sense switches
*/

#include <errno.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/select.h>
#include <unistd.h>
#include <termios.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <time.h>
#include <sys/file.h>

/* this macro is used when I/O is successful.  In VI modes, it sets
   the EQ condition code bit.  In SR modes, it does a skip */

#define IOSKIP \
  if (crs[KEYS] & 010000) \
    crs[KEYS] |= 0100; \
  else \
    RPL++

/* this macro is used to decide whether blocking should be enabled for
   an I/O operation.  The rules are:
   - if process exchange is disabled then it's safe to block
   - if the instruction (assumed to be I/O) is followed by JMP *-1 (SR
     modes) or BCNE *-2 (VI modes), then it's okay to block
*/

#if 1
#define BLOCKIO \
  (!(crs[MODALS] & 010) && (iget16(RP) == 03776 || (iget16(RP) == 0141603 && iget16(RP+1) == RPL-2)))
#else
#define BLOCKIO 0
#endif



/* this is a template for new device handlers */

int devnew (int class, int func, int device) {

  switch (class) {

  case -1:
    return 0;

  case 0:
    TRACE(T_INST, " OCP '%02o%02o\n", func, device);
    if (func == 99) {
      ;
    } else {
      printf("Unimplemented OCP device '%02o function '%02o\n", device, func);
      fatal(NULL);
    }
    break;

  case 1:
    TRACE(T_INST, " SKS '%02o%02o\n", func, device);
    if (func == 99)
      IOSKIP;                     /* assume it's always ready */
    else {
      printf("Unimplemented SKS device '%02o function '%02o\n", device, func);
      fatal(NULL);
    }
    break;

  case 2:
    TRACE(T_INST, " INA '%02o%02o\n", func, device);
    if (func == 99) {
      ;
    } else {
      printf("Unimplemented INA device '%02o function '%02o\n", device, func);
      fatal(NULL);
    }
    break;

  case 3:
    TRACE(T_INST, " OTA '%02o%02o\n", func, device);
    if (func == 99) {
      IOSKIP;
    } else {
      printf("Unimplemented OTA device '%02o function '%02o, A='%o\n", device, func, crs[A]);
      fatal(NULL);
    }
    break;
  }
}


/* this is a template for null (not present) devices */

int devnone (int class, int func, int device) {

  static int seen[64] = {64*0};

  switch (class) {

  case -1:
    return 0;

  case 0:
    TRACE(T_INST, " OCP '%02o%02o\n", func, device);
    break;

  case 1:
    TRACE(T_INST, " SKS '%02o%02o\n", func, device);
    break;

  case 2:
    TRACE(T_INST, " INA '%02o%02o\n", func, device);
    break;

  case 3:
    TRACE(T_INST, " OTA '%02o%02o\n", func, device);
    break;
  }
  if (!seen[device])
    fprintf(stderr, " pio to unimplemented device '%o, class '%o, func '%o\n", device, class, func);
  seen[device] = 1;
}


/* Device '4: system console

   NOTES:
   - this driver only implements the basic needs of the system console
   - needs to reset tty attributes when emulator shuts down
   - Primos only handles ASRATE 110 (10cps), 1010 (30cps), 3410 (960 cps)
   - input ID is wrong, causes OCP '0477 on clock interrupt
   - issues with setting blocking flags & terminal attributes: doesn't block

   OCP '0004 = initialize for input only, echoplex, 110 baud, 8-bit, no parity
   OCP '0104 = same, but for output only
   OCP '0204 = set receive interrupt mask
   OCP '0304 = enable receive DMA/C
   OCP '0404 = reset receive interrupt mask and DMA/C enable
   OCP '0504 = set transmit interrupt mask
   OCP '0604 = enable transmit DMA/C
   OCP '0704 = reset transmit interrupt mask and DMA/C enable
   OCP '1004 = Full duplex; software must echo
   OCP '1104 = output a sync pulse (diagnostics) 
   OCP '1204 = Prime normal, independent xmit and recv w/echoplex
   OCP '1304 = Self test mode (internally connects transmitter to receiver)
   OCP '1504 = Set both xmit & rcv interrupt masks
   OCP '1604 = Reset both xmit & rcv interrupt masks
   OCP '1704 = same as '0004, but clears interrupt masks and dma/c enables
               (this is the state after Master Clear)

   SKS '0004 = skip if either receive or xmit ready, whichever are enabled
   SKS '0104 = skip if not busy
   SKS '0204 = skip if receiver not interrupting
   SKS '0304 = skip if control registers valid
   SKS '0404 = skip if neither xmit nor recv are interrupting
   SKS '0504 = skip if xmit not interrupting
   SKS '0604 = skip is xmit ready (can accept a character)
   SKS '0704 = skip if recv ready (character present)
   SKS '1104-'1404 = skip if input bit 1/2/3/4 marking
   SKS '1504 = skip if parity error
   SKS '1604 = skip if character overrun
   SKS '1704 = skip if framing error

   INA '0004 = "or" character into right byte of A, not modifying left byte
   INA '0404 = input receive control register 1 (always works)
   INA '0504 = input receive control register 2 (always works)
   INA '0604 = input xmit control register 1 (always works)
   INA '0704 = input xmit control register 2 (always works)
   INA '1004 = like '0004, but clear A first
   INA '1104 = input device ID
   INA '1404 = input recv DMA/C channel address (these last 4 always work)
   INA '1504 = input xmit DMA/C channel address
   INA '1604 = input recv interrupt vector
   INA '1704 = input xmit interrupt vector

   OTA '0004 = xmit character from A; fails if inited for recv only
   OTA '0404 = output rcv CR 1
   OTA '0504 = output rcv CR 2
   OTA '0604 = output xmit CR 1
   OTA '0704 = output xmit CR 2
   OTA '1404 = output recv DMA/C channel address (these last 4 always work)
   OTA '1504 = output xmit DMA/C channel address
   OTA '1604 = output recv interrupt vector
   OTA '1704 = output xmit interrupt vector

*/


int devasr (int class, int func, int device) {

  static FILE *conslog;
  static int ttydev;
  static int ttyflags;
  static int needflush;     /* true if data has been written but not flushed */
  static struct termios terminfo;
  static fd_set fds;
  static short vcptime[8] = {7*0, 1};
  static short vcptimeix;

  struct timeval timeout;
  unsigned char ch;
  int newflags;
  int n;
  int doblock;
  time_t unixtime;
  struct tm *tms;
  
  doblock = BLOCKIO;

  switch (class) {

  case -1:
    setsid();
    ttydev = open("/dev/tty", O_RDWR, 0);
    if (ttydev < 0) {
      perror(" error opening /dev/tty");
      fatal(NULL);
    }
    if (fcntl(ttydev, F_GETFL, ttyflags) == -1) {
      perror(" unable to get tty flags");
      fatal(NULL);
    }
    FD_ZERO(&fds);
    if (tcgetattr(ttydev, &terminfo) == -1) {
      perror(" unable to get tty attributes");
      fatal(NULL);
    }

    /* NOTE: some of these are not restored by the host OS after the
       emulator is suspended (VSUSP) then restarted, eg, the VSUSP and
       VINTR characters */

    terminfo.c_iflag &= ~(INLCR | ICRNL);
    terminfo.c_lflag &= ~(ECHOCTL | ICANON);
    terminfo.c_oflag &= ~(TOSTOP);
    terminfo.c_cc[VINTR] = _POSIX_VDISABLE;  /* use ^\ instead */
    terminfo.c_cc[VSUSP] = '';
    terminfo.c_cc[VMIN] = 0;
    terminfo.c_cc[VTIME] = 0;
    if (tcsetattr(ttydev, TCSANOW, &terminfo) == -1) {
      perror(" unable to set tty attributes");
      fatal(NULL);
    }

    /* ignore SIGTTIN in case the emulator is put in the background */

    signal(SIGTTIN, SIG_IGN);

    /* open console log file and set to line buffering */

    if ((conslog = fopen("console.log", "w")) == NULL) {
      perror(" unable to open console log file");
      fatal(NULL);
    }
    setvbuf(conslog, NULL, _IOLBF, 0);
    return 0;

  case 0:
    TRACE(T_INST, " OCP '%02o%02o\n", func, device);
    break;

  case 1:
    TRACE(T_INST, " SKS '%02o%02o\n", func, device);

    if (func == 6) {               /* skip if room for a character */
      if (crs[MODALS] & 010)       /* PX enabled? */
	timeout.tv_sec = 0;        /* yes, can't delay */
      else
	timeout.tv_sec = 1;        /* single user: okay to delay */
      timeout.tv_usec = 0;
      FD_SET(ttydev, &fds);
      n = select(ttydev+1, NULL, &fds, NULL, &timeout);
      if (n == -1) {
	perror(" unable to do write select on tty");
	fatal(NULL);
      }
      if (n) {
	IOSKIP;
      }

    } else if (func == 7) {        /* skip if received a char */
      if (crs[MODALS] & 010)       /* PX enabled? */
	timeout.tv_sec = 0;        /* yes, can't delay */
      else
	timeout.tv_sec = 1;        /* single user: okay to delay */
      timeout.tv_usec = 0;
      FD_SET(ttydev, &fds);
      n = select(ttydev+1, &fds, NULL, NULL, &timeout);
      if (n == -1) {
	perror(" unable to do read select on tty");
	fatal(NULL);
      }
      if (n) {
	IOSKIP;
      }

    } else if (func <= 014)
      IOSKIP;                     /* assume it's always ready */
    break;

  /* signal SIGTTIN is ignore during initialization, so if the
     emulator is put in the background, read() will return EIO */

  case 2:
    TRACE(T_INST, " INA '%02o%02o\n", func, device);
    //TRACE(T_INST, "INA, RPH=%o, RPL=%o, [RP]=%o, [RP+1]=%o, BLOCKIO=%d\n", RPH, RPL, iget16(RP), iget16(RP+1),BLOCKIO);
    if (func == 0 || func == 010) {         /* read a character */
      if (doblock)
	newflags = ttyflags & ~O_NONBLOCK;
      else
	newflags = ttyflags | O_NONBLOCK;
      if (newflags != ttyflags && fcntl(ttydev, F_SETFL, newflags) == -1) {
	perror(" unable to set tty flags");
	fatal(NULL);
      }
      ttyflags = newflags;
      if (doblock && needflush) {
	if (fflush(stdout) == 0) {
	  needflush = 0;
	  devpoll[device] = 0;
	}
	fflush(conslog);
      }
readasr:
      n = read(ttydev, &ch, 1);
      if (n == 0) {
	if (doblock) {
	  usleep(500000);
	  goto readasr;
	}
      } else if (n < 0) {
	if (errno != EAGAIN && errno != EIO) {
	  perror(" error reading from tty");
	  fatal(NULL);
	}
      } else if (n == 1) {
	if (ch == '') {
	  if (savetraceflags == 0) {
	    TRACEA("\nTRACE ENABLED:\n\n");
	    savetraceflags = ~TB_MAP;
	    savetraceflags = ~0;
	  } else {
	    TRACEA("\nTRACE DISABLED:\n\n");
	    dumpsegs();
	    savetraceflags = 0;
	  }
	  fflush(tracefile);
	  goto readasr;
	}
	if (func >= 010)
	  crs[A] = 0;
	crs[A] = crs[A] | ch;
	TRACE(T_INST, " character read=%o: %c\n", crs[A], crs[A] & 0x7f);
	if (ch != 015) {         /* log all except carriage returns */
	  fputc(ch, conslog);
	  fflush(conslog);       /* immediately flush typing echos */
	}
	IOSKIP;
      } else {
	printf("Unexpected error reading from tty, n=%d\n", n);
	fatal(NULL);
      }
    } else if (04 <= func && func <= 07) {  /* read control register 1/2 */
      crs[A] = 0;
      IOSKIP;
    } else if (func == 011) {    /* read device id? */
      crs[A] = 4;
      IOSKIP;
    } else if (func == 012) {    /* read control word */
      crs[A] = 04110;
      IOSKIP;
    } else if (func == 017) {    /* read xmit interrupt vector -OR- clock */
      crs[A] = vcptime[vcptimeix++];
      if (vcptimeix > 7)
	vcptimeix = 0;
      IOSKIP;
    } else {
      printf("Unimplemented INA '04 function '%02o\n", func);
      fatal(NULL);
    }
    break;

  case 3:
    TRACE(T_INST, " OTA '%02o%02o\n", func, device);
    if (func == 0) {
      ch = crs[A] & 0x7f;
      TRACE(T_INST, " char to write=%o: %c\n", crs[A], ch);
      if (ch == 0 || ch == 0x7f) {
	IOSKIP;
	return;
      }
#if 0
      /* could do this here too, but Primos does it with SKS before
	 doing the OTA for the console, so it isn't really necessary.
	 Also, if done here, it might confused a program if SKS said
	 the character could be output, then OTA said it couldn't.
	 The program might stay in a tight OTA loop and hang the
	 machine until sys console output clears */

      timeout.tv_sec = 0;
      timeout.tv_usec = 0;
      FD_SET(2, &fds);
      n = select(2+1, NULL, &fds, NULL, &timeout);
      if (n == -1) {
	perror(" unable to do write select on stdout");
	fatal(NULL);
      }
      if (!n)
	return;
#endif
      putchar(ch);
      if (ch != 015)
	putc(ch, conslog);
      needflush = 1;
      if (devpoll[device] == 0)
	devpoll[device] = instpermsec*100;
      IOSKIP;
    } else if (func == 1) {       /* write control word */
      IOSKIP;
    } else if (04 <= func && func <= 07) {  /* write control register 1/2 */
      IOSKIP;
    } else if (func == 013) {
      /* NOTE: does this in rev 20 on settime command (set clock on VCP?) */
      IOSKIP;

    } else if (func == 017) {
      if (crs[A] == 0) {

	/* setup to read VCP battery backup clock (only on certain models);
	   all words are 2 BCD digits */

#define BCD2(i) ((((i)/10)<<4) | ((i)%10))

	unixtime = time(NULL);
	tms = localtime(&unixtime);
	vcptime[0] = BCD2(tms->tm_year);
	vcptime[1] = BCD2(tms->tm_mon+1);
	vcptime[2] = BCD2(tms->tm_mday);
	vcptime[3] = BCD2(tms->tm_wday);
	vcptime[4] = BCD2(tms->tm_hour);
	vcptime[5] = BCD2(tms->tm_min);
	vcptime[6] = BCD2(tms->tm_sec);
	vcptime[7] = 0;
	vcptimeix = 0;
      }
      IOSKIP;
    } else {
      printf("Unimplemented OTA device '%02o function '%02o, A='%o\n", device, func, crs[A]);
      fatal(NULL);
    }
    break;

  case 4:

    /* tty output is blocking, even under Primos, which means that
       writes and fflush can hang the entire system, eg, if XOFF
       happens while writing to the console).  Console output should
       be changed to non-blocking one of these days... */

    if (needflush) {
      if (fflush(stdout) == 0)
	needflush = 0;
      else
	devpoll[device] = instpermsec*100;
      fflush(conslog);
    }
  }
}




/* Device '14 - magtape controller #1

   NOTES: 

   This code only supports 1 tape controller (4 units), but could be
   extended to support 2 controllers (eg, see disk controller code)

   All tape files are assumed to be in SimH .TAP format:
   - 4-bytes (little endian integer) indicating data record length (in bytes)
   - n bytes of data
   - repeat of the 4-byte data record length
   - odd length records have a pad byte inserted
   - if MSB of record length is set, there is an error in the record
   - record length of zero indicates a file mark (doesn't repeat)
   - record length of all 1's indicates end-of-tape mark (doesn't repeat)

   Tape files are named according to the device and unit, like disk drives:
   - dev14u0, dev14u1, dev14u2, dev14u3
   These names are usually just links to the actual tape file.

   Tape status word bits (MSB to LSB):
    1 = Vertical Parity Error
    2 = Runaway
    3 = CRC Error
    4 = LRCC Error
    5 = False Gap or Insufficient DMX Range
    6 = Uncorrectable Read Error
    7 = Raw Erorr
    8 = Illegal Command
    9 = Selected Tape Ready (online and not rewinding)
   10 = Selected Tape Online
   11 = Selected Tape is at End Of Tape (EOT)
   12 = Selected Tape is Rewinding
   13 = Selected Tape is at Beginning Of Tape (BOT)
   14 = Selected Tape is Write Protected
   15 = DMX Overrun
   16 = Rewind Interrupt

   OTA '01 Motion control A-register bits:
    1 = Select Tape Only
    2 = 0 for File operation, 1 for Record operation
    3 = 1 for Space operation
    4 = 1 for Read & Correct
    5 = 1 for 7-track
    6 = 1 for 9-track
    7 = unused
    8 = 1 for 2 chars/word, 0 for 1 char/word (not supported here)
    9-11 = motion:  100 = Forward,  010 = Reverse,  001 = Rewind
    12 = 1 for Write
    13 = 1 for Unit 0
    14 = 1 for Unit 1
    15 = 1 for Unit 2
    16 = 1 for Unit 3

*/

int mtread (int fd, unsigned short *iobuf, int nw, int fw, int *mtstat) {
  unsigned char buf[4];
  int n,reclen,reclen2,bytestoread;

  TRACE(T_TIO, " mtread, nw=%d, initial tape status is 0x%04x\n", nw, *mtstat);
  if (fw) {
    if (*mtstat & 0x20)             /* already at EOT, can't read */
      return 0;
    n = read(fd, buf, 4);
    TRACE(T_TIO, " mtread read foward, %d bytes for reclen\n", n);
    if (n == 0) {                   /* now we're at EOT */
      *mtstat |= 0x20;
      return 0;
    }
    *mtstat &= ~8;                   /* not at BOT now */
readerr:
    if (n == -1) {
      perror("Error reading from tape file");
      *mtstat = 0;                   /* take drive offline */
      return 0;
    }
    if (n < 4) {
      fprintf(stderr," only read %d bytes for reclen\n", n);
fmterr:
      warn("Tape file isn't in .TAP format");
      *mtstat = 0;
      return 0;
    }
    reclen = buf[0] | (buf[1]<<8) | (buf[2]<<16) | (buf[3]<<24);
    TRACE(T_TIO,  " mtread reclen = %d bytes\n", reclen);
    if (reclen == 0) {             /* hit a file mark */
      *mtstat |= 0x100;
      return 0;
    }
    if (reclen == 0xFFFF) {        /* hit EOT mark */

      /* NOTE: simh says to backup here, probably to wipe out EOT if
       more data is written.  IMO, EOT should never be written to
       simulated tape files. */

      if (lseek(fd, -4, SEEK_CUR) == -1) {
	perror("em: unable to backspace over EOT");
	goto readerr;
      }
      *mtstat |= 0x20;
      return 0;
    }
    if (reclen & 0x8000) {         /* record marked in error */
      /* NOTE: simh may have non-zero record length here... */
      *mtstat |= 0xB600;           /* set all error bits */;
      return 0;
    }
    if (reclen & 1)
      warn("odd-length record in tape file!");
    if (reclen < 0) {
      fprintf(stderr," negative record length %d\n", reclen);
      goto fmterr;
    }

    /* now either position or read forward */

    if (nw == 0) {                 /* spacing only */
      if ((n=lseek(fd, reclen, SEEK_CUR)) == -1) {
	perror("em: unable to forward space record");
	goto fmterr;
      } else {
	TRACE(T_TIO, " spaced forward %d bytes to position %d\n", reclen, n);
      }
    } else {
      if ((reclen+1)/2 > nw) {
	fprintf(stderr,"em: reclen = %d bytes in tape file - too big!\n", reclen);
	*mtstat |= 2;              /* set DMX overrun status */
	bytestoread = nw*2;
      } else {
	bytestoread = reclen;
      }
      n = read(fd, iobuf, bytestoread);
      TRACE(T_TIO, " mtread read %d/%d bytes of data \n", n, reclen);
      if (n == -1) goto readerr;
      if (n != bytestoread) goto fmterr;
      if (bytestoread != reclen) {     /* skip the rest of the record */
	if (lseek(fd, reclen-bytestoread, SEEK_CUR) == -1) {
	  fprintf(stderr,"em: unable to handle large record\n");
	  goto readerr;
	}
      }
    }

    /* now get the trailing record length */

    n = read(fd, buf, 4);
    TRACE(T_TIO, " mtread read %d bytes for trailer reclen\n", n);
    if (n == -1) goto readerr;
    if (n != 4) goto fmterr;
    reclen2 = buf[0] | (buf[1]<<8) | (buf[2]<<16) | (buf[3]<<24);
    if (reclen2 != reclen) goto fmterr;
    /* XXX: maybe should pad odd-length record with a zero... */
    return (bytestoread+1)/2;

  } else {

    /* spacing backward, see if we're at BOT */

    if ((*mtstat & 8) || (lseek(fd, 0, SEEK_CUR) == 0)) {
      *mtstat |= 8;                    /* yep, at BOT */
      return 0;
    }

    /* backup 4 bytes, read reclen */

    if (lseek(fd, -4, SEEK_CUR) == -1)
      goto fmterr;
    n = read(fd, buf, 4);
    if (n == -1) goto readerr;
    if (n != 4) goto fmterr;
    reclen = buf[0] | (buf[1]<<8) | (buf[2]<<16) | (buf[3]<<24);

    /* backup reclen+8 bytes unless this is a file mark, error,
       or EOT */

    if (reclen == 0) {
      *mtstat |= 0x100;        /* set filemark status */
      goto repo;
    }
    if (reclen & 0x8000)       /* error record (don't report) */
      goto repo;
    if (reclen == 0xFFFF) {
      reclen = 0;
      goto repo;
    }
    if (lseek(fd, -(reclen+8), SEEK_CUR) == -1)
      goto fmterr;

    /* read leading reclen again to make sure we're positioned correctly */
      
    n = read(fd, buf, 4);
    if (n == -1) goto readerr;
    if (n != 4) goto fmterr;
    reclen2 = buf[0] | (buf[1]<<8) | (buf[2]<<16) | (buf[3]<<24);
    if (reclen2 != reclen) goto fmterr;

    /* finally, backup over the reclen to be positioned for read */

repo:

    if ((n = lseek(fd, -4, SEEK_CUR)) == -1)
      goto readerr;
    if (n == 0)
      *mtstat |= 8;                    /* now at BOT */
    return 0;
  }
}

int mtwrite (int fd, unsigned short *iobuf, int nw, int *mtstat) {
  int n;

  n = write(fd, iobuf, nw*2);
  if (nw*2 != n) {
    perror("Error writing to tape file");
    fatal(NULL);
  }
  *mtstat &= ~8;                     /* not at BOT now */
}

int devmt (int class, int func, int device) {

  static unsigned short mtvec = 0114;          /* interrupt vector */
  static unsigned short dmxchan = 0;           /* dmx channel number */
  static unsigned short datareg = 0;           /* data holding register */
  static unsigned short ready = 0;             /* true if datareg valid */
  static unsigned short enabled = 0;           /* interrupts enabled */
  static unsigned short interrupting = 0;      /* true if interrupt pending */
  static unsigned short usel = 0;              /* last unit selected */
  static struct {
    int fd;                           /* tape file descriptor */
    int mtstat;                       /* last tape status */
    int firstwrite;                   /* true if next write is the first */
  } unit[4];

  int u;
  char devfile[8];

  /* the largest rec size Primos ever supported is 16K bytes, plus 8
   bytes for the 4-byte .TAP format record length at the beginning &
   end of each record */

#define MAXTAPEWORDS 8*1024

  unsigned short iobuf[MAXTAPEWORDS+4]; /* 16-bit WORDS! */
  unsigned short *iobufp;
  unsigned short dmxreg;                /* DMA/C register address */
  unsigned short tempdmxchan;           /* temp to incr during xfer */
  short dmxnch;                         /* number of DMX channels - 1 */
  unsigned short dmxaddr;
  unsigned long dmcpair;
  short dmxnw, dmxtotnw;
  int i,n;
  char reclen[4];

  switch (class) {

  case -1:                    /* initialize emulator device */
    for (u=0; u<4; u++) {
      unit[u].fd = -1;
      unit[u].mtstat = 0;
      unit[u].firstwrite = 1;
    }
    return 0;

  case 0:
    TRACE(T_INST|T_TIO, " OCP '%02o%02o\n", func, device);

    if (func == 012 || func == 013) {     /* set normal/diag mode - ignored */
      ;

    } else if (func == 014) {             /* ack interrupt */
      /* this is a hack because Primos acks interrupts immediately after
	 OTA 01 (due to a tape controller bug) */
      if (interrupting)
	interrupting--;

    } else if (func == 015) {             /* set interrupt mask */
      enabled = 1;

    } else if (func == 016) {             /* reset interrupt mask */
      enabled = 0;

    } else if (func == 017) {             /* initialize */
      mtvec = 014;
      dmxchan = 0;
      datareg = 0;
      interrupting = 0;
      ready = 0;
      usel = 0;

    } else {
      printf("Unimplemented OCP device '%02o function '%02o\n", device, func);
      fatal(NULL);
    }
    break;

  case 1:
    TRACE(T_INST|T_TIO, " SKS '%02o%02o\n", func, device);
    if (func == 00) {                      /* skip if ready */
      if (ready)
	IOSKIP;
    } else if (func == 01) {               /* skip if not busy */
      IOSKIP;
    } else if (func == 04) {               /* skip if not interrupting */
      if (!interrupting)
	IOSKIP;
    } else {
      printf("Unimplemented SKS device '%02o function '%02o\n", device, func);
      fatal(NULL);
    }
    break;

  case 2:
    TRACE(T_INST|T_TIO, " INA '%02o%02o\n", func, device);
    if (func == 0) {
#if 0
      if (!ready) warn("INA 00 on tape device w/o matching OTA!");
#endif
      crs[A] = datareg;
      datareg = 0;
      ready = 0;
      IOSKIP;

    } else {
      printf("Unimplemented INA device '%02o function '%02o\n", device, func);
      fatal(NULL);
    }
    break;

  case 3:
    TRACE(T_INST|T_TIO, " OTA '%02o%02o, A='%06o %04x\n", func, device, crs[A], crs[A]);

#if 0
    /* don't accept any OTA's if we're interrupting */

    if (interrupting)
      break;
#endif

    if (func == 01) {

      /* here's the hard part where everything happens... decode unit first */

      u = crs[A] & 0xF;
      if (u == 1) u = 3;
      else if (u == 2) u = 2;
      else if (u == 4) u = 1;
      else if (u == 8) u = 0;
      else
	fatal("em: no unit selected on tape OTA '01");
      usel = u;

      /* XXX: if the tape device file changes (inode changes?), close
	 and re-open the file (new tape reel).  Does offline need to
	 be reported at least once in this case?  Or should it somehow
	 be related to rewinds? (only do the check at BOT?) */

#if 0
      if (unit[u].mtstat & 8 && /* tape device file changed */) {
	/* close tape fd if open & set to -1 */
      }
#endif

      /* if the tape file has never been opened, do it now. */

      if (unit[u].fd == -1) {
	unit[u].mtstat = 0;
	unit[u].firstwrite = 1;
	snprintf(devfile,sizeof(devfile),"dev%ou%d", device, u);
	TRACE(T_TIO, " filename for tape dev '%o unit %d is %s\n", device, u, devfile);
	if ((unit[u].fd = open(devfile, O_RDWR, 0770)) == -1) {
	  fprintf(stderr,"em: unable to open tape device file %s for device '%o unit %d for read/write\n", devfile, device, u);
	  IOSKIP;
	  break;
	}
	unit[u].mtstat = 0x00C8;   /* Ready, Online, BOT */
      }
      
      /* "select only" is ignored.  On a real tape controller, this
       blocks (I think) if the previous tape operation is in progress */

      if (crs[A] & 0x8000) {
	TRACE(T_TIO, " select only\n");
	IOSKIP;
	break;
      }

      /* clear "last operation" error bits in mtstat, but preserve status
	 things like "at BOT", "at EOT", "write protected" */

      unit[u].mtstat &= 0x00EC;

      /* for rewind, read, write, & space, setup a completion
	 interrupt if controller interrupts are enabled.  NOTE: there
	 is a race condition here.  Immediately following OTA 01,
	 Primos clears pending interrupts because of an old tape
	 controller bug.  To get around this, "interrupting" is a
	 counter and is set to 2 so that when Primos clears
	 interrupts, the counter is decremented in this driver, but
	 the interrupt will still occur later */

      interrupting = 2;
      devpoll[device] = 10;

      if ((crs[A] & 0x00E0) == 0x0020) {       /* rewind */
	//traceflags = ~TB_MAP;
	TRACE(T_TIO, " rewind\n");
	if (lseek(unit[u].fd, 0, SEEK_SET) == -1) {
	  perror("Unable to rewind tape drive file");
	  fatal(NULL);
	}
	unit[u].mtstat = 0x00C8;    /* Ready, Online, BOT */
	IOSKIP;
	break;
      }

      /* Now we're reading, writing, or spacing either a record or file:
	 - space file/record, forward or backward = okay
	 - read record = okay, read file doesn't make sense
	 - write record = okay, write file = write file mark */


      /* write file mark
	 NOTE: the tape file should probably be truncated on the first
	 write, not on each file mark, although this does let us
	 create garbage tape images for testing by doing a Magsav,
	 rewind, starting another Magsav, and aborting it. */

      if ((crs[A] & 0x4010) == 0x0010) {
	TRACE(T_TIO, " write file mark\n");
	*(int *)iobuf = 0;
	mtwrite(unit[u].fd, iobuf, 2, &unit[u].mtstat);
	ftruncate(unit[u].fd, lseek(unit[u].fd, 0, SEEK_CUR));
	IOSKIP;
	break;
      }

      /* space forward or backward a record or file at a time */

      if (crs[A] & 0x2000) {           /* space operation */
	if ((crs[A] & 0xC0) == 0)
	  warn("Motion = 0 for tape spacing operation");
	else if (crs[A] & 0x4000) {    /* record operation */
	  TRACE(T_TIO, " space record, dir=%x\n", crs[A] & 0x80);
	  mtread(unit[u].fd, iobuf, 0, crs[A] & 0x80, &unit[u].mtstat);
	} else {                       /* file spacing operation */
	  TRACE(T_TIO, " space file, dir=%x\n", crs[A] & 0x80);
	  do {
	    mtread(unit[u].fd, iobuf, 0, crs[A] & 0x80, &unit[u].mtstat);
	  } while (!(unit[u].mtstat & 0x128));  /* FM, EOT, BOT */
	}
	IOSKIP;
	break;
      }

      /* read/write backward aren't supported */

      if (((crs[A] & 0x00E0) == 0x0040) && ((crs[A] & 0x2000) == 0)) {
	warn("em: read/write reverse not supported for tapes");
	unit[u].mtstat = 0;
	IOSKIP;
	break;
      }

      /* for read command, fill the io buffer first.  Note that mtread
	 doesn't store record lengths in iobuf and the return value is
	 the actual data record length in words.  It also ensures that
	 the return value is never > the # of words requested (MAXTAPEWORDS)

         for write commands, the 4-byte .TAP record length IS stored in
	 iobuf and the length returned by mtwrite reflects that */

      if (crs[A] & 0x10) {         /* write record */
	dmxtotnw = 0;
	iobufp = iobuf+2;
      } else {
	TRACE(T_TIO, " read record\n");
	dmxtotnw = mtread(unit[u].fd, iobuf, MAXTAPEWORDS, 1, &unit[u].mtstat);
	iobufp = iobuf;
      }

      /* data transfer from iobuf (read) or to iobuf (write) */

      while (1) {
	dmxreg = dmxchan & 0x7FF;
	if (dmxchan & 0x0800) {         /* DMC */
	  dmcpair = get32r0(dmxreg);    /* fetch begin/end pair */
	  dmxaddr = dmcpair>>16;
	  dmxnw = (dmcpair & 0xffff) - dmxaddr + 1;
	  TRACE(T_INST|T_TIO,  " DMC channels: ['%o]='%o, ['%o]='%o, nwords=%d\n", dmxreg, dmxaddr, dmxreg+1, (dmcpair & 0xffff), dmxnw);
	} else {                        /* DMA */
	  dmxreg = dmxreg << 1;
	  dmxnw = regs.sym.regdmx[dmxreg];
	  if (dmxnw <= 0)
	    dmxnw = -(dmxnw>>4);
	  else
	    dmxnw = -((dmxnw>>4) ^ 0xF000);
	  dmxaddr = regs.sym.regdmx[dmxreg+1];
	  TRACE(T_INST|T_TIO,  " DMA channels: ['%o]='%o, ['%o]='%o, nwords=%d\n", dmxreg, regs.sym.regdmx[dmxreg], dmxreg+1, dmxaddr, dmxnw);
	}
	if (dmxnw < 0) {            /* but is legal for >32K DMC transfer... */
	  printf("devmt: requested negative DMX of size %d\n", dmxnw);
	  fatal(NULL);
	}
	if (crs[A] & 0x10) {            /* write record */
	  if (dmxtotnw+dmxnw > MAXTAPEWORDS)
	    fatal("Tape write is too big");
	  for (n=dmxnw; n > 0; n--) {
	    *iobufp++ = get16r0(dmxaddr+i);
	  }
	  dmxtotnw = dmxtotnw + dmxnw;
	} else {
	  if (dmxnw > dmxtotnw)
	    dmxnw = dmxtotnw;
	  for (n=dmxnw; n > 0; n--) {
	    put16r0(*iobufp++, dmxaddr+i);
	  }
	  dmxtotnw = dmxtotnw - dmxnw;
	}
	TRACE(T_TIO, " read/wrote %d words\n", dmxnw);
	if (dmxchan & 0x0800) {         /* DMC */
	  put16r0(dmxaddr+dmxnw, dmxreg);          /* update starting address */
	} else {
	  regs.sym.regdmx[dmxreg] += dmxnw<<4;     /* increment # words */
	  regs.sym.regdmx[dmxreg+1] += dmxnw;      /* increment address */
	}

	/* if chaining, bump channel number and decrement # channels */

	if (dmxchan & 0xF000)
	  dmxchan = dmxchan + 2 - (1<<12);
	else
	  break;
      }

      /* for write record, do the write */

      if (crs[A] & 0x10) {             /* write record */
	TRACE(T_TIO, " write record\n");
	n = dmxtotnw*2;
	reclen[0] = n & 0xFF;
	reclen[1] = n>>8 & 0xFF;
	reclen[2] = n>>16 & 0xFF;
	reclen[3] = n>>24 & 0xFF;
	*(int *)iobuf = *(int *)reclen;
	*(int *)(iobuf+2+dmxtotnw) = *(int *)reclen;
	mtwrite(unit[u].fd, iobuf, dmxtotnw+4, &unit[u].mtstat);
      } else {                         /* read record */
	if (dmxtotnw > 0) {
	  TRACE(T_TIO,  " DMA Overrun, lost %d words\n", dmxtotnw);
	  unit[u].mtstat |= 0x800;
	}
      }
      IOSKIP;
      break;
	
    } else if (func == 02) {
      TRACE(T_TIO,  "  setup INA, A='%06o, 0x%04x\n", crs[A], crs[A]);
      if (crs[A] & 0x8000)
	datareg = unit[usel].mtstat;
      else if (crs[A] & 0x4000)
	datareg = 0114;           /* device ID */
      else if (crs[A] & 0x2000)
	datareg = dmxchan;
      else if (crs[A] & 0x1000)
	datareg = mtvec;
      else {
	TRACE(T_TIO, "  Bad OTA '02 to tape drive, A='%06o, 0x$04x\n", crs[A], crs[A]);
	if (enabled) {
	  interrupting = 1;
	  devpoll[device] = 10;
	}
      }
      TRACE(T_TIO,  "  datareg='%06o, 0x%04x\n", datareg, datareg);
      IOSKIP;

    } else if (func == 03) {                /* power on */
      TRACE(T_TIO,  " power on\n");
      IOSKIP;

    } else if (func == 05) {                /* illegal - DIAG */
      TRACE(T_TIO,  " illegal DIAG OTA '05\n");
      if (enabled) {
	interrupting = 1;
	devpoll[device] = 10;
	IOSKIP;
      }

    } else if (func == 014) {               /* set DMX channel */
      dmxchan = crs[A];
      TRACE(T_TIO,  " dmx channel '%o, 0x%04x\n", dmxchan, dmxchan);
      IOSKIP;

    } else if (func == 016) {               /* set interrupt vector */
      mtvec = crs[A];
      TRACE(T_TIO,  " interrupt vector '%o\n", mtvec);
      IOSKIP;

    } else {
      printf("Unimplemented OTA device '%02o function '%02o, A='%o\n", device, func, crs[A]);
      fatal(NULL);
    }
    break;

  case 4:
    TRACE(T_TIO,  " POLL device '%02o, enabled=%d, interrupting=%d\n", device, enabled, interrupting);
    if (enabled && interrupting) {
      if (intvec == -1) {
	TRACE(T_TIO,  " CPU interrupt to vector '%o\n", mtvec);
	intvec = mtvec;
      }
      /* HACK: keep interrupting because of Primos/controller race bug */
      devpoll[device] = 100;
    }
  }
}

/* Device '20: control panel switches and lights, and realtime clock

   OCP '0020 = start Line Frequency Clock, enable mem increment, ack previous overflow (something else on VCP?)
   OCP '0120 = ack PIC interrupt
   OCP '0220 = stop LFC, disable mem increment, ack previous overflow
   OCP '0420 = select LFC for memory increment (something else on VCP?)
   OCP '0520 = select external clock for memory increment
   OCP '0620 = starts a new 50-ms watchdog timeout interval
   OCP '0720 = stops the watchdog timer
   OCP '1520 = set interrupt mask
   OCP '1620 = reset interrupt mask
   OCP '1720 = initialize as in Master Clear

   SKS '0020 = skip if clock IS interrupting
   SKS '0220 = skip if clock IS NOT interrupting
   SKS '0520 = skip if WDT timed out
   SKS '0720 = skip if WDT caused external interrupt (loc '60)

   OTA '0220 = set PIC Interval register (negative, interrupts on incr to zero)
   OTA '0720 = set control register
   OTA '1220 = set Memory Increment Cell address
   OTA '1320 = set interrupt vector address
   OTA '1720 = write to lights (sets CP fetch address)

   INA '0220 = read PIC Interval register
   INA '1120 = read device ID, don't clear A first
   INA '1220 = read Memory Increment Cell address
   INA '1320 = read interrupt vector address
   INA '1420 = read location from CP ROM (not implemented, used to boot)
   INA '1620 = read control panel up switches
   INA '1720 = read control panel down switches

   IMPORTANT NOTE 1: this device ('20) never skips!

   IMPORTANT NOTE 2: when the emulator wakes up after the host system
   suspends, devcp will either reset the clock (if it knows where
   Primos' DATNOW variable is), or will make the clock tick faster
   until it catches up to where it should be.  Fake clock interrupts
   are injected every 100 Prime instructions to catch up.  These fast
   interrupts can't occur too quickly (like every 10 Prime
   instructions), because CLKDIM can't execute completely in 10
   instructions and the repeated interrupts will eventually cause the
   clock semaphore to overflow.
*/

/* initclock sets Primos' real-time clock variable */

initclock(ea_t datnowea) {
  int datnow, i;
  time_t unixtime;
  struct tm *tms;

  unixtime = time(NULL);
  tms = localtime(&unixtime);
  datnow = tms->tm_year<<25 | (tms->tm_mon+1)<<21 | tms->tm_mday<<16 | ((tms->tm_hour*3600 + tms->tm_min*60 + tms->tm_sec)/4);
  put32r0(datnow, datnowea);
}


int devcp (int class, int func, int device) {
  static short enabled = 0;
  static unsigned short clkvec = 0;
  static short clkpic = -947;
  static float clkrate = 3.2;
  static unsigned long ticks = -1;
  static unsigned long absticks = -1;
  static struct timeval start_tv;
  static ea_t datnowea = 0;
  static struct timeval prev_tv;
  static unsigned long previnstcount=0; /* value of instcount corresponding to above */

  struct timeval tv;
  unsigned long elapsedms,targetticks;
  int i;

#define SETCLKPOLL devpoll[device] = instpermsec*(-clkpic*clkrate)/1000;

  switch (class) {

  case -1:

    /* if -map is used, lookup DATNOW symbol and set the 32-bit 
       Primos time value (DATNOW+TIMNOW) */

    datnowea = 0;
    for (i=0; i<numsyms; i++) {
      if (strcmp(mapsym[i].symname, "DATNOW") == 0)
	datnowea = mapsym[i].address;
    }
    return 0;

  case 0:
    TRACE(T_INST, " OCP '%02o%02o\n", func, device);

    if (func == 00) {           /* does something on VCP after OCP '0420 */
      ;

    } else if (func == 01) {    /* ack PIC interrupt */
      ; 

    } else if (func == 02) {    /* stop LFC (IO.PNC DIAG) */
      ; 

    } else if (func == 04) {    /* does something on VCP during boot */
      ;

    } else if (func == 015) {   /* set interrupt mask */
      /* this enables tracing when the clock process initializes */
      //traceflags = ~TB_MAP;
      //TRACEA("Clock interrupt enabled!\n");
      enabled = 1;
      SETCLKPOLL;
      ticks = -1;

    } else if (func == 016 || func == 017) {
      enabled = 0;
      devpoll[device] = 0;

    } else {
      printf("Unimplemented OCP device '%02o function '%02o\n", device, func);
      fatal(NULL);
    }
    break;

  case 1:
    TRACE(T_INST, " SKS '%02o%02o\n", func, device);
    printf("Unimplemented SKS device '%02o function '%02o\n", device, func);
    fatal(NULL);
    break;

  case 2:
    TRACE(T_INST, " INA '%02o%02o\n", func, device);
    if (func == 011) {             /* input ID */
      crs[A] = 020;                /* this is the Option-A board */
      crs[A] = 0120;               /* this is the SOC board */
      //traceflags = ~TB_MAP;
    } else if (func == 016) {
      crs[A] = sswitch;
    } else if (func == 017) {      /* read switches pushed down */
      crs[A] = 0;
    } else {
      printf("Unimplemented INA device '%02o function '%02o\n", device, func);
      fatal(NULL);
    }
    break;

  case 3:
    TRACE(T_INST, " OTA '%02o%02o\n", func, device);

    /* standard Primos always sets a PIC interval of 947 and uses the
       hardware clock rate of 3.2 usec.  This causes a software interrupt
       every 947*3.2 usec, or 3030.4 milliseconds, and this works out
       to 330 software interrupts per second.  BUT newer machines tick
       at whatever rate they want and ignore the PIC interval set here.

       To keep emulator overhead low, some versions of Primos have been
       modified to set the PIC interval to 15625 and send 20 ticks per
       second to Primos.

       To handle both cases, this code looks at the cpuid to determine
       the actual # of ticks per second expected by Primos whenever
       the PIC interval is set to 947.  If PIC is set to something
       other than 947, that value is used.
    */

    if (func == 02) {            /* set PIC interval */
      clkpic = *(short *)(crs+A);
      if (clkpic == -947)
	if (cpuid == 11 || cpuid == 20)
	  clkpic = -625;         /* P2250: 500 ticks/second */
	else if (cpuid >= 15)    /* newer machines: 250 ticks/second */
	  clkpic = -1250;
      TRACE(T_INST, "Clock PIC %d requested, set to %d\n", *(short *)(crs+A), clkpic);
      SETCLKPOLL;

    } else if (func == 07) {
      TRACE(T_INST, "Clock control register set to '%o\n", crs[A]);
      if (crs[A] & 020)
	clkrate = 102.4;
      else
	clkrate = 3.2;
      SETCLKPOLL;

    } else if (func == 013) {     /* set interrupt vector */
      clkvec = crs[A];
      TRACE(T_INST, "Clock interrupt vector address = '%o\n", clkvec);

    } else if (func == 017) {     /* write lights */

    } else {
      printf("Unimplemented OTA device '%02o function '%02o, A='%o\n", device, func, crs[A]);
      fatal(NULL);
    }
    break;

    /* Clock poll important considerations are:
       1. ticks = -1 initially; this triggers initialization here
       2. start_tv corresponds to the time where ticks = 0
       3. if the clock gets out of sync, it ticks faster or slower until
          it is right
       4. if the clock is WAY out of sync, try to jump to the correct time
       5. once the clock is in sync, reset start_tv if ticks gets too big
          (uh, after 5 months!) to prevent overflows in time calculations
    */

  case 4:
    /* interrupt if enabled and no interrupt active */

    if (enabled) {
      if (intvec == -1) {
	intvec = clkvec;
	SETCLKPOLL;
	ticks++;
	if (gettimeofday(&tv, NULL) != 0)
	  fatal("em: gettimeofday 3 failed");
	if (ticks == 0) {
	  start_tv = tv;
	  prev_tv = tv;
	  previnstcount = instcount;
	  if (datnowea != 0)
	    initclock(datnowea);
	} 
	elapsedms = (tv.tv_sec-start_tv.tv_sec-1)*1000 + (tv.tv_usec+1000000-start_tv.tv_usec)/1000;
	targetticks = elapsedms/(-clkpic*clkrate/1000);
#if 0
	absticks++;
	if (absticks%1000 == 0 || abs(ticks-targetticks) > 5)
	  printf("\nClock check: target=%d, ticks=%d\n", targetticks, ticks);
#endif

	/* if the clock gets way out of whack (eg, because of a host
	   suspend), reset it IFF datnowea is set.  This causes an
	   immediate jump to the correct time.  If datnowea is not
	   available (no Primos maps), then we have to tick our way to
	   the correct time.  In addition to lowering overhead, slower
	   clock tick rates make catching up much faster. */

	if (abs(ticks-targetticks) > 5000 && datnowea != 0)
	  ticks = -1;
	else if (ticks < targetticks)
	  devpoll[device] = 100;                /* behind, so catch-up */
	else if (ticks > targetticks)
	  devpoll[device] = devpoll[device]*2;  /* ahead, so slow down */
	else {                                  /* just right! */
	  if (ticks > 1000000000) {             /* after a long time, */
	    start_tv = tv;                      /* reset tick vars */
	    ticks = 0;
	  }
	}

	/* update instpermsec every 5 seconds
	   NB: this should probably be done whether the clock is running
	   or not */

	if (instcount-previnstcount > instpermsec*1000*5) {
	  instpermsec = (instcount-previnstcount) /
	    ((tv.tv_sec-prev_tv.tv_sec-1)*1000 + (tv.tv_usec+1000000-prev_tv.tv_usec)/1000);
	  //printf("instcount = %d, previnstcount = %d, diff=%d, instpermsec=%d\n", instcount, previnstcount, instcount-previnstcount, instpermsec);
	  //printf("\ninstpermsec=%d\n", instpermsec);
	  previnstcount = instcount;
	  prev_tv = tv;
	}
      } else {
	devpoll[device] = 100;         /* couldn't interrupt, try again soon */
      }
    }
    break;
  }
}


/* disk controller at '26 and '27

  NOTES:
  - in the DSEL disk channel program command, unit number is a 4-bit field,
  with these binary values:
  0001 = unit 0 (pdev 460/461)
  0010 = unit 1 (pdev 462/463)
  0100 = unit 2 (pdev 464/465)
  1000 = unit 3 (pdev 466/467)

  OCP '1626 = reset interrupt
  OCP '1726 = reset controller

  INA '0626 = ??

  INA '1126 = input ID, don't clear A first, fails if no controller
  - bits 1,2,8-16 are significant, bits 8-9 are type, 10-16 are ID
  - 4004 controller responds '26 (type=0)
  - 4005 controller responds '126 (type=1)
  - 2382 controller responds '040100 (type=1)
  - LCDTC controller responds '226 (type=2)
  - 10019 controller responds '040100 (type=1)
  - 6590 controller responds '040100 (type=1)

  INA '1726 = read oar, fails if controller is not halted

  OTA '1726 = load OAR (Order Address Register), ie, run channel
  program, address is in A

 */

int devdisk (int class, int func, int device) {

#define S_HALT 0
#define S_RUN 1
#define S_INT 2

/* this should be 8, but not sure how it is supported on controllers */

#define MAXDRIVES 4
#define HASHMAX 4451

#if 0
  #define CID4005 0100
#else
  #define CID4005 0
#endif


  static struct {
    unsigned short oar;
    unsigned short state;                  /* channel program state: S_XXXX */
    unsigned short status;                 /* controller status */
    short usel;                            /* unit selected (0-3, -1=none) */
    short dmachan;                         /* dma channel selected */
    short dmanch;                          /* number of dma channels-1 */
    struct {
      int rtfd;                            /* read trace file descriptor */
      unsigned short theads;               /* total heads (cfg file) */
      unsigned short spt;                  /* sectors per track */
      unsigned short curtrack;             /* current head position */
      int devfd;                           /* Unix device file descriptor */
      int readnum;                         /* increments on each read */
      unsigned char** modrecs;             /* hash table of modified records */
    } unit[MAXDRIVES];
  } dc[64];

  short i,u;

  /* temps for running channel programs */

  unsigned short order;
  unsigned short m,m1,m2;
  short head, track, rec, recsize, nwords;
  unsigned short dmareg, dmaaddr;
  unsigned char *hashp;


  /* NOTE: this iobuf size looks suspicious; probably should be 2080 bytes,
     the largest disk record size, and there probably should be some checks
     that no individual DMA exceeds this size, that no individual disk
     read or write exceeds 1040 words.  Maybe it's 4096 bytes because this
     is the largest DMA transfer request size... */

  unsigned short iobuf[4096];             /* local I/O buf (for mapped I/O) */
  unsigned short *iobufp;
  unsigned short access;
  short dmanw, dmanw1, dmanw2;
  unsigned int utempl;
  char ordertext[8];
  int theads, spt, phyra, phyra2;
  int nb;                   /* number of bytes returned from read/write */
  char devfile[8];
  char rtfile[16];          /* read trace file name */
  int rtnw;                 /* total number of words read (all channels) */

  //traceflags |= TB_DIO;

  switch (class) {

  case -1:
#ifdef DISKSAFE
    printf("em: Running in DISKSAFE mode; no changes will be permanent\n");
#endif
    for (i=0; i<64; i++) {
      dc[i].state = S_HALT;
      dc[i].status = 0100000;
      dc[i].usel = -1;
      for (u=0; u<MAXDRIVES; u++) {
	dc[i].unit[u].rtfd = -1;
	dc[i].unit[u].theads = 40;
	dc[i].unit[u].spt = 9;
	dc[i].unit[u].curtrack = 0;
	dc[i].unit[u].devfd = -1;
	dc[i].unit[u].readnum = -1;
	dc[i].unit[u].modrecs = NULL;
      }
    }
    return 0;
      
  case 0:
    TRACE(T_INST|T_DIO, " OCP '%2o%2o\n", func, device);
    if (func == 016) {                /* reset interrupt */
      if (dc[device].state == S_INT) {
	dc[device].state = S_RUN;
	devpoll[device] = 1;
      }
    } else if (func == 017) {         /* reset controller */
      dc[device].state = S_HALT;
      dc[device].status = 0100000;
      dc[device].usel = -1;
    } else {
      printf(" Unrecognized OCP '%2o%2o\n", func, device);
      fatal(NULL);
    }
    break;

  case 1:
    TRACE(T_INST|T_DIO, " SKS '%2o%2o\n", func, device);
    if (func == 04) {                  /* skip if not interrupting */
      if (dc[device].state != S_INT)
	IOSKIP;
    } else {
      printf(" Unrecognized SKS '%2o%2o\n", func, device);
      fatal(NULL);
    }
    break;

  case 2:
    TRACE(T_INST|T_DIO, " INA '%2o%2o\n", func, device);

    /* this turns tracing on when the Primos disk processes initialize */
    //traceflags = ~TB_MAP;

    /* INA's are only accepted when the controller is not busy */

    if (dc[device].state != S_HALT)
      return;

    if (func == 01)          /* read device id, clear A first */
      crs[A] = CID4005 + device;
    else if (func == 011)    /* read device id, don't clear A */
      crs[A] |= (CID4005 + device);
    else if (func == 017) {  /* read OAR */
      crs[A] = dc[device].oar;
    } else {
      printf("Unimplemented INA device '%02o function '%02o\n", device, func);
      fatal(NULL);
    }
    IOSKIP;
    break;

  case 3:
    TRACE(T_INST|T_DIO, " OTA '%02o%02o\n", func, device);
    if (func == 017) {        /* set OAR (order address register) */
      dc[device].state = S_RUN;
      dc[device].oar = crs[A];
      devpoll[device] = 1;
    } else {
      printf("Unimplemented OTA device '%02o function '%02o, A='%o\n", device, func, crs[A]);
      fatal(NULL);
    }
    IOSKIP;
    break;

  case 4:   /* poll (run channel program) */

    while (dc[device].state == S_RUN) {
      m = get16r0(dc[device].oar);
      m1 = get16r0(dc[device].oar+1);
      TRACE(T_INST|T_DIO, "\nDIOC %o: %o %o %o\n", dc[device].oar, m, m1, get16r0(dc[device].oar+2));
      dc[device].oar += 2;
      order = m>>12;

      /* this is for conditional execution, and needs some work... */

      if (m & 04000) {   /* "execute if ..." */
	if (order == 2 || order == 5 || order == 6)
	  dc[device].oar++;
	continue;
      }

      switch (order) {

      case 0: /* DHLT = Halt */
	dc[device].state = S_HALT;
	devpoll[device] = 0;
	TRACE(T_INST|T_DIO, " channel halted at '%o\n", dc[device].oar);
	break;

      case 2: /* SFORM = Format */
      case 5: /* SREAD = Read */
      case 6: /* SWRITE = Write */
	dc[device].status &= ~076000;             /* clear bits 2-6 */
	m2 = get16r0(dc[device].oar++);
	recsize = m & 017;
	track = m1 & 01777;
	rec = m2 >> 8;   /* # records for format, rec # for R/W */
	head = m2 & 077;
	u = dc[device].usel;
	if (order == 2)
	  strcpy(ordertext,"Format");
	else if (order == 5)
	  strcpy(ordertext,"Read");
	else if (order == 6)
	  strcpy(ordertext,"Write");
	TRACE(T_INST|T_DIO, "%s, head=%d, track=%d, rec=%d, recsize=%d\n", ordertext, head, track, rec, recsize);
	if (u == -1) {
	  fprintf(stderr," Device '%o, order %d with no unit selected\n", device, order);
	  dc[device].status |= 2;      /* select error (right?)... */
	  break;
	}
	if (recsize != 0) {
	  fprintf(stderr," Device '%o, order %d, recsize=%d\n", device, order, recsize);
	  dc[device].status |= 02000;  /* header check (right error?) */
	  break;
	}
	if (track != dc[device].unit[u].curtrack) {
	  fprintf(stderr," Device '%o, order %d at track %d, but already positioned to track %d\n", device, order, track, dc[device].unit[u].curtrack);
	  dc[device].status |= 4;      /* illegal seek */
	  break;
	}
	if (dc[device].unit[u].devfd == -1) {
	  TRACE(T_INST|T_DIO, " Device '%o unit %d not ready\n", device, u);
	  dc[device].status = 0100001;
	} else if (order == 2) {
	  TRACE(T_INST|T_DIO, " Format order\n");
	  //fatal("DFORMAT channel order not implemented");
	} else {            /* order = 5 (read) or 6 (write) */

	  /* translate head/track/sector to drive record address */

	  phyra = (track*dc[device].unit[u].theads*dc[device].unit[u].spt) + head*9 + rec;
	  TRACE(T_INST|T_DIO,  " Unix ra=%d, byte offset=%d\n", phyra, phyra*2080);

	  /* does this record exist in the disk unit hash table?  If it does,
	     we'll do I/O to the hash entry.  If it doesn't, then for read,
	     go to the disk file; for write, make a new hash entry */

	  hashp = NULL;

#ifdef DISKSAFE
	  //fprintf(stderr," R/W, modrecs=%p\n", dc[device].unit[u].modrecs);
	  for (hashp = dc[device].unit[u].modrecs[phyra%HASHMAX]; hashp != NULL; hashp = *((unsigned char **)hashp)) {
	    //fprintf(stderr," lookup, hashp=%p\n", hashp);
	    if (phyra == *((int *)(hashp+sizeof(void *))))
	      break;
	  }
	  //fprintf(stderr,"After search, hashp=%p\n", hashp);

	  if (hashp == NULL)
	    if (order == 5) {        /* read */
#endif
	      if (lseek(dc[device].unit[u].devfd, phyra*2080, SEEK_SET) == -1) {
		perror("Unable to seek drive file");
		fatal(NULL);
	      }
#ifdef DISKSAFE
	    } else {                 /* write */
	      hashp = malloc(1040*2 + sizeof(void*) + sizeof(int));
	      *(unsigned char **)hashp = dc[device].unit[u].modrecs[phyra%HASHMAX];
	      *((int *)(hashp+sizeof(void *))) = phyra;
	      //fprintf(stderr," Write, new hashp = %p, old bucket head = %p\n", hashp, *(unsigned char **)hashp);
	      dc[device].unit[u].modrecs[phyra%HASHMAX] = hashp;
	      hashp = hashp + sizeof(void*) + sizeof(int);
	    }
	  else
	    hashp = hashp + sizeof(void*) + sizeof(int);
	  //fprintf(stderr," Before disk op %d, hashp=%p\n", order, hashp);
#endif

#if 0
	  if (order == 6) {
	    dmareg = dc[device].dmachan << 1;
	    dmanw = regs.sym.regdmx[dmareg];
	    dmanw = -(dmanw>>4);
	    dmaaddr = regs.sym.regdmx[dmareg+1];
	    phyra2 = get16r0(dmaaddr+0);
	    phyra2 = phyra2<<16 | get16r0(dmaaddr+1);
	    if (phyra2 != phyra)
	      fprintf(stderr,"devdisk: phyra=%d, phyra2=%d; CRA mismatch (dmanw = %d)!\n", phyra, phyra2, dmanw);
	  }
#endif
	    
	  while (dc[device].dmanch >= 0) {
	    dmareg = dc[device].dmachan << 1;
	    dmanw = regs.sym.regdmx[dmareg];
	    dmanw = -(dmanw>>4);
	    dmaaddr = regs.sym.regdmx[dmareg+1];
	    TRACE(T_INST|T_DIO,  " DMA channels: nch-1=%d, ['%o]='%o, ['%o]='%o, nwords=%d\n", dc[device].dmanch, dc[device].dmachan, regs.sym.regdmx[dmareg], dc[device].dmachan+1, dmaaddr, dmanw);
	    
	    if (order == 5) {
	      if (crs[MODALS] & 020)
		if ((dmaaddr & 01777) || dmanw > 1024)
		  iobufp = iobuf;
		else
		  iobufp = mem+mapva(dmaaddr, WACC, &access, 0);
	      else
		iobufp = mem+dmaaddr;
	      if (hashp != NULL) {
		memcpy((char *)iobufp, hashp, dmanw*2);
		hashp += dmanw*2;
	      } else if ((nb=read(dc[device].unit[u].devfd, (char *)iobufp, dmanw*2)) != dmanw*2) {
		fprintf(stderr, "Disk read error: device='%o, u=%d, fd=%d, nb=%d\n", device, u, dc[device].unit[u].devfd, nb);
		if (nb == -1) perror("Unable to read drive file");
		memset((char *)iobufp, 0, dmanw*2);
	      }
	      if (iobufp == iobuf)
		for (i=0; i<dmanw; i++)
		  put16r0(iobuf[i], dmaaddr+i);
	    } else {
	      if (crs[MODALS] & 020) {
		iobufp = iobuf;
		for (i=0; i<dmanw; i++)
		  iobuf[i] = get16r0(dmaaddr+i);
	      } else
		iobufp = mem+dmaaddr;
	      if (hashp != NULL) {
		memcpy(hashp, (char *)iobufp, dmanw*2);
		hashp += dmanw*2;
	      } else if (write(dc[device].unit[u].devfd, (char *)iobufp, dmanw*2) != dmanw*2) {
		perror("Unable to write drive file");
		fatal(NULL);
	      }
	    }
	    regs.sym.regdmx[dmareg] = 0;
	    regs.sym.regdmx[dmareg+1] += dmanw;
	    dc[device].dmachan += 2;
	    dc[device].dmanch--;
	  }
	}
	break;

      case 3: /* SSEEK = Seek */
	u = dc[device].usel;
	if (u == -1) {
	  fprintf(stderr," Device '%o, order %d with no unit selected\n", device, order);
	  dc[device].status |= 2;      /* select error (right?)... */
	  break;
	}
	if (m1 & 0100000) {
	  track = 0;
	  dc[device].status &= ~4;   /* clear bit 14: seek error */
	} else {
	  track = m1 & 01777;
	}
	TRACE(T_INST|T_DIO, " seek track %d, restore=%d, clear=%d\n", track, (m1 & 0100000) != 0, (m1 & 040000) != 0);
	dc[device].unit[u].curtrack = track;
	break;

      case 4: /* DSEL = Select unit */
	u = (m1 & 017);            /* get unit bits */
	if (u == 0) {
	  dc[device].usel = -1;    /* de-select */
	  TRACE(T_INST|T_DIO, " de-select\n");
	  break;
	}
	dc[device].status &= ~3;  /* clear 15-16: select err + unavailable */
	if (u != 1 && u != 2 && u != 4 && u != 8) {
	  fprintf(stderr," Device '%o, bad select '%o\n", device, u);
	  dc[device].usel = -1;    /* de-select */
	  dc[device].status != 2;  /* set bit 15: select error */
	  break;
	}
	u = u >> 1;                /* unit => 0/1/2/4 */
	if (u == 4) u = 3;         /* unit => 0/1/2/3 */
	TRACE(T_INST|T_DIO, " select unit %d\n", u);
	dc[device].usel = u;
	if (dc[device].unit[u].devfd == -1) {
	  snprintf(devfile,sizeof(devfile),"dev%ou%d", device, u);
	  TRACE(T_INST|T_DIO, " filename for dev '%o unit %d is %s\n", device, u, devfile);
	  /* NOTE: add O_CREAT to allow creating new drives on the fly */
	  if ((dc[device].unit[u].devfd = open(devfile, O_RDWR, 0770)) == -1) {
	    fprintf(stderr,"em: unable to open disk device file %s for device '%o unit %d; flagging \n", devfile, device, u);
	    dc[device].status = 0100001;    /* not ready */
	  } else {
#ifdef DISKSAFE
	    if (flock(dc[device].unit[u].devfd, LOCK_SH+LOCK_NB) == -1)
	      fatal("Disk drive file is in use");
	    dc[device].unit[u].modrecs = calloc(HASHMAX, sizeof(void *));
	    //fprintf(stderr," Device '%o, unit %d, modrecs=%p\n", device, u, dc[device].unit[u].modrecs);
#else
	    if (flock(dc[device].unit[u].devfd, LOCK_EX+LOCK_NB) == -1)
	      fatal("Disk drive file is in use");
#endif
	  }
	}
	break;

      case 7: /* DSTALL = Stall */
	TRACE(T_INST|T_DIO, " stall\n");

	/* NOTE: technically, the stall command is supposed to wait
	   210 usecs, so that the disk controller doesn't hog the I/O
	   bus by looping in a channel program waiting for I/O to
	   complete.  With the emulator, this delay isn't necessary,
	   although it will cause DIAG tests to fail if the delay is
	   omitted.  Hence the PX test.  Ignoring stall gives a 25%
	   increase in I/O's per second on a 2GHz Mac (8MB emulator). */

	if (crs[MODALS] & 010)             /* PX enabled? */
	  break;                           /* yes, no stall */
	devpoll[device] = instpermsec/5;   /* 200 microseconds, sb 210 */
	return;

      case 9: /* DSTAT = Store status to memory */
	TRACE(T_INST|T_DIO,  " store status='%o to '%o\n", dc[device].status, m1);
	put16r0(dc[device].status,m1);
	break;

      case 11: /* DOAR = Store OAR to memory (2 words) */
	TRACE(T_INST|T_DIO,  " store OAR='%o to '%o\n", dc[device].oar, m1);
	put16r0(dc[device].oar,m1);
	break;

      case 13: /* SDMA = select DMA channel(s) to use */
	dc[device].dmanch = m & 017;
	dc[device].dmachan = m1;
	TRACE(T_INST|T_DIO,  " set DMA channels, nch-1=%d, channel='%o\n", dc[device].dmanch, dc[device].dmachan);
	break;

      case 14: /* DINT = generate interrupt through vector address */
	TRACE(T_INST|T_DIO,  " interrupt through '%o\n", m1);
	if (intvec >= 0 || !(crs[MODALS] & 0100000) || inhcount > 0)
	  dc[device].oar -= 2;     /* can't take interrupt right now */
	else {
	  intvec = m1;
	  dc[device].state = S_INT;
	}
	//traceflags = ~TB_MAP;
	devpoll[device] = 10;
	return;

      case 15: /* DTRAN = channel program jump */
	dc[device].oar = m1;
	TRACE(T_INST|T_DIO,  " jump to '%o\n", m1);
	break;

      default:
	/* NOTE: orders 1 & 8 are supposed to halt the channel program
	   but leave the controller busy (model 4004).
           Orders 10 & 12 (Store/Load) are not implemented */
	printf("Unrecognized channel order = %d\n", order);
	fatal(NULL);
      }
    }
  }
}

/* 
  AMLC I/O operations:

  OCP '0054 - stop clock
  OCP '0154 - single step clock

  OCP '1234 - set normal/DMT mode
  OCP '1354 - set diagnostic/DMQ mode
  OCP '1554 - enable interrupts
  OCP '1654 - disable interrupts
  OCP '1754 - initialize AMLC

  SKS '0454 - skip if NOT interrupting

  INA '0054 - input data set sense bit for all lines (read clear to send)
  INA '0754 - input AMLC status and clear (always skips)
  INA '1154 - input AMLC controller ID
  INA '1454 - input DMA/C channel number
  INA '1554 - input DMT/DMQ base address
  INA '1654 - input interrupt vector address

  OTA '0054 - output line number for INA '0054 (older models only)
  OTA '0154 - output line configuration for 1 line
  OTA '0254 - output line control for 1 line
  OTA '1454 - output DMA/C channel number
  OTA '1554 - output DMT/DMQ base address
  OTA '1654 - output interrupt vector address
  OTA '1754 - output programmable clock constant

  Primos AMLC usage:

  OCP '17xx
  - initialize controller
  - clear all registers and flip-flops
  - start line clock
  - clear all line control bits during the 1st line scan
  - responds not ready until 1 line scan is complete

  INA '11xx
  - read AMLC ID
  - emulator always returns '20054 (DMQ, 16 lines)

  OTA '17xx
  - set AMLC programmable clock constant
  - ignored by the emulator

  OTA '14xx
  - set DMC channel address for double input buffers
  - emulator stores in a structure

  OCP '13xx
  - set dmq mode (used to be "set diagnostic mode")
  - ignored by the emulator

  OTA '15xx
  - set DMT Base Address
  - also used for DMQ address?
  - emulator stores in a structure

  OTA '16xx
  - set interrupt vector address
  - emulator stores in a structure

  OTA '01xx
  - set line configuration
  - emulator ignores: all lines are 8-bit raw

  OTA '02xx
  - set line control
  - emulator ignores: all lines are enabled

  OCP '15xx/'16xx
  - enable/disable interrupts
  - emulator stores in a structure
*/

int devamlc (int class, int func, int device) {

#define MAXLINES 128
#define MAXFD MAXLINES+20
#define MAXBOARDS 8
  /* AMLC poll rate in milliseconds */
#define AMLCPOLL 75

#if 1
  #define QAMLC 020000   /* this is to enable QAMLC/DMQ functionality */
#else
  #define QAMLC 0
#endif

  /* terminal states needed to process telnet connections */

#define TS_DATA 0      /* data state, looking for IAC */
#define TS_IAC 1       /* have seen initial IAC */
#define TS_SUBOPT 2    /* inside a suboption */
#define TS_OPTION 3    /* inside an option */

  static short inited = 0;
  static short devices[MAXBOARDS];         /* list of AMLC devices initialized */
  static int tsfd;                         /* socket fd for terminal server */
  static struct {                          /* maps socket fd to device & line */
    int device;
    int line;
  } socktoline[MAXLINES];
  static struct {
    char dmqmode;                          /* 0=DMT, 1=DMQ */
    char bufnum;                           /* 0=1st input buffer, 1=2nd */
    char eor;                              /* 1=End of Range on input */
    unsigned short dmcchan;                /* DMC channel (for input) */
    unsigned short baseaddr;               /* DMT/Q base address (for output) */
    unsigned short intvector;              /* interrupt vector */
    unsigned short intenable;              /* interrupts enabled? */
    unsigned short interrupting;           /* am I interrupting? */
    unsigned short xmitenabled;            /* 1 bit per line */
    unsigned short recvenabled;            /* 1 bit per line */
    unsigned short ctinterrupt;            /* 1 bit per line */
    unsigned short dss;                    /* 1 bit per line */
    unsigned short sockfd[16];             /* Unix socket fd, 1 per line */
    unsigned short tstate[16];             /* terminal state (telnet) */
  } dc[64];

  int lx;
  unsigned short utempa;
  unsigned int utempl;
  ea_t qcbea, dmcea, dmcbufbegea, dmcbufendea;
  unsigned short dmcnw;
  int dmcpair;
  int optval;
  int tsflags;
  struct sockaddr_in addr;
  int fd;
  unsigned int addrlen;
  unsigned char buf[1024];      /* max size of DMQ buffer */
  int i, n, nw, newdevice;
  fd_set fds;
  struct timeval timeout;
  unsigned char ch;
  int state;

  switch (class) {

  case -1:

    /* this part of initialization only occurs once, no matter how many
       AMLC boards are configured */

    if (!inited) {
      for (i=0; i<MAXBOARDS; i++)
	devices[i] = 0;
      if (tport != 0) {
	tsfd = socket(AF_INET, SOCK_STREAM, 0);
	if (tsfd == -1) {
	  perror("socket failed for AMLC");
	  fatal(NULL);
	}
	optval = 1;
	if (setsockopt(tsfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval))) {
	  perror("setsockopt failed for AMLC");
	  fatal(NULL);
	}
	addr.sin_family = AF_INET;
	addr.sin_port = htons(tport);
	addr.sin_addr.s_addr = INADDR_ANY;
	if(bind(tsfd, (struct sockaddr *)&addr, sizeof(addr))) {
	  perror("bind: unable to bind for AMLC");
	  fatal(NULL);
	}
	if(listen(tsfd, 10)) {
	  perror("listen failed for AMLC");
	  fatal(NULL);
	}
	if ((tsflags = fcntl(tsfd, F_GETFL)) == -1) {
	  perror("unable to get ts flags for AMLC");
	  fatal(NULL);
	}
	tsflags |= O_NONBLOCK;
	if (fcntl(tsfd, F_SETFL, tsflags) == -1) {
	  perror("unable to set ts flags for AMLC");
	  fatal(NULL);
	}
      } else
	fprintf(stderr, "-tport is zero, can't start AMLC devices\n");
      inited = 1;
    }

    /* this part of initialization occurs for every AMLC board */

    if (!inited || tport == 0)
      return -1;

    /* add this device to the devices array, in the proper slot
       so we can tell what order the boards should be in */

    switch (device) {
    case 054: devices[0] = device; break;
    case 053: devices[1] = device; break;
    case 052: devices[2] = device; break;
    case 035: devices[3] = device; break;
    case 015: devices[4] = device; break;
    case 016: devices[5] = device; break;
    case 017: devices[6] = device; break;
    case 032: devices[7] = device; break;
    }
    return 0;

  case 0:
    TRACE(T_INST, " OCP '%02o%02o\n", func, device);
    //printf(" OCP '%02o%02o\n", func, device);

    if (func == 012) {            /* set normal (DMT) mode */
      dc[device].dmqmode = 0;

    } else if (func == 013 && QAMLC) {     /* set diagnostic (DMQ) mode */
      dc[device].dmqmode = 1;

    } else if (func == 015) {     /* enable interrupts */
      dc[device].intenable = 1;

    } else if (func == 016) {     /* disable interrupts */
      dc[device].intenable = 0;

    } else if (func == 017) {     /* initialize AMLC */
      dc[device].dmqmode = 0;
      dc[device].bufnum = 0;
      dc[device].dmcchan = 0;
      dc[device].baseaddr = 0;
      dc[device].intvector = 0;
      dc[device].intenable = 0;
      dc[device].interrupting = 0;
      dc[device].xmitenabled = 0;
      dc[device].recvenabled = 0;
      dc[device].ctinterrupt = 0;
      dc[device].dss = 0;         /* NOTE: this is stored inverted: 1=connected */
      dc[device].eor = 0;

    } else {
      printf("Unimplemented OCP device '%02o function '%02o\n", device, func);
      fatal(NULL);
    }
    break;

  case 1:
    TRACE(T_INST, " SKS '%02o%02o\n", func, device);

    if (func == 04) {             /* skip if not interrupting */
      if (!dc[device].interrupting)
	IOSKIP;

    } else {
      printf("Unimplemented SKS device '%02o function '%02o\n", device, func);
      fatal(NULL);
    }
    break;

  case 2:
    TRACE(T_INST, " INA '%02o%02o\n", func, device);

    if (func == 00) {              /* input Data Set Sense (carrier) */
      crs[A] = ~dc[device].dss;    /* to the outside world, 1 = no carrier*/
      IOSKIP;

    } else if (func == 07) {       /* input AMLC status */
      crs[A] = 040000 | (dc[device].bufnum<<8) | (dc[device].intenable<<5) | (dc[device].dmqmode<<4);
      if (dc[device].eor) {
	crs[A] |= 0100000;
	dc[device].eor = 0;
      }
      if (dc[device].ctinterrupt)
	if (dc[device].ctinterrupt & 0xfffe)
	  crs[A] |= 0xcf;          /* multiple char time interrupt */
	else
	  crs[A] |= 0x8f;          /* last line cti */
      dc[device].interrupting = 0;
      //printf("INA '07%02o returns 0x%x\n", device, crs[A]);
      IOSKIP;

    } else if (func == 011) {      /* input ID */
      crs[A] = QAMLC | 054;
      IOSKIP;

    } else {
      printf("Unimplemented INA device '%02o function '%02o\n", device, func);
      fatal(NULL);
    }
    break;

  case 3:
    TRACE(T_INST, " OTA '%02o%02o\n", func, device);

    if (func == 01) {              /* set line configuration */
      lx = crs[A] >> 12;
      //printf("OTA '01%02o: AMLC line %d config = %x\n", device, lx, crs[A]);
      /* if DTR drops on a connected line, disconnect */
      if (!(crs[A] & 0x400) && (dc[device].dss & bitmask16[lx+1])) {
	/* see similar code below */
	write(dc[device].sockfd[lx], "\r\nDisconnecting logged out session\r\n", 36);
	close(dc[device].sockfd[lx]);
	dc[device].dss &= ~bitmask16[lx+1];
	printf("em: closing AMLC line %d on device '%o\n", lx, device);
      }
      IOSKIP;

    } else if (func == 02) {      /* set line control */
      //printf("OTA '02%02o: AMLC line control = %x\n", device, crs[A]);
      lx = (crs[A]>>12);
      if (crs[A] & 040)           /* character time interrupt enable/disable */
	dc[device].ctinterrupt |= bitmask16[lx+1];
      else
	dc[device].ctinterrupt &= ~bitmask16[lx+1];
      if (crs[A] & 010)           /* transmit enable/disable */
	dc[device].xmitenabled |= bitmask16[lx+1];
      else
	dc[device].xmitenabled &= ~bitmask16[lx+1];
      if (crs[A] & 01)            /* receive enable/disable */
	dc[device].recvenabled |= bitmask16[lx+1];
      else
	dc[device].recvenabled &= ~bitmask16[lx+1];
      if ((dc[device].ctinterrupt || dc[device].xmitenabled || dc[device].recvenabled) && devpoll[device] == 0)
	devpoll[device] = AMLCPOLL*instpermsec;  /* setup another poll */
      IOSKIP;

    } else if (func == 014) {      /* set DMA/C channel (for input) */
      dc[device].dmcchan = crs[A] & 0x7ff;
      //printf("OTA '14%02o: AMLC chan = %o\n", device, dc[device].dmcchan);
      if (!(crs[A] & 0x800))
	fatal("Can't run AMLC in DMA mode!");
      IOSKIP;

    } else if (func == 015) {      /* set DMT/DMQ base address (for output) */
      dc[device].baseaddr = crs[A];
      IOSKIP;

    } else if (func == 016) {      /* set interrupt vector */
      dc[device].intvector = crs[A];
      IOSKIP;

    } else if (func == 017) {      /* set programmable clock constant */
      IOSKIP;

    } else {
      printf("Unimplemented OTA device '%02o function '%02o, A='%o\n", device, func, crs[A]);
      fatal(NULL);
    }
    break;

  case 4:
    //printf("poll device '%o, cti=%x, xmit=%x, recv=%x, dss=%x\n", device, dc[device].ctinterrupt, dc[device].xmitenabled, dc[device].recvenabled, dc[device].dss);
    
    /* check for 1 new terminal connection on each AMLC poll */

    while ((fd = accept(tsfd, (struct sockaddr *)&addr, &addrlen)) == -1 && errno == EINTR)
      ;
    if (fd == -1) {
      if (errno != EWOULDBLOCK) {
	perror("accept error for AMLC");
      }
    } else {
      if (fd >= MAXFD)
	fatal("New connection fd is too big");
      newdevice = 0;
      for (i=0; devices[i] && !newdevice && i<MAXBOARDS; i++)
	for (lx=0; lx<16; lx++) {
	  /* NOTE: don't allow connections on clock line */
	  if (lx == 15 && (i+1 == MAXBOARDS || !devices[i+1]))
	      break;
	  if (!(dc[devices[i]].dss & bitmask16[lx+1])) {
	    newdevice = devices[i];
	    socktoline[fd].device = newdevice;
	    socktoline[fd].line = lx;
	    dc[newdevice].dss |= bitmask16[lx+1];
	    dc[newdevice].sockfd[lx] = fd;
	    dc[newdevice].tstate[lx] = TS_DATA;
	    printf("em: AMLC connection, fd=%d, device='%o, line=%d\n", fd, newdevice, lx);
	    break;
	  }
	}
      if (!newdevice) {
	warn("No free AMLC connection");
	write(fd, "\rAll AMLC lines are in use!\r\n", 29);
	close(fd);
      } else {

	/* these Telnet commands put the connecting telnet client
	   into character-at-a-time mode and binary mode.  Since
	   these probably display garbage for other connection
	   methods, this stuff might be better off in a very thin
	   connection server */
	
	buf[0] = 255;   /* IAC */
	buf[1] = 251;   /* will */
	buf[2] = 1;     /* echo */
	buf[3] = 255;   /* IAC */
	buf[4] = 251;   /* will */
	buf[5] = 3;     /* supress go ahead */
	buf[6] = 255;   /* IAC */
	buf[7] = 251;   /* will */
	buf[8] = 0;     /* binary mode */
	write(fd, buf, 9);
      }
    }

    /* do a receive/transmit scan loop for every line.  This loop is
       fairly efficient because Primos turns off xmitenable on DMQ
       lines after a short time w/o any output. */

    if (dc[device].xmitenabled != 0 || dc[device].dss != 0) {
      for (lx = 0; lx < 16; lx++) {

	/* Transmit is first.  Lots of opportunity to optimize this:
	   rather than using the rtq instruction, do a mapva call to
	   access the qcb directly, pack the queue data to a buffer,
	   attempt to write the buffer to the socket, and then update
	   the qcb to reflect how many bytes were actually written.
	   This would avoid lots of get16/mapva calls and the write
	   select could be removed. 

	   NOTE: it's important to do xmit processing even if a line
	   is not currently connected, to drain the tty output buffer.
	   Otherwise, the DMQ buffer fills, this stalls the tty output
	   buffer, and when the next connection occurs on this line,
	   the output buffer from the previous terminal session will
	   then be displayed to the new user. */

	if (dc[device].xmitenabled & bitmask16[lx+1]) {
	  n = 0;
	  if (dc[device].dmqmode) {
	    qcbea = dc[device].baseaddr + lx*4;
	    if (dc[device].dss & bitmask16[lx+1]) {

	      /* ensure there's some room in the socket buffer */

	      timeout.tv_sec = 0;
	      timeout.tv_usec = 0;
	      FD_ZERO(&fds);
	      FD_SET(dc[device].sockfd[lx], &fds);
	      if (select(dc[device].sockfd[lx]+1, NULL, &fds, NULL, &timeout) <= 0)
		continue;                /* no room at the inn */

	      /* pack DMQ characters into a buffer & fix parity */

	      n = 0;
	      while (n < sizeof(buf) && rtq(qcbea, &utempa, 0)) {
		if (utempa & 0x8000) {           /* valid character */
		  //printf("Device %o, line %d, entry=%o (%c)\n", device, lx, utempa, utempa & 0x7f);
		  buf[n++] = utempa & 0x7F;
		}
	      }
	    } else {         /* no line connected, just drain queue */
	      //printf("Draining output queue on line %d\n", lx);
	      put16r0(get16r0(qcbea), qcbea+1);
	    }
	  } else {  /* DMT */
	    utempa = get16r0(dc[device].baseaddr + lx);
	    if (utempa != 0) {
	      if ((utempa & 0x8000) && (dc[device].dss & bitmask16[lx+1])) {
		//printf("Device %o, line %d, entry=%o (%c)\n", device, lx, utempa, utempa & 0x7f);
		buf[n++] = utempa & 0x7F;
	      }
	      put16r0(0, dc[device].baseaddr + lx);
	    }
	    /* would need to setup DMT xmit poll here, and/or look for
	       char time interrupt.  In practice, DMT isn't used when
	       the AMLC device is configured as a QAMLC */
	  }

	  /* NOTE: on a partial write, terminal data is lost.  The best
	     option would be to find out how much room is left in the
	     socket buffer and only remove that many characters from the
	     queue in the loop above.

	     Mac OSX write selects on sockets will only return true if
	     there are SO_SNDLOWAT (defaults to 1024) bytes available in
	     the write buffer.  Since the max DMQ buffer size is also
	     1024, the write below should never fail because of the
	     write select above.  On other OS's, it may be necessary to
	     limit the number of characters dequeued in the loop above.
	  */

	  if (n > 0)
	    if ((nw = write(dc[device].sockfd[lx], buf, n)) != n) {
	      perror("Writing to AMLC");
	      fprintf(stderr," %d bytes written, but %d bytes sent\n", n, nw);
	    }
	}

	/* process input, but only as much as will fit into the DMC buffer.
	   NOTE: because the size of the AMLC tumble tables is limited, this
	   could pose a denial of service issue.  Input should probably be
	   processed in a round to be more fair with this limited resource.

	   The AMLC tumble tables should never overflow, because we only
	   read as many characters from the socket buffers as will fit in 
	   the tumble tables. */

	if ((dc[device].dss & dc[device].recvenabled & bitmask16[lx+1]) && !dc[device].eor) {
	  if (dc[device].bufnum)
	    dmcea = dc[device].dmcchan ^ 2;
	  else
	    dmcea = dc[device].dmcchan;
	  dmcpair = get32r0(dmcea);
	  dmcbufbegea = dmcpair>>16;
	  dmcbufendea = dmcpair & 0xffff;
	  dmcnw = dmcbufendea - dmcbufbegea + 1;
	  if (dmcnw <= 0)
	    continue;
	  if (dmcnw > sizeof(buf))
	    dmcnw = sizeof(buf);
	  while ((n = read(dc[device].sockfd[lx], buf, dmcnw)) == -1 && errno == EINTR)
	    ;
	  //printf("processing recv on device %o, line %d, b#=%d, dmcnw=%d, n=%d\n", device, lx, dc[device].bufnum, dmcnw, n);

	  /* zero length read means the socket has been closed */

	  if (n == 0) {
	    n = -1;
	    errno = EPIPE;
	  }
	  if (n == -1) {
	    n = 0;
	    if (errno == EAGAIN || errno == EWOULDBLOCK)
	      ;
	    else if (errno == EPIPE || errno == ECONNRESET) {

	      /* see similar code above */
	      close(dc[device].sockfd[lx]);
	      dc[device].dss &= ~bitmask16[lx+1];
	      printf("Closing AMLC line %d on device '%o\n", lx, device);
	    } else {
	      perror("Reading AMLC");
	    }
	  }

	  /* very primitive support here for telnet - only enough to
	     ignore commands sent by the telnet client.  Telnet
	     commands could be split across reads and AMLC interrupts,
	     so a small state machine is used for each line */

	  if (n > 0) {
	    state = dc[device].tstate[lx];
	    for (i=0; i<n; i++) {
	      ch = buf[i];
	      switch (state) {
	      case TS_DATA:
		if (ch == 255)
		  state = TS_IAC;
		else if (ch != 0) {
storech:
		  utempa = lx<<12 | 0x0200 | ch;
		  put16r0(utempa, dmcbufbegea);
		  //printf("******* stored character %o (%c) at %o\n", utempa, utempa&0x7f, dmcbufbegea);
		  dmcbufbegea = INCVA(dmcbufbegea, 1);
		}
		break;
	      case TS_IAC:
		switch (ch) {
		case 255:
		  state = TS_DATA;
		  goto storech;
		case 251:   /* will */
		case 252:   /* won't */
		case 253:   /* do */
		case 254:   /* don't */
		  state = TS_OPTION;
		  break;
		case 250:   /* begin suboption */
		  state = TS_SUBOPT;
		  break;
		default:    /* ignore other chars after IAC */
		  state = TS_DATA;
		}
		break;
	      case TS_SUBOPT:
		if (ch == 255)
		  state = TS_IAC;
		break;
	      case TS_OPTION:
	      default:
		state = TS_DATA;
	      }
	    }
	    dc[device].tstate[lx] = state;
	    if (dmcbufbegea-1 > dmcbufendea)
	      fatal("AMLC tumble table overflowed?");
	    put16r0(dmcbufbegea, dmcea);
	    if (dmcbufbegea > dmcbufendea) {          /* end of range has occurred */
	      dc[device].bufnum = 1-dc[device].bufnum;
	      dc[device].eor = 1;
	    }
	  }
	}
      }
    }

    /* time to interrupt? */

    if (dc[device].intenable && (dc[device].ctinterrupt || dc[device].eor)) {
      if (intvec == -1) {
	intvec = dc[device].intvector;
	dc[device].interrupting = 1;
      } else
	devpoll[device] = 100;         /* come back soon! */
    }

    /* conditions to setup another poll:
       - any board with ctinterrupt set on any line is polled
       - any board with xmitenabled on any line is polled (to drain output)
       - any board with recvenabled on a connected line is polled
       - the device must not already be set for a poll

       NOTE: there is always at least one board with ctinterrupt set
       (the last board), so it will always be polling and checking
       for new incoming connections */

    if ((dc[device].ctinterrupt || dc[device].xmitenabled || (dc[device].recvenabled & dc[device].dss)) && devpoll[device] == 0)
      devpoll[device] = AMLCPOLL*instpermsec;  /* setup another poll */
    break;
  }
}



/* PNC (ring net) device handler

  On a real Prime ring network, each node has a unique node id from 1
  to 254.  The node id is configured with software and stored into the
  PNC during network initialization.  In practice, CONFIG_NET limits
  the node id to 247.  When a node starts, it sends a message to
  itself.  If any system acknowledges this packet, it means the node
  id is already in use.  If this occcurs, the new node will disconnect
  from the ring.  Since all systems in a ring have to be physically
  cabled together, there was usually a common network administration
  to ensure that no two nodes had the same node id.

  Node id 255 is the broadcast id - all nodes receive these messages.
  Beginning with 19.3, An "I'm alive" broadcast message is sent every
  10 seconds to let all nodes know that a machine is still up.

  For use with the emulator, the unique node id concept doesn't make a
  lot of sense.  If a couple of guys running the emulator wanted to
  have a 2-node network with nodes 1 and 2 (network A), and another
  couple of guys had a 2-node network with nodes 1 and 2 (network B),
  then it wouldn't be possible for a node in one network to add a node
  in the other network without other people redoing their node ids to
  ensure uniqueness.  To get around this, the emulator has a config
  file RING.CFG that lets you say "Here are the guys in *my* ring
  (with all unique node ids), here is each one's IP address and port,
  my node id on their ring is *blah*, their node id on their ring is
  *blah2*"".  This allows the node id to be a per-host number that
  only needs to be coordinated with two people that want to talk to
  each other, and effectively allows one emulator to be in multiple
  rings simulataneously.  Cool, eh?

  PNC ring buffers are 256 words, with later versions of Primos also
  supporting 512, and 1024 word packets.  "nbkini" (rev 18) allocates
  12 ring buffers + 1 for every 2 nodes in the ring.  Both the 1-to-2
  queue for received packets, and 2-to-1 queue for xmit packets have
  63 entries.  PNC ring buffers are wired and never cross page
  boundaries.

  The actual xmit/recv buffers for PNC are 256-1024 words, and each
  buffer has an associated "block header", stored in a different
  location in system memory, that is 8 words.

  The BH fields are (16-bit words):
       0: type field (1)
       1: free pool id (3 for ring buffers)
       2-3: pointer to data block

       4-7 are available for use by the drivers.  For PNC:
       4: number of words received (calculated by pncdim based on DMA regs)
       5: receive status word
       6: data 1
       7: data 2

  The PNC data buffer has a 2-word header, followed by the data:
       0: To (left) and From (right) bytes containing node-ids
       1: "Type" word. 
          Bit 1 set = odd number of bytes (if set, last word is only 1 byte)
          Bit 7 set = normal data messages (otherwise, a timer message)
	  Bit 16 set = broadcast timer message
  PNC data buffers never cross page boundaries.

  Primos PNC usage:

  OCP '0007
  - disconnect from ring

  OCP '0207
  - inject a token into the ring

  OCP '0507
  - set PNC into "delay" mode (token recovery BS)

  OCP '1007
  - stop any xmit in progress


  INA '1707
  - read PNC status word
  - does this in a loop until "connected" bit is clear after disconnect above
  - PNC status word:
      bit 1 set if receive interrupt (rcv complete)
      bit 2 set if xmit interrupt (xmit complete)
      bit 3 set if "PNC booster" (repeater?)
      bit 4-5 not used
      bit 6 set if connected to ring
      bit 7 set if multiple tokens detected (only after xmit EOR)
      bit 8 set if token detected (only after xmit EOR)
      bits 9-16 controller node ID

  INA '1207
  - read receive status word (byte?)
      bit 1 set for previous ACK
      bit 2 set for multiple previous ACK
      bit 3 set for previous WACK
      bit 4 set for previous NACK
      bits 5-6 unused
      bit 7 ACK byte parity error
      bit 8 ACK byte check error (parity on bits 1-6)
      bit 9 recv buffer parity error
      bit 10 recv busy
      bit 11 end of range before end of message (received msg was too big
             for the receive buffer)
      bits 12-16 unused

  INA '1307
  - read xmit status word
      bit 1 set for ACK
      bit 2 set for multiple ACK (more than 1 node accepted packet)
      bit 3 set for WACK
      bit 4 set for NACK (bad CRC)
      bit 5 unused
      bit 6 parity bit of ACK (PNC only)
      bit 7 ACK byte parity error
      bit 8 ACK byte check error (parity on bits 1-6)
      bit 9 xmit buffer parity error
      bit 10 xmit busy
      bit 11 packet did not return
      bit 12 packet returned with bits 6-8 nonzero
      bits 13-16 retry count (booster only, zero on PNC)

  OCP '1707
  - initialize

  OTA '1707
  - set my node ID (in A register)
  - if this fails, no PNC present

  OTA '1607
  - set interrupt vector (in A reg)

  OTA '1407
  - initiate receive, dma channel in A

  OTA '1507
  - initiate xmit, dma channel in A

  OCP '0107
  - connect to the ring
  - (Primos follows this with INA '1707 until "connected" bit is set)

  OCP '1407
  - ack receive (clears recv interrupt request)

  OCP '0407
  - ack xmit (clears xmit interrupt request)

  OCP '1507
  - set interrupt mask (enable interrupts)

  OCP '1107
  - rev 20 does this, not sure what it does

*/

int devpnc (int class, int func, int device) {

#define PNCPOLL 100

  /* PNC controller status bits */
#define PNCRCVINT  0x8000    /* bit 1 rcv interrupt (rcv complete) */
#define PNCXMITINT 0x4000    /* bit 2 xmit interrupt (xmit complete) */
#define PNCBOOSTER 0x2000    /* bit 3 PNC booster = 1 */

#define PNCCONNECTED 0x400   /* bit 6 */
#define PNCTWOTOKENS 0x200   /* bit 7, only set after xmit EOR */
#define PNCTOKDETECT 0x100   /* bit 8, only set after xmit EOR */

  static short configured = 0; /* true if PNC configured */
  static unsigned short pncstat;    /* controller status word */
  static unsigned short recvstat;   /* receive status word */
  static unsigned short xmitstat;   /* xmit status word */
  static unsigned short pncvec;     /* PNC interrupt vector */
  static unsigned short myid;       /* my PNC node id */
  static unsigned short enabled;    /* interrupts enabled flag */
  static int pncfd;        /* socket fd for all PNC network connections */

  /* the ni structure contains the important information for each node
     in the network and is indexed by the local node id */

  static struct {          /* node info for each node in my network */
    short cfg;             /* true if seen in ring.cfg */
    short fd;              /* socket fd for this node, -1 if unconnected */
    char  ip[16];          /* IP address of the remote node */
    short port;            /* emulator network port on the remote node */
    short myremid;         /* my node ID on the remote node's ring network */
    short yourremid;       /* remote node's id on its own network */
  } ni[256];

  /* array to map socket fd's to local node id's for accessing the ni
     structure.  Not great programming, because the host OS could
     choose to use large fd's, which will cause a runtime error */

#define FDMAPSIZE 1024
  static short fdnimap[FDMAPSIZE];

  typedef struct {
    unsigned short dmachan, dmareg, dmaaddr;
    short dmanw, toid, fromid, remtoid, remfromid;
    unsigned short *iobufp;
  } t_dma;
  static t_dma recv, xmit;

  short i;
  unsigned short access, dmaword, dmaword1, dmaword2;

  struct sockaddr_in addr;
  int fd, optval, fdflags;
  unsigned int addrlen;

  FILE *ringfile;
  char *tok, buf[128], *p;
  int n, linenum;
  int tempid, tempmyremid, tempyourremid, tempport, cfgerrs;
  char tempip[22];       /* xxx.xxx.xxx.xxx:ppppp */
#define DELIM " \t\n"
#define PDELIM ":"

  //traceflags = ~TB_MAP;

  if (nport <= 0) {
    if (class == -1)
      fprintf(stderr, "-nport is zero, PNC not started\n");
    else
      TRACE(T_INST|T_RIO, "nport <= 0, PIO to PNC ignored, class=%d, func='%02o, device=%02o\n", class, func, device);
    return -1;
  }

  switch (class) {

  case -1:
    for (i=0; i<FDMAPSIZE; i++)
      fdnimap[i] = -1;
    for (i=0; i<256; i++) {
      ni[i].cfg = 0;
      ni[i].fd = -1;
      strcpy(ni[i].ip, "               ");
      ni[i].port = 0;
      ni[i].myremid = 0;
      ni[i].yourremid = 0;
    }
    myid = 0;                 /* set an invalid node id */

    /* read the ring.cfg config file.  Each line contains:
          localid  ip:port  [myremoteid yourremoteid]
       where:
          localid = the remote node's id (1-247) on my ring
          ip = the remote emulator's TCP/IP address (or later, name)
	  port = the remote emulator's TCP/IP PNC port
          myremoteid = my node's id (1-247) on the remote ring
	  yourremoteid = the remote node's id on its own ring

       The two remote id fields are optional, and allow the emulator
       to be in multiple rings with different administration.  If
       these are not specified, myremoteid = my local id, and
       yourremoteid = localid (the remote host's local id).

       There cannot be duplicates among the localid field, but there
       can be duplicates in the remoteid fields */

    linenum = 0;
    if ((ringfile=fopen("ring.cfg", "r")) != NULL) {
      while (fgets(buf, sizeof(buf), ringfile) != NULL) {
	linenum++;
	if (strcmp(buf,"") == 0 || buf[0] == ';')
	  continue;
	if ((p=strtok(buf, DELIM)) == NULL) {
	  fprintf(stderr,"Line %d of ring.cfg: node id missing\n", linenum);
	  continue;
	}
	tempid = atoi(p);
	if (tempid < 1 || tempid > 247) {
	  fprintf(stderr,"Line %d of ring.cfg: node id is out of range 1-247\n", linenum);
	  continue;
	}
	if (ni[tempid].cfg) {
	  fprintf(stderr,"Line %d of ring.cfg: node id occurs twice\n", linenum);
	  continue;
	}
	if ((p=strtok(NULL, DELIM)) == NULL) {
	  fprintf(stderr,"Line %d of ring.cfg: IP address missing\n", linenum);
	  continue;
	}
	if (strlen(p) > 21) {
	  fprintf(stderr,"Line %d of ring.cfg: IP address is too long\n", linenum);
	  continue;
	}
	strcpy(tempip, p);
	if ((p=strtok(NULL, DELIM)) != NULL) {
	  tempmyremid = atoi(p);
	  if (tempmyremid < 1 || tempmyremid > 247) {
	    fprintf(stderr,"Line %d of ring.cfg: my remote node id out of range 1-247\n", linenum);
	    continue;
	  }
	} else 
	  tempmyremid = -1;
	if ((p=strtok(NULL, DELIM)) != NULL) {
	  tempyourremid = atoi(p);
	  if (tempyourremid < 1 || tempyourremid > 247) {
	    fprintf(stderr,"Line %d of ring.cfg: your remote node id out of range 1-247\n", linenum);
	    continue;
	  }
	} else 
	  tempyourremid = tempid;
	if (tempyourremid == tempmyremid) {
	    fprintf(stderr,"Line %d of ring.cfg: my remote node id and your remote node id can't be equal\n", linenum);
	    continue;
	  }
	/* parse the port number from the IP address */
	tempport = -1;
	if ((p=strtok(tempip, PDELIM)) != NULL) {
	  strcpy(ni[tempid].ip, p);
	  if ((p=strtok(NULL, PDELIM)) != NULL) {
	    tempport = atoi(p);
	    if (tempport < 1 || tempport > 32000)
	      fprintf(stderr,"Line %d of ring.cfg: port number is out of range 1-32000\n", linenum);
	  }
	}
	if (tempport == -1) {
	  fprintf(stderr, "Line %d of ring.cfg: IP should be xxx.xxx.xxx.xxx:pppp\n", linenum);
	  continue;
	}
	ni[tempid].cfg = 1;
	ni[tempid].myremid = tempmyremid;
	ni[tempid].yourremid = tempyourremid;
	ni[tempid].port = tempport;
	TRACE(T_RIO, "Line %d: id=%d, ip=\"%s\", port=%d, myremid=%d, yourremid=%d\n", linenum, tempid, tempip, tempport, tempmyremid, tempyourremid);
	configured = 1;
      }
      if (!feof(ringfile)) {
	perror(" error reading ring.cfg");
	fatal(NULL);
      }
      fclose(ringfile);
    }
    if (!configured) {
      fprintf(stderr, "PNC not configured because ring.cfg missing or errors occurred.\n");
      return -1;
    }
    return 0;

  case 0:

    /* OCP '0700 - disconnect */

    if (func == 00) {
      TRACE(T_INST|T_RIO, " OCP '%02o%02o - disconnect\n", func, device);
      if (pncfd >= 0) {
	close(pncfd);
	pncfd = -1;
      }
      for (i=0; i<256; i++) {
	if (ni[i].fd >= 0) {
	  fdnimap[ni[i].fd] = -1;
	  close(ni[i].fd);
	  ni[i].fd = -1;
	}
      }
      pncstat &= ~PNCCONNECTED;

    /* OCP '0701 - connect 
       If any errors occur while trying to setup the listening socket,
       it seems reasonable to leave the PNC unconnected and continue,
       but this will cause Primos (rev 19) to hang in a spin loop. So
       for now, bomb.  Later, we could put the PNC in a disabled state,
       where INA/OTA don't skip, since Primos handles this better.
    */

    } else if (func == 01) {    /* connect to the ring */
      TRACE(T_INST|T_RIO, " OCP '%02o%02o - connect\n", func, device);

      /* start listening on the network port */

      pncfd = socket(AF_INET, SOCK_STREAM, 0);
      if (pncfd == -1) {
	perror("socket failed for PNC");
	fatal(NULL);
      }
      if (fcntl(pncfd, F_GETFL, fdflags) == -1) {
	perror("unable to get ts flags for PNC");
	fatal(NULL);
      }
      fdflags |= O_NONBLOCK;
      if (fcntl(pncfd, F_SETFL, fdflags) == -1) {
	perror("unable to set ts flags for PNC");
	fatal(NULL);
      }
      optval = 1;
      if (setsockopt(pncfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval))) {
	perror("setsockopt failed for PNC");
	fatal(NULL);
      }
      addr.sin_family = AF_INET;
      addr.sin_port = htons(nport);
      addr.sin_addr.s_addr = INADDR_ANY;
      if(bind(pncfd, (struct sockaddr *)&addr, sizeof(addr))) {
	perror("bind: unable to bind for PNC");
	fatal(NULL);
      }
      if(listen(pncfd, 10)) {
	perror("listen failed for PNC");
	fatal(NULL);
      }
      pncstat |= PNCCONNECTED;

    } else if (func == 02) {    /* inject a token */
      TRACE(T_INST|T_RIO, " OCP '%02o%02o - inject token\n", func, device);

    } else if (func == 04) {    /* ack xmit (clear xmit int) */
      TRACE(T_INST|T_RIO, " OCP '%02o%02o - ack xmit int\n", func, device);
      pncstat &= ~PNCXMITINT;   /* clear "xmit interrupting" */
      pncstat &= ~PNCTOKDETECT; /* clear "token detected" */
      xmitstat = 0;

    } else if (func == 05) {    /* set PNC into "delay" mode */
      TRACE(T_INST|T_RIO, " OCP '%02o%02o - set delay mode\n", func, device);

    } else if (func == 010) {   /* stop xmit in progress */
      TRACE(T_INST|T_RIO, " OCP '%02o%02o - stop xmit\n", func, device);
      xmitstat = 0;

    } else if (func == 011) {   /* dunno what this is - rev 20 startup */
      TRACE(T_INST|T_RIO, " OCP '%02o%02o - unknown\n", func, device);
      ;

    } else if (func == 012) {   /* set normal mode */
      TRACE(T_INST|T_RIO, " OCP '%02o%02o - set normal mode\n", func, device);
      ;

    } else if (func == 013) {   /* set diagnostic mode */
      TRACE(T_INST|T_RIO, " OCP '%02o%02o - set diag mode\n", func, device);
      ;

    } else if (func == 014) {   /* ack receive (clear rcv int) */
      TRACE(T_INST|T_RIO, " OCP '%02o%02o - ack recv int\n", func, device);
      pncstat &= ~PNCRCVINT;
      recvstat = 0;

    } else if (func == 015) {   /* set interrupt mask (enable int) */
      TRACE(T_INST|T_RIO, " OCP '%02o%02o - enable interrupts\n", func, device);
      enabled = 1;

    } else if (func == 016) {   /* clear interrupt mask (disable int) */
      TRACE(T_INST|T_RIO, " OCP '%02o%02o - disable interrupts\n", func, device);
      enabled = 0;

    } else if (func == 017) {   /* initialize */
      TRACE(T_INST|T_RIO, " OCP '%02o%02o - initialize\n", func, device);
      pncstat = 0;
      recvstat = 0;
      xmitstat = 0;
      pncvec = 0;
      enabled = 0;

    } else {
      printf("Unimplemented OCP device '%02o function '%02o\n", device, func);
      fatal(NULL);
    }
    break;

  case 1:
    TRACE(T_INST|T_RIO, " SKS '%02o%02o\n", func, device);
    if (func == 99)
      IOSKIP;                     /* assume it's always ready */
    else {
      printf("Unimplemented SKS device '%02o function '%02o\n", device, func);
      fatal(NULL);
    }
    break;

  case 2:
    if (func == 011) {          /* input ID */
      TRACE(T_INST|T_RIO, " INA '%02o%02o - input ID\n", func, device);
      crs[A] = 07;
      IOSKIP;

    } else if (func == 012) {   /* read receive status word */   
      TRACE(T_INST|T_RIO, " INA '%02o%02o - get recv status '%o\n", func, device, recvstat);
      crs[A] = recvstat;
      IOSKIP;

    } else if (func == 013) {   /* DIAG - read static register; not impl. */
      crs[A] = 0;
      IOSKIP;

    } else if (func == 014) {   /* read xmit status word */   
      TRACE(T_INST|T_RIO, " INA '%02o%02o - get xmit status '%o\n", func, device, xmitstat);
      crs[A] = xmitstat;
      IOSKIP;

    } else if (func == 017) {   /* read controller status word */   
      TRACE(T_INST|T_RIO, " INA '%02o%02o - get ctrl status '%o\n", func, device, pncstat);
      crs[A] = pncstat;
      IOSKIP;

    } else {
      printf("Unimplemented INA device '%02o function '%02o\n", device, func);
      fatal(NULL);
    }
    break;

  case 3:
    TRACE(T_INST|T_RIO, " OTA '%02o%02o\n", func, device);
    if (func == 011) {          /* DIAG - single step; not impl.*/
      IOSKIP;

    } else if (func == 014) {   /* initiate recv, dma chan in A */
      recvstat = 0x0040;        /* set receive busy */
      recv.dmachan = crs[A];
      recv.dmareg = recv.dmachan << 1;
      recv.dmanw = regs.sym.regdmx[recv.dmareg];
      if (recv.dmanw <= 0)
	recv.dmanw = -(recv.dmanw>>4);
      else
	recv.dmanw = -((recv.dmanw>>4) ^ 0xF000);
      recv.dmaaddr = regs.sym.regdmx[recv.dmareg+1];
      recv.iobufp = mem + mapva(recv.dmaaddr, WACC, &access, 0);
      TRACE(T_INST|T_RIO, " recv: dmachan=%o, dmareg=%o, dmaaddr=%o, dmanw=%d\n", recv.dmachan, recv.dmareg, recv.dmaaddr, recv.dmanw);
      devpoll[device] = 10;
      IOSKIP;

    } else if (func == 015) {   /* initiate xmit, dma chan in A */
      xmitstat = 0x0040;        /* set xmit busy */
      xmit.dmachan = crs[A];
      xmit.dmareg = xmit.dmachan<<1;
      xmit.dmanw = regs.sym.regdmx[xmit.dmareg];
      if (xmit.dmanw <= 0)
	xmit.dmanw = -(xmit.dmanw>>4);
      else
	xmit.dmanw = -((xmit.dmanw>>4) ^ 0xF000);
      xmit.dmaaddr = regs.sym.regdmx[xmit.dmareg+1];
      TRACE(T_INST|T_RIO, " xmit: dmachan=%o, dmareg=%o, dmaaddr=%o, dmanw=%d\n", xmit.dmachan, xmit.dmareg, xmit.dmaaddr, xmit.dmanw);

      /* PNC transmit:

         NOTE: most of this stuff needs to be moved to poll, because
	 it may not be possible to start/complete a transmit here!
	 
	 get a physical pointer to the xmit buffer, which can't cross
	 a page boundary, then xmit the buffer.  Since PNC packets
	 have a specific size of either 512, 1024, or 2048 bytes, and
	 we're using a byte-stream for communication, 2 extra header
	 bytes are sent with the packet length (in words), to let the
	 other end know how big the packet is.
      */

      xmit.iobufp = mem + mapva(xmit.dmaaddr, RACC, &access, 0);

      /* read the first word, the to and from node id's, and map them
	 to the remote hosts' expected to and from node id's */

      dmaword = *xmit.iobufp++;
      xmit.toid = dmaword >> 8;
      xmit.fromid = dmaword & 0xFF;
      TRACE(T_INST|T_RIO, " xmit: dmaword = '%o/%d [%03o %03o]\n", dmaword, *(short *)&dmaword, dmaword>>8, dmaword&0xff);

      /* broadcast packets are "I am up" msgs and are simply "eaten"
	 here, as this is handled later in the devpnc poll code.
	 XXX: should check that this really is the "I am up" msg */
      
      if (xmit.toid == 255) {
	regs.sym.regdmx[xmit.dmareg+1] += xmit.dmanw;   /* bump xmit address */
	regs.sym.regdmx[xmit.dmareg] += xmit.dmanw;     /* and count */
	goto xmitdone;
      }

      /* check for errors, then map to and from node id to remote to
	 and from node id */

      if (xmit.toid == 0)
	fatal("PNC: xmit.toid is zero");
      if (xmit.fromid == 0)
	fatal("PNC: xmit.fromid is zero");
      if (xmit.fromid != myid) {
	printf("PNC: xmit fromid=0x%02x != myid=0x%02x\n", xmit.fromid, myid);
	fatal(NULL);
      }

      if (ni[xmit.fromid].myremid > 0)
	xmit.remfromid = ni[xmit.fromid].myremid;
      else
	xmit.remfromid = myid;
      xmit.remtoid = ni[xmit.toid].yourremid;

      dmaword1 = (xmit.remtoid << 8) | xmit.remfromid;
      for (i=0; i < xmit.dmanw; i++) {
	if (i == 0)
	  dmaword = dmaword1;
	else
	  dmaword = *xmit.iobufp++;
	TRACE(T_INST|T_RIO, " xmit: word %d = '%o/%d [%03o %03o]\n", i, dmaword, *(short *)&dmaword, dmaword>>8, dmaword&0xff);

	/* if this xmit is local and there is a receive pending and there is
	   room left in the receive buffer, put the word directly in my 
	   receive buffer.  If it's local but we can't receive it, set
	   NACK xmit and receive status */

	if (xmit.toid == myid) {
	  if ((recvstat & 0x0040) && regs.sym.regdmx[recv.dmareg]) {
	    *recv.iobufp++ = dmaword;
	    regs.sym.regdmx[recv.dmareg]++;     /* bump count */
	    regs.sym.regdmx[recv.dmareg+1]++;   /* bump address */
	  } else {
	    xmitstat |= 0x1000;             /* set xmit NACK status */
	    recvstat |= 0x20;               /* set recv premature EOR */
	  }
	} else {
	  /* send remote over sockets */
	}
	regs.sym.regdmx[xmit.dmareg+1]++;   /* bump xmit address */
	regs.sym.regdmx[xmit.dmareg]++;     /* and count */
      }

xmitdone:
      pncstat |= 0x4100;                    /* set xmit interrupt + token */
      if (xmit.toid == myid)
	pncstat |= 0x8000;                  /* set recv interrupt too */
      if (xmitstat == 0x0040)               /* complete w/o errors? */
	xmitstat |= 0x8000;                 /* yes, set ACK xmit status */

      devpoll[device] = PNCPOLL*instpermsec;
      if (enabled && (pncstat & 0xC000))
	if (intvec == -1)
	  intvec = pncvec;
	else
	  devpoll[device] = 100;
      IOSKIP;

    } else if (func == 016) {   /* set interrupt vector */
      pncvec = crs[A];
      TRACE(T_INST|T_RIO, " interrupt vector = '%o\n", pncvec);
      IOSKIP;

    } else if (func == 017) {   /* set my node ID */
      myid = crs[A] & 0xFF;
      pncstat = (pncstat & 0xFF00) | myid;
      TRACE(T_INST|T_RIO, " my node id is %d\n", myid);
      if (ni[myid].cfg)
	fprintf(stderr, "Warning: my node id of %d is in ring.cfg\n", myid);
      strcpy(ni[myid].ip, "127.0.0.1");
      ni[myid].port = nport;
      ni[myid].myremid = myid;
      ni[myid].yourremid = myid;
      IOSKIP;

    } else {
      printf("Unimplemented OTA device '%02o function '%02o, A='%o\n", device, func, crs[A]);
      fatal(NULL);
    }
    break;

  case 4:
    TRACE(T_INST|T_RIO, " POLL '%02o%02o\n", func, device);

#if 0
    while ((fd = accept(pncfd, (struct sockaddr *)&addr, &addrlen)) == -1 && errno == EINTR)
      ;
    if (fd == -1) {
      if (errno != EWOULDBLOCK) {
	perror("accept error for PNC");
      }
    } else {
      if (fd >= MAXFD)
	fatal("New connection fd is too big");
      newdevice = 0;
      for (i=0; devices[i] && !newdevice && i<MAXBOARDS; i++)
	for (lx=0; lx<16; lx++)
	  if (!(dc[devices[i]].dss & bitmask16[lx+1])) {
	    newdevice = devices[i];
	    socktoline[fd].device = newdevice;
	    socktoline[fd].line = lx;
	    dc[newdevice].dss |= bitmask16[lx+1];
	    dc[newdevice].sockfd[lx] = fd;
	    printf("em: AMLC connection, fd=%d, device='%o, line=%d\n", fd, newdevice, lx);
	    break;
	  }
      if (!newdevice) {
	warn("No free AMLC connection");
	write(fd, "\rAll AMLC lines are in use!\r\n", 29);
	close(fd);
      }
    }
#endif
    devpoll[device] = PNCPOLL*instpermsec;
    break;

  default:
    fatal("Bad func in devpcn");
  }
}
