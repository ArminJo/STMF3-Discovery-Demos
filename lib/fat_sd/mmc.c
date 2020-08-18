/*-----------------------------------------------------------------------*/
/* MMCv3/SDv1/SDv2 (in SPI mode) control module  (C)ChaN, 2010           */
/*-----------------------------------------------------------------------*/

#define _SYS_SELECT_H //to avoid error "conflicting types for 'select'"
#include "timing.h"
#include "stm32fx0xPeripherals.h"
#include "diskio.h"
#include "stm32f3_discovery.h" // for LED10
#include "stm32fx0xPeripherals.h" // For Watchdog_reload()

#include "main.h" //for sStringBuffer

#include <stdio.h> // for snprintf
#include <string.h> // for strcpy

#include "ff.h"
extern FATFS Fatfs[1];
/*--------------------------------------------------------------------------

 Module Private Functions

 ---------------------------------------------------------------------------*/

/* Definitions for MMC/SDC command */
#define CMD0	(0)			/* GO_IDLE_STATE */
#define CMD1	(1)			/* SEND_OP_COND (MMC) */
#define	ACMD41	(0x80+41)	/* SEND_OP_COND (SDC) */
#define CMD8	(8)			/* SEND_IF_COND */
#define CMD9	(9)			/* SEND_CSD */
#define CMD10	(10)		/* SEND_CID */
#define CMD12	(12)		/* STOP_TRANSMISSION */
#define ACMD13	(0x80+13)	/* SD_STATUS (SDC) */
#define CMD16	(16)		/* SET_BLOCKLEN */
#define CMD17	(17)		/* READ_SINGLE_BLOCK */
#define CMD18	(18)		/* READ_MULTIPLE_BLOCK */
#define CMD23	(23)		/* SET_BLOCK_COUNT (MMC) */
#define	ACMD23	(0x80+23)	/* SET_WR_BLK_ERASE_COUNT (SDC) */
#define CMD24	(24)		/* WRITE_BLOCK */
#define CMD25	(25)		/* WRITE_MULTIPLE_BLOCK */
#define CMD32  (32)			/* ERASE_ER_BLK_START */
#define CMD33  (33)			/* ERASE_ER_BLK_END */
#define CMD38  (38)			/* ERASE */
#define CMD55	(55)		/* APP_CMD */
#define CMD58	(58)		/* READ_OCR */

/* Card-Select Controls  (Platform dependent) */
#define CS_LOW()        MICROSD_CSEnable()    /* MMC CS = L */
#define CS_HIGH()      MICROSD_CSDisable()   /* MMC CS = H */
#define	FCLK_SLOW()	SPI1_setPrescaler(SPI_BAUDRATEPRESCALER_256)		/* Set slow clock (F_CPU / 256)  280kHz at 72 Mhz Clock*/
//extern int testValueForExtern;
//#define	FCLK_FAST()	SPI1_setPrescaler(testValueForExtern)	/* Set fast clock (F_CPU / 4) */
#define	FCLK_FAST()	SPI1_setPrescaler(SPI_BAUDRATEPRESCALER_2)	/* Set fastest clock (F_CPU / 2) */
#define TIMEOUT_WAIT_READY 1000

static volatile DSTATUS Stat = STA_NOINIT; /* Disk status */

static BYTE CardType; /* Card type flags */

static
int power_status(void) /* Socket power state: 0=off, 1=on */
{
    return 1;
}

static
void power_on(void) {
}

static
void power_off(void) {
}

/*-----------------------------------------------------------------------*/
/* Transmit/Receive a byte to MMC via SPI  (Platform dependent)          */
/*-----------------------------------------------------------------------*/
static BYTE stm32_spi_rw(BYTE out) {
    /* Send byte through the SPI peripheral */
    return SPI1_sendReceiveFast(out);
//    SPI_SendData8(SPI1, out);
//
//    /* Wait to receive a byte */
//    while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_RXNE ) == RESET) {
//        ;
//    }
//
//    /* Return the byte read from the SPI bus */
//    return SPI_ReceiveData8(SPI1 );
}
/*-----------------------------------------------------------------------*/
/* Transmit a byte to MMC via SPI  (Platform dependent)                  */
/*-----------------------------------------------------------------------*/

#define xchg_spi(dat)  stm32_spi_rw(dat)

/*-----------------------------------------------------------------------*/
/* Receive a byte from MMC via SPI  (Platform dependent)                 */
/*-----------------------------------------------------------------------*/

static BYTE rcvr_spi(void) {
    return stm32_spi_rw(0xff);
}

/*-----------------------------------------------------------------------*/
/* Wait for card ready                                                   */
/*-----------------------------------------------------------------------*/

static
int wait_ready( /* 1:Ready, 0:Timeout */
UINT wt /* Timeout [ms] */
) {
    uint32_t tLR14 = getLR14();
    // setTimeoutMillis(wt);  does not work because setTimeoutMillis() is not reentrant and used by calling routine too
    uint32_t tTimestamp = getMillisSinceBoot() + wt;
    while (rcvr_spi() != 0xFF) {
        if (getMillisSinceBoot() > tTimestamp) {
            failParamMessage(tLR14, "Timeout in wait_ready()");
            break;
        }
    }
    return (rcvr_spi() == 0xFF) ? 1 : 0;
}

/*-----------------------------------------------------------------------*/
/* Deselect the card and release SPI bus                                 */
/*-----------------------------------------------------------------------*/

static
void deselect(void) {
    CS_HIGH();
    xchg_spi(0xFF);
    /* Dummy clock (force DO hi-z for multiple slave SPI) */
}

/*-----------------------------------------------------------------------*/
/* Select the card and wait for ready                                    */
/*-----------------------------------------------------------------------*/

static
int select(void) /* 1:Successful, 0:Timeout */
{
    CS_LOW();
    xchg_spi(0xFF);
    /* Dummy clock (force DO enabled) */

    if (wait_ready(TIMEOUT_WAIT_READY)) {
        return 1; /* OK */
    }
    deselect();
    return 0; /* Timeout */
}

/*-----------------------------------------------------------------------*/
/* Receive a data packet from MMC                                        */
/*-----------------------------------------------------------------------*/

static
int rcvr_datablock(BYTE *buff, /* Data buffer to store received data */
UINT btr /* Byte count (must be multiple of 4) */
) {
    uint32_t tLR14 = getLR14();

    BYTE token;

    setTimeoutMillis(200);

    do { /* Wait for data packet in timeout of 200ms */
        token = xchg_spi(0xFF);
    } while ((token == 0xFF) && !isTimeout(tLR14));
    if (token != 0xFE) {
        return 0; /* If not valid data token, retutn with error */
    }

//	rcvr_spi_multi(buff, btr); /* Receive the data block into buffer */
#ifdef STM32_SD_USE_DMA
    stm32_dma_transfer( TRUE, buff, btr );
#else
    do { /* Receive the data block into buffer */
        *(buff++) = stm32_spi_rw(0xff);
        *(buff++) = stm32_spi_rw(0xff);
        *(buff++) = stm32_spi_rw(0xff);
        *(buff++) = stm32_spi_rw(0xff);
    } while (btr -= 4);
#endif /* STM32_SD_USE_DMA */

    xchg_spi(0xFF);
    /* Discard CRC */
    xchg_spi(0xFF);

    return 1; /* Return with success */
}

/*-----------------------------------------------------------------------*/
/* Send a data packet to MMC                                             */
/*-----------------------------------------------------------------------*/

#if	_USE_WRITE
static
int xmit_datablock(const BYTE *buff, /* 512 byte data block to be transmitted */
BYTE token /* Data/Stop token */
) {
    BYTE resp;

    if (!wait_ready(TIMEOUT_WAIT_READY)) {
        return 0;
    }

    xchg_spi(token);
    /* Xmit data token */
    if (token != 0xFD) { /* Is data token */
//		xmit_spi_multi(buff, 512); /* Xmit the data block to the MMC */
#ifdef STM32_SD_USE_DMA
        stm32_dma_transfer( FALSE, buff, 512 );
#else
        BYTE wc = 0;
        do { /* transmit the 512 byte data block to MMC */
            stm32_spi_rw(*buff++);
            stm32_spi_rw(*buff++);
        } while (--wc);
#endif /* STM32_SD_USE_DMA */
        xchg_spi(0xFF);
        /* CRC (Dummy) */
        xchg_spi(0xFF);
        resp = xchg_spi(0xFF); /* Reveive data response */
        if ((resp & 0x1F) != 0x05) { /* If not accepted, return with error */
            return 0;
        }
    }

    return 1;
}
#endif

/*-----------------------------------------------------------------------*/
/* Send a command packet to MMC                                          */
/*-----------------------------------------------------------------------*/

static BYTE send_cmd( /* Returns R1 resp (bit7==1:Send failed) */
BYTE cmd, /* Command index */
DWORD arg /* Argument */
) {
    BYTE n, res;

    if (cmd & 0x80) { /* ACMD<n> is the command sequence of CMD55-CMD<n> */
        cmd &= 0x7F;
        res = send_cmd(CMD55, 0);
        if (res > 1) {
            return res;
        }
    }

    /* Select the card and wait for ready */
    deselect();
    if (!select()) {
        return 0xFF;
    }

    /* Send command packet */
    xchg_spi(0x40 | cmd);
    /* Start + Command index */
    xchg_spi((BYTE ) (arg >> 24));
    /* Argument[31..24] */
    xchg_spi((BYTE ) (arg >> 16));
    /* Argument[23..16] */
    xchg_spi((BYTE ) (arg >> 8));
    /* Argument[15..8] */
    xchg_spi((BYTE ) arg);
    /* Argument[7..0] */
    n = 0x01; /* Dummy CRC + Stop */
    if (cmd == CMD0) {
        n = 0x95; /* Valid CRC for CMD0(0) + Stop */
    }
    if (cmd == CMD8) {
        n = 0x87; /* Valid CRC for CMD8(0x1AA) Stop */
    }
    xchg_spi(n);

    /* Receive command response */
    if (cmd == CMD12) {
        xchg_spi(0xFF);
    }
    /* Skip a stuff byte when stop reading */
    n = 10; /* Wait for a valid response in timeout of 10 attempts */
    do {
        res = xchg_spi(0xFF);
    } while ((res & 0x80) && --n);

    return res; /* Return with the response value */
}

/*--------------------------------------------------------------------------

 Public Functions

 ---------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------*/
/* Initialize Disk Drive                                                 */
/*-----------------------------------------------------------------------*/

DSTATUS disk_initialize(BYTE pdrv /* Physical drive number (0) */
) {
    uint32_t tLR14 = getLR14();
    BYTE n, cmd, ty, ocr[4];

    if (pdrv) {
        return STA_NOINIT; /* Supports only single drive */
    }
    power_off(); /* Turn off the socket power to reset the card */
    if (Stat & STA_NODISK) {
        return Stat; /* No card in the socket */
    }
    power_on(); /* Turn on the socket power */
    FCLK_SLOW();
    /* 80 dummy clocks */
    for (n = 80; n; n--) {
        xchg_spi(0xFF);
    }

    ty = 0;
    if (send_cmd(CMD0, 0) == 1) { /* Enter Idle state */
        setTimeoutMillis(1000); /* Initialization timeout of 1000 msec */
        if (send_cmd(CMD8, 0x1AA) == 1) { /* SDv2? */
            for (n = 0; n < 4; n++)
                ocr[n] = xchg_spi(0xFF); /* Get trailing return value of R7 resp */
            if (ocr[2] == 0x01 && ocr[3] == 0xAA) { /* The card can work at vdd range of 2.7-3.6V */
                while (!isTimeout(tLR14) && send_cmd(ACMD41, 1UL << 30)) {
                    /* Wait for leaving idle state (ACMD41 with HCS bit) */
                }
                if (!isTimeout(tLR14) && send_cmd(CMD58, 0) == 0) { /* Check CCS bit in the OCR */
                    for (n = 0; n < 4; n++)
                        ocr[n] = xchg_spi(0xFF);
                    ty = (ocr[0] & 0x40) ? CT_SD2 | CT_BLOCK : CT_SD2; /* SDv2 */
                }

            }
        } else { /* SDv1 or MMCv3 */
            if (send_cmd(ACMD41, 0) <= 1) {
                ty = CT_SD1;
                cmd = ACMD41; /* SDv1 */
            } else {
                ty = CT_MMC;
                cmd = CMD1; /* MMCv3 */
            }
            while (!isTimeout(tLR14) && send_cmd(cmd, 0)) {
                /* Wait for leaving idle state */
            }
            if (isTimeout(tLR14) || send_cmd(CMD16, 512) != 0) { /* Set R/W block length to 512 */
                ty = 0;
            }
        }
    }
    CardType = ty;
    deselect();

    if (ty) { /* Initialization succeeded */
        Stat &= ~STA_NOINIT; /* Clear STA_NOINIT */
        FCLK_FAST();
    } else { /* Initialization failed */
        power_off();
    }

    return Stat;
}

/*-----------------------------------------------------------------------*/
/* Get Disk Status                                                       */
/*-----------------------------------------------------------------------*/

DSTATUS disk_status(BYTE pdrv /* Physical drive nmuber (0) */
) {
    if (pdrv) {
        return STA_NOINIT; /* Supports only single drive */
    }
    return Stat;
}

/*-----------------------------------------------------------------------*/
/* Read Sector(s)                                                        */
/*-----------------------------------------------------------------------*/

DRESULT disk_read(BYTE pdrv, /* Physical drive nmuber (0) */
BYTE *buff, /* Pointer to the data buffer to store read data */
DWORD sector, /* Start sector number (LBA) */
BYTE count /* Sector count (1..255) */
) {
    if (pdrv || !count) {
        return RES_PARERR;
    }
    if (Stat & STA_NOINIT) {
        return RES_NOTRDY;
    }

    if (!(CardType & CT_BLOCK)) {
        sector *= 512; /* Convert to byte address if needed */
    }
    if (count == 1) { /* Single block read */
        if ((send_cmd(CMD17, sector) == 0) /* READ_SINGLE_BLOCK */
        && rcvr_datablock(buff, 512)) {
            count = 0;
        }
    } else { /* Multiple block read */
        if (send_cmd(CMD18, sector) == 0) { /* READ_MULTIPLE_BLOCK */
            do {
                if (!rcvr_datablock(buff, 512)) {
                    break;
                }
                buff += 512;
            } while (--count);
            send_cmd(CMD12, 0); /* STOP_TRANSMISSION */
        }
    }
    deselect();

    return count ? RES_ERROR : RES_OK;
}

/*-----------------------------------------------------------------------*/
/* Write Sector(s)                                                       */
/*-----------------------------------------------------------------------*/

#if _USE_WRITE
DRESULT disk_write(BYTE pdrv, /* Physical drive nmuber (0) */
const BYTE *buff, /* Pointer to the data to be written */
DWORD sector, /* Start sector number (LBA) */
BYTE count /* Sector count (1..255) */
) {
    if (pdrv || !count) {
        return RES_PARERR;
    }
    if (Stat & STA_NOINIT) {
        return RES_NOTRDY;
    }
    if (Stat & STA_PROTECT) {
        return RES_WRPRT;
    }
    if (!(CardType & CT_BLOCK)) {
        sector *= 512; /* Convert to byte address if needed */
    }
    if (count == 1) { /* Single block write */
        if ((send_cmd(CMD24, sector) == 0) /* WRITE_BLOCK */
        && xmit_datablock(buff, 0xFE))
            count = 0;
    } else { /* Multiple block write */
        if (CardType & CT_SDC) {
            send_cmd(ACMD23, count);
        }
        if (send_cmd(CMD25, sector) == 0) { /* WRITE_MULTIPLE_BLOCK */
            do {
                if (!xmit_datablock(buff, 0xFC))
                    break;
                buff += 512;
            } while (--count);
            if (!xmit_datablock(0, 0xFD)) { /* STOP_TRAN token */
                count = 1;
            }
        }
    }
    deselect();

    return count ? RES_ERROR : RES_OK;
}
#endif

/*-----------------------------------------------------------------------*/
/* Miscellaneous Functions                                               */
/*-----------------------------------------------------------------------*/

#if _USE_IOCTL
DRESULT disk_ioctl(BYTE pdrv, /* Physical drive nmuber (0) */
BYTE cmd, /* Control code */
void *buff /* Buffer to send/receive control data */
) {
    DRESULT res;
    BYTE n, csd[16], *ptr = buff;
    DWORD *dp, st, ed, csize;

    if (pdrv) {
        return RES_PARERR;
    }

    res = RES_ERROR;

    if (cmd == CTRL_POWER) {
        switch (ptr[0]) {
        case 0: /* Sub control code (POWER_OFF) */
            power_off(); /* Power off */
            res = RES_OK;
            break;
        case 1: /* Sub control code (POWER_GET) */
            ptr[1] = (BYTE) power_status();
            res = RES_OK;
            break;
        default:
            res = RES_PARERR;
            break;
        }
    } else {
        if (Stat & STA_NOINIT) {
            return RES_NOTRDY;
        }
        switch (cmd) {
        case CTRL_SYNC: /* Make sure that no pending write process. Do not remove this or written sector might not left updated. */
            if (select()) {
                res = RES_OK;
            }
            break;

        case GET_SECTOR_COUNT: /* Get number of sectors on the disk (DWORD) */
            if ((send_cmd(CMD9, 0) == 0) && rcvr_datablock(csd, 16)) {
                if ((csd[0] >> 6) == 1) { /* SDC ver 2.00 */
                    csize = csd[9] + ((WORD) csd[8] << 8) + ((DWORD) (csd[7] & 63) << 16) + 1;
                    *(DWORD*) buff = csize << 10;
                } else { /* SDC ver 1.XX or MMC*/
                    n = (csd[5] & 15) + ((csd[10] & 128) >> 7) + ((csd[9] & 3) << 1) + 2;
                    csize = (csd[8] >> 6) + ((WORD) csd[7] << 2) + ((WORD) (csd[6] & 3) << 10) + 1;
                    *(DWORD*) buff = csize << (n - 9);
                }
                res = RES_OK;
            }
            break;

        case GET_SECTOR_SIZE: /* Get sector size (WORD) */
            *(WORD*) buff = 512;
            res = RES_OK;
            break;

        case GET_BLOCK_SIZE: /* Get erase block size in unit of sector (DWORD) */
            if (CardType & CT_SD2) { /* SDv2? */
                if (send_cmd(ACMD13, 0) == 0) { /* Read SD status */
                    xchg_spi(0xFF);
                    if (rcvr_datablock(csd, 16)) { /* Read partial block */
                        for (n = 64 - 16; n; n--)
                            xchg_spi(0xFF);
                        /* Purge trailing data */
                        *(DWORD*) buff = 16UL << (csd[10] >> 4);
                        res = RES_OK;
                    }
                }
            } else { /* SDv1 or MMCv3 */
                if ((send_cmd(CMD9, 0) == 0) && rcvr_datablock(csd, 16)) { /* Read CSD */
                    if (CardType & CT_SD1) { /* SDv1 */
                        *(DWORD*) buff = (((csd[10] & 63) << 1) + ((WORD) (csd[11] & 128) >> 7) + 1) << ((csd[13] >> 6) - 1);
                    } else { /* MMCv3 */
                        *(DWORD*) buff = ((WORD) ((csd[10] & 124) >> 2) + 1) * (((csd[11] & 3) << 3) + ((csd[11] & 224) >> 5) + 1);
                    }
                    res = RES_OK;
                }
            }
            break;

        case CTRL_ERASE_SECTOR: /* Erase a block of sectors (used when _USE_ERASE == 1) */
            if (!(CardType & CT_SDC)) {
                break; /* Check if the card is SDC */
            }
            if (disk_ioctl(pdrv, MMC_GET_CSD, csd)) {
                break; /* Get CSD */
            }
            if (!(csd[0] >> 6) && !(csd[10] & 0x40)) {
                break; /* Check if sector erase can be applied to the card */
            }
            dp = buff;
            st = dp[0];
            ed = dp[1]; /* Load sector block */
            if (!(CardType & CT_BLOCK)) {
                st *= 512;
                ed *= 512;
            }
            if (send_cmd(CMD32, st) == 0 && send_cmd(CMD33, ed) == 0 && send_cmd(CMD38, 0) == 0
                    && wait_ready(30 * TIMEOUT_WAIT_READY)) {/* Erase sector block */
                res = RES_OK; /* FatFs does not check result of this command */
            }
            break;

            /* Following commands are never used by FatFs module */

        case MMC_GET_TYPE: /* Get card type flags (1 byte) */
            *ptr = CardType;
            res = RES_OK;
            break;

        case MMC_GET_CSD: /* Receive CSD as a data block (16 bytes) */
            if (send_cmd(CMD9, 0) == 0 /* READ_CSD */
            && rcvr_datablock(ptr, 16)) {
                res = RES_OK;
            }
            break;

        case MMC_GET_CID: /* Receive CID as a data block (16 bytes) */
            if (send_cmd(CMD10, 0) == 0 /* READ_CID */
            && rcvr_datablock(ptr, 16)) {
                res = RES_OK;
            }
            break;

        case MMC_GET_OCR: /* Receive OCR as an R3 resp (4 bytes) */
            if (send_cmd(CMD58, 0) == 0) { /* READ_OCR */
                for (n = 4; n; n--) {
                    *ptr++ = xchg_spi(0xFF);
                }
                res = RES_OK;
            }
            break;

        case MMC_GET_SDSTAT: /* Receive SD statsu as a data block (64 bytes) */
            if (send_cmd(ACMD13, 0) == 0) { /* SD_STATUS */
                xchg_spi(0xFF);
                if (rcvr_datablock(ptr, 64)) {
                    res = RES_OK;
                }
            }
            break;

        default:
            res = RES_PARERR;
            break;
        }

        deselect();
    }

    return res;
}
#endif

/*-----------------------------------------------------------------------*/
/* Device Timer Interrupt Procedure                                      */
/*-----------------------------------------------------------------------*/
/**
 * @brief  This function handles EXTI4_IRQ Handler for MMC Card detect pin.
 * @retval None
 */
void EXTI4_IRQHandler(void) {
    bool tInitialState = MICROSD_isCardInserted();
    /* Delay since it is a manual insert */
    int32_t tTimeMillisOverall = 3000; // if state changes, wait max 3 seconds
    int32_t tTimeMillis = 500; // can be rearmed
    while (tTimeMillis != 0 && tTimeMillisOverall != 0) {
        // detect reload of systic timer value
        if (hasSysticCounted()) {
            tTimeMillis--;
            tTimeMillisOverall--;
#ifdef HAL_WWDG_MODULE_ENABLED
            Watchdog_reload();
#endif
        }
        // rearm timeout if state changed within timeout period
        if (tInitialState != MICROSD_isCardInserted()) {
            tInitialState = MICROSD_isCardInserted();
            tTimeMillis = 500;
        }

    }

    if (MICROSD_isCardInserted()) {
        // card inserted
        Stat &= ~STA_NODISK;
        DSTATUS tDiskStatus = disk_initialize(0);
        if (tDiskStatus != 0x00) {
            if (tDiskStatus != 0x01) {
                // disk status 01 gives timeout message - no need to add another message here
                assertFailedParamMessage((uint8_t *) __FILE__, __LINE__, getLR14(), tDiskStatus, "Disk_initialize failed");
            }
        } else {
            FRESULT tMountStatus = f_mount(0, &Fatfs[0]);
            if (tMountStatus == FR_OK) {
                BSP_LED_On(LED_RED_2); // RED Front
            } else {
                assertFailedParamMessage((uint8_t *) __FILE__, __LINE__, getLR14(), tMountStatus, "f_mount");
            }
        }
    } else {
        // card removed
        Stat |= (STA_NODISK | STA_NOINIT);
        BSP_LED_Off(LED_RED_2); // RED Front
    }
    /* Clear the EXTI line pending bit */
    MICROSD_ClearITPendingBit();
}

void testAttachMMC(void) {
    EXTI4_IRQHandler();
}

char StringNoCardInserted[] = "No card inserted";
/**
 * returns 4 lines with card information
 * @return number of chars written or DRESULT error code 1 - 4 or 0 if no card inserted
 *
 */
int getCardInfo(char asStringBuffer[], size_t sizeofsStringBuffer) {
    BYTE res = 0;
    if (!MICROSD_isCardInserted()) {
        strcpy(asStringBuffer, StringNoCardInserted);
    } else {
        long tSectorCount;
        long tEraseBlockSize;
        BYTE tType;
        WORD tSectorSize;
        res = disk_ioctl(0, GET_SECTOR_COUNT, &tSectorCount);
        if (!res) {
            disk_ioctl(0, GET_SECTOR_SIZE, &tSectorSize);
            disk_ioctl(0, GET_BLOCK_SIZE, &tEraseBlockSize);
            disk_ioctl(0, MMC_GET_TYPE, &tType);
            return snprintf(sStringBuffer, sizeof sStringBuffer,
                    "Drive size: %lu sectors\nSector size: %u\nErase block size: %lu sectors\nMMC/SDC type: %u", tSectorCount,
                    tSectorSize, tEraseBlockSize, tType);
        }
    }
    return res; // 0- 4
}

/**
 * returns 10 lines with filesystem information
 * @return number of chars written or FRESULT error code 1 - 19 or 0 if no card inserted
 */
int getFSInfo(char *asStringBuffer, size_t sizeofsStringBuffer) {
    BYTE res = 0;
    if (!MICROSD_isCardInserted()) {
        strcpy(asStringBuffer, StringNoCardInserted);
    } else {
        FATFS *fs; /* Pointer to file system object */
        long tFreeClusterCount;
        res = f_getfree("", (DWORD*) &tFreeClusterCount, &fs);
        if (!res) {
            return snprintf(asStringBuffer, sizeofsStringBuffer, "FAT type = %u (%s)\nBytes/Cluster = %lu\nNumber of FATs = %u\n"
                    "Root DIR entries = %u\nSectors/FAT = %lu\nNumber of clusters = %lu\nNumber of free clusters = %lu\n"
                    "FAT start (lba) = %lu\nDIR start (lba,cluster) = %lu\nData start (lba) = %lu\n", (WORD) fs->fs_type,
                    (fs->fs_type == FS_FAT12) ? "FAT12" : (fs->fs_type == FS_FAT16) ? "FAT16" : "FAT32", (DWORD) fs->csize * 512,
                    (WORD) fs->n_fats, fs->n_rootdir, fs->fsize, tFreeClusterCount, (DWORD) fs->n_fatent - 2, fs->fatbase,
                    fs->dirbase, fs->database);
        }
    }
    return res; // 0- 19
}

