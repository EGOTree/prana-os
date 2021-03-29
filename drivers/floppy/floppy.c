//includes
#include <libc.h>
#include <error.h>
#include <time.h>
#include <x86/dma.h>
#include <x86/i8259.h>
#include <x86/cmos.h>
#include <x86/cpu.h>
#include <fs/vfs.h>

/*
All floppy control registers.
some of them are not usefull here, but kept just for
the sake of completeness
*/

enum FlpReg
{
    STATUS_REG_A =      0x3F0,  /* read-only */
    STATUS_REG_B =      0x3F1,  /* read-only */
    /* Digital Output Register contains the drive select and
     * motor enable bits, a reset bit and a DMA gate bit */
    DOR_REG =           0x3F2,  /* r/w */
    /* For tape support */
    TAPE_DRIVE_REG =    0x3F3,  /* right... r/w */
    MAIN_STATUS_REG =   0x3F4,  /* read-only */
    DATARATE_SELECT_REG=0x3F4,  /* write-only */
    DATA_REG =          0x3F5,  /* r/w. (FIFO) */
    DIGITAL_INPUT_REG = 0x3F7,  /* read-only */
    CTRL_REG =          0x3F7   /* write-only */
};

enum data_cmd
{
    CMD_READ_TRACK =        0x02,  /* generates IRQ6 */
    CMD_SET_PARAM =         0x03,  /* set drive parameters */
    CMD_DRIVE_STATUS =      0x04,
    CMD_WRITE_DATA =        0x05,  /* write data to disk */
    CMD_READ_DATA =         0x06,  /* read data from disk */
    CMD_RECALIBRATE =       0x07,  /* seek to cylinder 0 */
    CMD_SENSE_INTERRUPT =   0x08,  /* ack IRQ6, get status of last cmd */
    CMD_WRITE_DELETED_DATA= 0x09,
    CMD_READ_ID =           0x0A,  /* generatess IRQ6 */
    CMD_READ_DELETED_DATA = 0x0C,
    CMD_FORMAT_TRACK =      0x0D,
    CMD_SEEK =              0x0F,  /* seek both heads to cylinder X */
    CMD_VERSION =           0x10,  /* used on init */
    CMD_SCAN_EQUAL =        0x11,
    CMD_PERPENDICULAR_MODE= 0x12,  /* used on init */
    CMD_CONFIGURE =         0x13,  /* set controller parameters */
    CMD_LOCK =              0x14,  /* protect controller parameters from reset */
    CMD_VERIFY =            0x16,
    CMD_SCAN_LOW_OR_EQUAL = 0x19,
    CMD_SCAN_HIGH_OR_EQUAL= 0x1D,

    /* When read FIFO register, this value indicates that
     * an invalid command was given on the previous write */
    CMD_ERROR = 0x80
};

enum data_read_mode
{
    READ_MODE_SKIP_DELETED_DATA = 0x20,
    READ_MODE_DOUBLE_DENSITY = 0x40,
    READ_MODE_MULTITRACK = 0x80
};

/*
Commands for writting to DOR_REG.
*/

enum dor_cmd
{
    /* Device selection */
    DOR_SEL_0 =     0x00,   /* 00000000 */
    DOR_SEL_1 =     0x01,   /* 00000001 */
    DOR_SEL_2 =     0x02,   /* 00000010 */
    DOR_SEL_3 =     0x03,   /* 00000011 */
    /* Clears the core circuits of 82077AA */
    DOR_RESET =     0x04,   /* 00000100 */
    /* Set floppy to DMA mode */
    DOR_DMA_GATE =  0x08,   /* 00001000 */
    /* Another device selection data.
     * It has to match with DOR_SEL_X */
    DOR_MOTOR_0 =   0x10,   /* 00010000 */
    DOR_MOTOR_1 =   0x20,   /* 00100000 */
    DOR_MOTOR_2 =   0x40,   /* 01000000 */
    DOR_MOTOR_3 =   0x80    /* 10000000 */
};

enum motor_delay
{
    WAIT_MOTOR_SPIN,
    NO_WAIT_MOTOR_SPIN
};

enum dsr_cmd
{
    /* Data tranfer rates */
    DSR_RATE_250KBPS = 0x02,
    DSR_RATE_300KBPS = 0x01,
    DSR_RATE_500KBPS = 0x00,
    DSR_RATE_1MBPS = 0x03,
    /* Shuts the chip off. One of the *_RESET
     * functionality will turn it ON again */
    DSR_PWR_DOWN =  0x40,
    /* This reset is the same as DOR reset except that this
     * reset is self clearing. */
    DSR_RESET =     0x80
};

enum msr_cmd
{
    /* Is set when drive is in seek or recalibrate states. */
    MSR_BUSY_0 = 0x01,
    MSR_BUSY_1 = 0x02,
    MSR_BUSY_2 = 0x04,
    MSE_BUSY_3 = 0x08,
    /* If set, the CMD execution is in progress */
    MSR_BUSY = 0x10,
    /* If MSR_CAN_TRANSFER is set, indicates the required
     * data transfer direction - 1 for read, 0 for write. */
    MSR_DIR = 0x40,
    /* Indicates that host can tranfser data.
     * If not set - access is not permited. */
    MSR_CAN_TRANSFER = 0x80
};

#define SECTOR_SIZE 512
#define TOTAL_SECTORS 2880
#define SECTORS_PER_TRACK 18
#define HEAD_COUNT 2
#define TRACK_COUNT 80
#define GAP_SIZE 0x1B

#define FLOPPY_DMA 2
#define FLOPPY_IRQ 6

#define CAN_TRANSFER_RETRIES 1000
#define RECALIBRATE_RETRIES 80

struct chs_t {
    unsigned int head;
    unsigned int cylinder;
    unsigned int sector;
};

struct floppy_t {
    /* This is the variable flp_wait_irq() is spinning on.
     * When a cmd is given to the controller, it will IRQ6
     * on complete, making the variable ON and the variable */
    int irq_received;
    struct dma_t dma;
    unsigned int drive_nr;
    /* Drive specific DOR and MSR registers */
    enum dor_cmd dor_select_reg;
    enum dor_cmd dor_motor_reg;
    enum msr_cmd msr_busy_bit;
    unsigned char cur_dor;
};

struct floppy_t flp = {
    .irq_received = 0,
    .drive_nr = 0
};

static void flp_wait_irq()
{
    while (!flp.irq_received)
        ;
    flp.irq_received = 0;
}

static void ctrl_disable()
{
    flp.cur_dor = 0x00;
    outportb(DOR_REG, flp.cur_dor);
}

// Enables floppy controller
static void ctrl_enable()
{
    flp.cur_dor = flp.dor_select_reg | DOR_RESET | DOR_DMA_GATE;
    flp.irq_received = 0;
    outportb(DOR_REG, flp.cur_dor);
    flp_wait_irq();
}

//Note: on real hardware this requires delay for the motor
static void set_motor_on(enum motor_delay delay)
{
    flp.cur_dor |= flp.dor_motor_reg;
    outportb(DOR_REG, flp.cur_dor);

    if (delay == WAIT_MOTOR_SPIN)
        msdelay(300);
}

//turns offs motor
static void set_motor_off(enum motor_delay delay)
{
    flp.cur_dor &= ~flp.dor_motor_reg;
    outportb(DOR_REG, flp.cur_dor);

    if (delay == WAIT_MOTOR_SPIN)
        msdelay(2000);
}

//sets the speed for data transfer
static void set_tranfer_rate(enum dsr_cmd drate)
{
    outportb(DATARATE_SELECT_REG, drate);
}

//returns msr register
static unsigned char get_flp_status()
{
    return inportb(MAIN_STATUS_REG);
}

//perfrom our cmd
static int cmd_tranfser()
{
    return get_flp_status() & MSR_CAN_TRANSFER;
}

//returns true of the controller expects read command
static int cmd_should_read()
{
    return get_flp_status() & MSR_DIR;
}

//returns true of the controller expects write command
static int cmd_should_write()
{
    return (get_flp_status() & MSR_DIR) == 0;
}

//writes data to FIFO register
static void flp_send_cmd(unsigned char cmd)
{
    int i;

    if (cmd_should_read())
        kernel_warning("floppy should read while write is requested");
    
    for (i = 0; i < CAN_TRANSFER_RETRIES; i++)
        if(can_transfer())
        {
            outportb(DATA_REG, cmd & 0xFF);
            break;
        }
        else if (i == CAN_TRANSFER_RETRIES)
            kernel_warning("floppy write cmd reached maximum retries");
}

