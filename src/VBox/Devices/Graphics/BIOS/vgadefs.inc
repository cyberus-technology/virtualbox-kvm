VGAREG_ACTL_ADDRESS             EQU     3C0h
VGAREG_ACTL_WRITE_DATA          EQU     3C0h
VGAREG_ACTL_READ_DATA           EQU     3C1h
VGAREG_ACTL_RESET               EQU     3DAh

VGAREG_DAC_READ_ADDRESS         EQU     3C7h
VGAREG_DAC_WRITE_ADDRESS        EQU     3C8h
VGAREG_DAC_DATA                 EQU     3C9h

VGAREG_PEL_MASK                 EQU     3C6h

VGAREG_SEQU_ADDRESS             EQU     3C4h
VGAREG_SEQU_DATA                EQU     3C5h

VGAREG_GRDC_ADDRESS             EQU     3CEh
VGAREG_GRDC_DATA                EQU     3CFh

VGAREG_MDA_CRTC_ADDRESS         EQU     3B4h
VGAREG_VGA_CRTC_ADDRESS         EQU     3D4h
VGAREG_VGA_CRTC_DATA            EQU     3D5h


VGAREG_READ_MISC_OUTPUT         EQU     3CCh
VGAREG_WRITE_MISC_OUTPUT        EQU     3C2h

BIOSMEM_SEG                     EQU     40h
BIOSMEM_CURRENT_MODE            EQU     49h
BIOSMEM_NB_COLS                 EQU     4Ah
BIOSMEM_CURRENT_PAGE            EQU     62h
BIOSMEM_CRTC_ADDRESS            EQU     63h
BIOSMEM_VIDEO_CTL               EQU     87h
BIOSMEM_SWITCHES                EQU     88h
BIOSMEM_MODESET_CTL             EQU     89h
BIOSMEM_DCC_INDEX               EQU     8Ah

BIOSMEM_VBE_FLAG                EQU     0B9h
BIOSMEM_VBE_MODE                EQU     0BAh

VBE_DISPI_IOPORT_INDEX          EQU     1CEh
VBE_DISPI_IOPORT_DATA           EQU     1CFh

VBE_DISPI_INDEX_ID              EQU     00h
VBE_DISPI_INDEX_XRES            EQU     01h
VBE_DISPI_INDEX_YRES            EQU     02h
VBE_DISPI_INDEX_BPP             EQU     03h
VBE_DISPI_INDEX_ENABLE          EQU     04h
VBE_DISPI_INDEX_BANK            EQU     05h
VBE_DISPI_INDEX_VIRT_WIDTH      EQU     06h
VBE_DISPI_INDEX_VIRT_HEIGHT     EQU     07h
VBE_DISPI_INDEX_X_OFFSET        EQU     08h
VBE_DISPI_INDEX_Y_OFFSET        EQU     09h

VBE_DISPI_ENABLED               EQU     01h
VBE_DISPI_GETCAPS               EQU     02h
VBE_DISPI_8BIT_DAC              EQU     20h

VBE_DISPI_ID0                   EQU     0B0C0h
VBE_DISPI_ID4                   EQU     0B0C4h
