#ifndef CHAROS_UEFI_H
#define CHAROS_UEFI_H

/* 26A: Minimal UEFI tanımları (gnu-efi yok; freestanding).
 * Yalnızca kullandığımız protokoller, alan SIRASI UEFI spec ile aynı olmalı. */

#include <stdint.h>

typedef uint64_t UINT64;
typedef uint32_t UINT32;
typedef uint16_t UINT16;
typedef uint16_t CHAR16;
typedef uint8_t  UINT8;
typedef UINT64   UINTN;
typedef int64_t  INT64;
typedef INT64    INTN;
typedef UINTN    EFI_STATUS;
typedef void*    EFI_HANDLE;
typedef void*    EFI_EVENT;

#define EFI_SUCCESS 0
#define EFI_ERROR_MASK ((UINTN)0x8000000000000000ULL)
#define EFI_ERROR(x) (((INTN)(x)) < 0)

typedef struct {
    UINT32 Data1;
    UINT16 Data2;
    UINT16 Data3;
    UINT8  Data4[8];
} EFI_GUID;

static const EFI_GUID gEfiLoadedImageProtocolGuid =
    {0x5B1B31A1,0x9562,0x11d2,{0x8E,0x3F,0x00,0xA0,0xC9,0x69,0x72,0x3B}};
static const EFI_GUID gEfiSimpleFileSystemProtocolGuid =
    {0x964E5B22,0x6459,0x11d2,{0x8E,0x39,0x00,0xA0,0xC9,0x69,0x72,0x3B}};
static const EFI_GUID gEfiGraphicsOutputProtocolGuid =
    {0x9042A9DE,0x23DC,0x4A38,{0x96,0xFB,0x7A,0xDE,0xD0,0x80,0x51,0x6A}};
static const EFI_GUID gEfiFileInfoGuid =
    {0x09576E92,0x6D3F,0x11d2,{0x8E,0x39,0x00,0xA0,0xC9,0x69,0x72,0x3B}};

typedef struct {
    UINT64 Signature;
    UINT32 Revision;
    UINT32 HeaderSize;
    UINT32 CRC32;
    UINT32 Reserved;
} EFI_TABLE_HEADER;

struct _EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL;
typedef EFI_STATUS (*EFI_TEXT_STRING)(
    struct _EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *This, CHAR16 *String);
typedef EFI_STATUS (*EFI_TEXT_RESET)(
    struct _EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *This, UINT8 ExtendedVerification);

typedef struct _EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL {
    EFI_TEXT_RESET  Reset;
    EFI_TEXT_STRING OutputString;
    void *TestString;
    void *QueryMode;
    void *SetMode;
    void *SetAttribute;
    void *ClearScreen;
    void *SetCursorPosition;
    void *EnableCursor;
    void *Mode;
} EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL;

typedef struct {
    UINT32 Revision;
    EFI_HANDLE ParentHandle;
    void *SystemTable;
    EFI_HANDLE DeviceHandle;
    void *FilePath;
    void *Reserved;
    UINT32 LoadOptionsSize;
    void *LoadOptions;
    void *ImageBase;
    UINT64 ImageSize;
    UINTN ImageCodeType;
    UINTN ImageDataType;
    void *Unload;
} EFI_LOADED_IMAGE_PROTOCOL;

typedef struct {
    UINT64 Signature;
    void *Revision;
    UINT64 NumberOfPages;
    UINT64 Attribute;
} EFI_MEMORY_DESCRIPTOR_MIN;
/* Gerçek descriptor 40B: Type(u32)+Pad(u32)+Phys(u64)+Virt(u64)+Pages(u64)+Attr(u64) */
typedef struct {
    UINT32 Type;
    UINT32 Pad;
    UINT64 PhysicalStart;
    UINT64 VirtualStart;
    UINT64 NumberOfPages;
    UINT64 Attribute;
} EFI_MEMORY_DESCRIPTOR;

#define EfiLoaderData        2
#define EfiBootServicesData  4
#define EfiConventionalMemory 7
#define EfiACPIReclaimMemory 9

typedef enum {
    AllocateAnyPages = 0,
    AllocateMaxAddress = 1,
    AllocateAddress = 2
} EFI_ALLOCATE_TYPE;

struct _EFI_BOOT_SERVICES;
typedef EFI_STATUS (*EFI_ALLOCATE_PAGES)(int Type, int MemoryType,
    UINTN Pages, UINT64 *Memory);
typedef EFI_STATUS (*EFI_GET_MEMORY_MAP)(UINTN *MemoryMapSize,
    EFI_MEMORY_DESCRIPTOR *MemoryMap, UINTN *MapKey,
    UINTN *DescriptorSize, UINT32 *DescriptorVersion);
typedef EFI_STATUS (*EFI_ALLOCATE_POOL)(int PoolType, UINTN Size, void **Buffer);
typedef EFI_STATUS (*EFI_HANDLE_PROTOCOL)(EFI_HANDLE Handle,
    EFI_GUID *Protocol, void **Interface);
typedef EFI_STATUS (*EFI_EXIT_BOOT_SERVICES)(EFI_HANDLE ImageHandle, UINTN MapKey);
typedef EFI_STATUS (*EFI_LOCATE_PROTOCOL)(EFI_GUID *Protocol,
    void *Registration, void **Interface);
typedef EFI_STATUS (*EFI_FREE_POOL)(void *Buffer);

typedef struct _EFI_BOOT_SERVICES {
    EFI_TABLE_HEADER Hdr;
    void *RaiseTPL;
    void *RestoreTPL;
    EFI_ALLOCATE_PAGES AllocatePages;       /* 3 */
    void *FreePages;                        /* 4 */
    EFI_GET_MEMORY_MAP GetMemoryMap;        /* 5 */
    EFI_ALLOCATE_POOL AllocatePool;         /* 6 */
    EFI_FREE_POOL FreePool;                 /* 7 */
    void *CreateEvent;                      /* 8 */
    void *SetTimer;                         /* 9 */
    void *WaitForEvent;                     /* 10 */
    void *SignalEvent;                      /* 11 */
    void *CloseEvent;                       /* 12 */
    void *CheckEvent;                       /* 13 */
    void *InstallProtocolInterface;         /* 14 */
    void *ReinstallProtocolInterface;       /* 15 */
    void *UninstallProtocolInterface;       /* 16 */
    EFI_HANDLE_PROTOCOL HandleProtocol;     /* 17 */
    void *Reserved;                         /* 18 */
    void *RegisterProtocolNotify;           /* 19 */
    void *LocateHandle;                     /* 20 */
    void *LocateDevicePath;                 /* 21 */
    void *InstallConfigurationTable;        /* 22 */
    void *LoadImage;                        /* 23 */
    void *StartImage;                       /* 24 */
    void *Exit;                             /* 25 */
    void *UnloadImage;                      /* 26 */
    EFI_EXIT_BOOT_SERVICES ExitBootServices;/* 27 */
    void *GetNextMonotonicCount;            /* 28 */
    void *Stall;                            /* 29 */
    void *SetWatchdogTimer;                 /* 30 */
    void *ConnectController;                /* 31 */
    void *DisconnectController;             /* 32 */
    void *OpenProtocol;                     /* 33 */
    void *CloseProtocol;                    /* 34 */
    void *OpenProtocolInformation;          /* 35 */
    void *ProtocolsPerHandle;               /* 36 */
    void *LocateHandleBuffer;               /* 37 */
    EFI_LOCATE_PROTOCOL LocateProtocol;     /* 38 */
} EFI_BOOT_SERVICES;

typedef struct {
    EFI_TABLE_HEADER Hdr;
    void *FirmwareVendor;
    UINT32 FirmwareRevision;
    EFI_HANDLE ConsoleInHandle;
    void *ConIn;
    EFI_HANDLE ConsoleOutHandle;
    EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *ConOut;
    EFI_HANDLE StandardErrorHandle;
    void *StdErr;
    void *RuntimeServices;
    EFI_BOOT_SERVICES *BootServices;
    UINTN NumberOfTableEntries;
    void *ConfigurationTable; /* EFI_CONFIGURATION_TABLE[] */
} EFI_SYSTEM_TABLE;

typedef struct {
    EFI_GUID VendorGuid;
    void *VendorTable;
} EFI_CONFIGURATION_TABLE;

static const EFI_GUID gEfiAcpi20TableGuid =
    {0x8868E871,0xE4F1,0x11D3,{0xBC,0x22,0x00,0x80,0xC7,0x3C,0x88,0x81}};
static const EFI_GUID gEfiAcpi10TableGuid =
    {0xEB9D2D30,0x2D88,0x11D3,{0x9A,0x16,0x00,0x90,0x27,0x3F,0xC1,0x4D}};

/* ---- GOP ---- */
typedef struct {
    UINT32 Version;
    UINT32 HorizontalResolution;
    UINT32 VerticalResolution;
    UINT32 PixelFormat; /* 0 RGBR, 1 BGRR, 2 BitMask, 3 BltOnly */
    UINT32 PixelInformation[4]; /* EFI_PIXEL_BITMASK: 16B (direkt modda yoksay) */
    UINT32 PixelsPerScanLine;
} EFI_GRAPHICS_OUTPUT_MODE_INFORMATION;

typedef struct {
    UINT32 MaxMode;
    UINT32 Mode;
    EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *Info;
    UINTN SizeOfInfo;
    UINT64 FrameBufferBase;
    UINTN FrameBufferSize;
} EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE;

struct _EFI_GRAPHICS_OUTPUT_PROTOCOL;
typedef EFI_STATUS (*EFI_GOP_QUERY_MODE)(
    struct _EFI_GRAPHICS_OUTPUT_PROTOCOL *This, UINT32 ModeNumber,
    UINTN *SizeOfInfo, EFI_GRAPHICS_OUTPUT_MODE_INFORMATION **Info);
typedef EFI_STATUS (*EFI_GOP_SET_MODE)(
    struct _EFI_GRAPHICS_OUTPUT_PROTOCOL *This, UINT32 ModeNumber);

typedef struct _EFI_GRAPHICS_OUTPUT_PROTOCOL {
    EFI_GOP_QUERY_MODE QueryMode;
    EFI_GOP_SET_MODE SetMode;
    void *Blt;
    EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE *Mode;
} EFI_GRAPHICS_OUTPUT_PROTOCOL;

/* ---- File ---- */
struct _EFI_FILE_PROTOCOL;
typedef EFI_STATUS (*EFI_FILE_OPEN)(struct _EFI_FILE_PROTOCOL *This,
    struct _EFI_FILE_PROTOCOL **NewHandle, CHAR16 *FileName,
    UINT64 OpenMode, UINT64 Attributes);
typedef EFI_STATUS (*EFI_FILE_CLOSE)(struct _EFI_FILE_PROTOCOL *This);
typedef EFI_STATUS (*EFI_FILE_READ)(struct _EFI_FILE_PROTOCOL *This,
    UINTN *BufferSize, void *Buffer);
typedef EFI_STATUS (*EFI_FILE_GETINFO)(struct _EFI_FILE_PROTOCOL *This,
    EFI_GUID *InformationType, UINTN *BufferSize, void *Buffer);
typedef EFI_STATUS (*EFI_FILE_GETPOS)(struct _EFI_FILE_PROTOCOL *This, UINT64 *Position);
typedef EFI_STATUS (*EFI_FILE_SETPOS)(struct _EFI_FILE_PROTOCOL *This, UINT64 Position);

typedef struct _EFI_FILE_PROTOCOL {
    UINT64 Revision;
    EFI_FILE_OPEN Open;
    EFI_FILE_CLOSE Close;
    void *Delete;
    EFI_FILE_READ Read;
    void *Write;
    EFI_FILE_GETPOS GetPosition;
    EFI_FILE_SETPOS SetPosition;
    EFI_FILE_GETINFO GetInfo;
    void *SetInfo;
    void *Flush;
} EFI_FILE_PROTOCOL;

typedef EFI_STATUS (*EFI_SFS_OPEN_VOLUME)(void *This, EFI_FILE_PROTOCOL **Root);

typedef struct {
    UINT64 Revision;
    EFI_SFS_OPEN_VOLUME OpenVolume;
} EFI_SIMPLE_FILE_SYSTEM_PROTOCOL;

typedef struct {
    UINT64 Size;
    UINT64 FileSize;
    UINT64 PhysicalSize;
} EFI_FILE_INFO_MIN;
/* FileSize offset 8, ihtiyacımız olan bu. */

#endif
