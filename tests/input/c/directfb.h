typedef unsigned int size_t;
__extension__ typedef signed long long int __int64_t;
__extension__ typedef struct { int __val[2]; } __fsid_t;
typedef __off64_t __loff_t;
typedef __quad_t *__qaddr_t;
__extension__ typedef int __intptr_t;
typedef __u_char u_char;
typedef int int8_t __attribute__ ((__mode__ (__QI__)));
typedef int __sig_atomic_t;
typedef struct
  {
    unsigned long int __val[(1024 / (8 * sizeof (unsigned long int)))];
  } __sigset_t;
typedef __sigset_t sigset_t;
struct timespec
  {
    __time_t tv_sec;
    long int tv_nsec;
  };
typedef struct
  {
    __fd_mask __fds_bits[1024 / (8 * sizeof (__fd_mask))];
  } fd_set;
typedef __fd_mask fd_mask;
extern int select (int __nfds, fd_set *__restrict __readfds,
     fd_set *__restrict __writefds,
     fd_set *__restrict __exceptfds,
     struct timeval *__restrict __timeout);
__extension__
extern __inline unsigned int gnu_dev_major (unsigned long long int __dev)
     __attribute__ ((__nothrow__));
__extension__ extern __inline unsigned long long int
__attribute__ ((__nothrow__)) gnu_dev_makedev (unsigned int __major, unsigned int __minor)
{
  return ((__minor & 0xff) | ((__major & 0xfff) << 8)
   | (((unsigned long long int) (__minor & ~0xff)) << 12)
   | (((unsigned long long int) (__major & ~0xfff)) << 32));
}
typedef struct __pthread_attr_s
{
  int __detachstate;
  int __schedpolicy;
  struct __sched_param __schedparam;
  int __inheritsched;
  int __scope;
  size_t __guardsize;
  int __stackaddr_set;
  void *__stackaddr;
  size_t __stacksize;
} pthread_attr_t;

enum __itimer_which
  {
    ITIMER_REAL = 0,
    ITIMER_VIRTUAL = 1,
    ITIMER_PROF = 2
  };

typedef enum {
     DIKT_UNICODE = 0x0000,
     DIKT_SPECIAL = 0xF000,
     DIKT_FUNCTION = 0xF100,
     DIKT_MODIFIER = 0xF200,
     DIKT_LOCK = 0xF300,
     DIKT_DEAD = 0xF400,
     DIKT_CUSTOM = 0xF500,
     DIKT_IDENTIFIER = 0xF600
} DFBInputDeviceKeyType;

typedef enum {
     DIKS_NULL = ((DIKT_UNICODE) | (0x00)),
     DIKS_BACKSPACE = ((DIKT_UNICODE) | (0x08)),
     DIKS_TAB = ((DIKT_UNICODE) | (0x09)),
     DIKS_SHIFT = (((DIKT_MODIFIER) | ((1 << DIMKI_SHIFT)))),
     DIKS_CUSTOM0 = (((DIKT_CUSTOM) | (0))),
} DFBInputDeviceKeySymbol;

typedef struct {
     int code;
     DFBInputDeviceLockState locks;
     DFBInputDeviceKeyIdentifier identifier;
     DFBInputDeviceKeySymbol symbols[DIKSI_LAST+1];
} DFBInputDeviceKeymapEntry;

const char * DirectFBCheckVersion( unsigned int required_major,
                                   unsigned int required_minor,
                                   unsigned int required_micro );

DFBResult DirectFBError(
                             const char *msg,
                             DFBResult result
                       );
const char *DirectFBErrorString(
                         DFBResult result
                      );
DFBResult DirectFBInit(
                         int *argc,
                         char **argv[]
                      );
DFBResult DirectFBCreate(
                          IDirectFB **interface
                        );
typedef enum {
     DSPF_UNKNOWN = 0x00000000,
     DSPF_ARGB1555 = ( (((0 ) & 0x7F) ) | (((15) & 0x1F) << 7) | (((1) & 0x0F) << 12) | (((1 ) ? 1 :0) << 16) | (((0 ) & 0x07) << 17) | (((2 ) & 0x07) << 20) | (((0 ) & 0x07) << 23) | (((0 ) & 0x03) << 26) | (((0 ) & 0x03) << 28) | (((0 ) ? 1 :0) << 30) | (((0 ) ? 1 :0) << 31) ),
} DFBSurfacePixelFormat;
struct _IDirectFB { void *priv; int magic; DFBResult (*AddRef)( IDirectFB *thiz ); DFBResult (*Release)( IDirectFB *thiz ); DFBResult (*SetCooperativeLevel) ( IDirectFB *thiz, DFBCooperativeLevel level ); DFBResult (*SetVideoMode) ( IDirectFB *thiz, int width, int height, int bpp ); DFBResult (*GetCardCapabilities) ( IDirectFB *thiz, DFBCardCapabilities *ret_caps ); DFBResult (*EnumVideoModes) ( IDirectFB *thiz, DFBVideoModeCallback callback, void *callbackdata ); DFBResult (*CreateSurface) ( IDirectFB *thiz, const DFBSurfaceDescription *desc, IDirectFBSurface **ret_interface ); DFBResult (*CreatePalette) ( IDirectFB *thiz, const DFBPaletteDescription *desc, IDirectFBPalette **ret_interface ); DFBResult (*EnumScreens) ( IDirectFB *thiz, DFBScreenCallback callback, void *callbackdata ); DFBResult (*GetScreen) ( IDirectFB *thiz, DFBScreenID screen_id, IDirectFBScreen **ret_interface ); DFBResult (*EnumDisplayLayers) ( IDirectFB *thiz, DFBDisplayLayerCallback callback, void *callbackdata ); DFBResult (*GetDisplayLayer) ( IDirectFB *thiz, DFBDisplayLayerID layer_id, IDirectFBDisplayLayer **ret_interface ); DFBResult (*EnumInputDevices) ( IDirectFB *thiz, DFBInputDeviceCallback callback, void *callbackdata ); DFBResult (*GetInputDevice) ( IDirectFB *thiz, DFBInputDeviceID device_id, IDirectFBInputDevice **ret_interface ); DFBResult (*CreateEventBuffer) ( IDirectFB *thiz, IDirectFBEventBuffer **ret_buffer ); DFBResult (*CreateInputEventBuffer) ( IDirectFB *thiz, DFBInputDeviceCapabilities caps, DFBBoolean global, IDirectFBEventBuffer **ret_buffer ); DFBResult (*CreateImageProvider) ( IDirectFB *thiz, const char *filename, IDirectFBImageProvider **ret_interface ); DFBResult (*CreateVideoProvider) ( IDirectFB *thiz, const char *filename, IDirectFBVideoProvider **ret_interface ); DFBResult (*CreateFont) ( IDirectFB *thiz, const char *filename, const DFBFontDescription *desc, IDirectFBFont **ret_interface ); DFBResult (*CreateDataBuffer) ( IDirectFB *thiz, const DFBDataBufferDescription *desc, IDirectFBDataBuffer **ret_interface ); DFBResult (*SetClipboardData) ( IDirectFB *thiz, const char *mime_type, const void *data, unsigned int size, struct timeval *ret_timestamp ); DFBResult (*GetClipboardData) ( IDirectFB *thiz, char **ret_mimetype, void **ret_data, unsigned int *ret_size ); DFBResult (*GetClipboardTimeStamp) ( IDirectFB *thiz, struct timeval *ret_timestamp ); DFBResult (*Suspend) ( IDirectFB *thiz ); DFBResult (*Resume) ( IDirectFB *thiz ); DFBResult (*WaitIdle) ( IDirectFB *thiz ); DFBResult (*WaitForSync) ( IDirectFB *thiz ); DFBResult (*GetInterface) ( IDirectFB *thiz, const char *type, const char *implementation, void *arg, void **ret_interface ); };
