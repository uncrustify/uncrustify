typedef unsigned int size_t;
typedef unsigned char __u_char;
typedef unsigned short int __u_short;
typedef unsigned int __u_int;
typedef unsigned long int __u_long;
typedef signed char __int8_t;
typedef unsigned char __uint8_t;
typedef signed short int __int16_t;
typedef unsigned short int __uint16_t;
typedef signed int __int32_t;
typedef unsigned int __uint32_t;
__extension__ typedef signed long long int __int64_t;
__extension__ typedef unsigned long long int __uint64_t;
__extension__ typedef long long int __quad_t;
__extension__ typedef unsigned long long int __u_quad_t;
__extension__ typedef __u_quad_t __dev_t;
__extension__ typedef unsigned int __uid_t;
__extension__ typedef unsigned int __gid_t;
__extension__ typedef unsigned long int __ino_t;
__extension__ typedef __u_quad_t __ino64_t;
__extension__ typedef unsigned int __mode_t;
__extension__ typedef unsigned int __nlink_t;
__extension__ typedef long int __off_t;
__extension__ typedef __quad_t __off64_t;
__extension__ typedef int __pid_t;
__extension__ typedef struct { int __val[2]; } __fsid_t;
__extension__ typedef long int __clock_t;
__extension__ typedef unsigned long int __rlim_t;
__extension__ typedef __u_quad_t __rlim64_t;
__extension__ typedef unsigned int __id_t;
__extension__ typedef long int __time_t;
__extension__ typedef unsigned int __useconds_t;
__extension__ typedef long int __suseconds_t;
__extension__ typedef int __daddr_t;
__extension__ typedef long int __swblk_t;
__extension__ typedef int __key_t;
__extension__ typedef int __clockid_t;
__extension__ typedef int __timer_t;
__extension__ typedef long int __blksize_t;
__extension__ typedef long int __blkcnt_t;
__extension__ typedef __quad_t __blkcnt64_t;
__extension__ typedef unsigned long int __fsblkcnt_t;
__extension__ typedef __u_quad_t __fsblkcnt64_t;
__extension__ typedef unsigned long int __fsfilcnt_t;
__extension__ typedef __u_quad_t __fsfilcnt64_t;
__extension__ typedef int __ssize_t;
typedef __off64_t __loff_t;
typedef __quad_t *__qaddr_t;
typedef char *__caddr_t;
__extension__ typedef int __intptr_t;
__extension__ typedef unsigned int __socklen_t;
typedef __u_char u_char;
typedef __u_short u_short;
typedef __u_int u_int;
typedef __u_long u_long;
typedef __quad_t quad_t;
typedef __u_quad_t u_quad_t;
typedef __fsid_t fsid_t;
typedef __loff_t loff_t;
typedef __ino_t ino_t;
typedef __dev_t dev_t;
typedef __gid_t gid_t;
typedef __mode_t mode_t;
typedef __nlink_t nlink_t;
typedef __uid_t uid_t;
typedef __off_t off_t;
typedef __pid_t pid_t;
typedef __id_t id_t;
typedef __ssize_t ssize_t;
typedef __daddr_t daddr_t;
typedef __caddr_t caddr_t;
typedef __key_t key_t;
typedef __time_t time_t;
typedef __clockid_t clockid_t;
typedef __timer_t timer_t;
typedef unsigned long int ulong;
typedef unsigned short int ushort;
typedef unsigned int uint;
typedef int int8_t __attribute__ ((__mode__ (__QI__)));
typedef int int16_t __attribute__ ((__mode__ (__HI__)));
typedef int int32_t __attribute__ ((__mode__ (__SI__)));
typedef int int64_t __attribute__ ((__mode__ (__DI__)));
typedef unsigned int u_int8_t __attribute__ ((__mode__ (__QI__)));
typedef unsigned int u_int16_t __attribute__ ((__mode__ (__HI__)));
typedef unsigned int u_int32_t __attribute__ ((__mode__ (__SI__)));
typedef unsigned int u_int64_t __attribute__ ((__mode__ (__DI__)));
typedef int register_t __attribute__ ((__mode__ (__word__)));
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
struct timeval
  {
    __time_t tv_sec;
    __suseconds_t tv_usec;
  };
typedef __suseconds_t suseconds_t;
typedef long int __fd_mask;
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
__extension__
extern __inline unsigned int gnu_dev_minor (unsigned long long int __dev)
     __attribute__ ((__nothrow__));
__extension__
extern __inline unsigned long long int gnu_dev_makedev (unsigned int __major,
       unsigned int __minor)
     __attribute__ ((__nothrow__));
__extension__ extern __inline unsigned int
__attribute__ ((__nothrow__)) gnu_dev_major (unsigned long long int __dev)
{
  return ((__dev >> 8) & 0xfff) | ((unsigned int) (__dev >> 32) & ~0xfff);
}
__extension__ extern __inline unsigned int
__attribute__ ((__nothrow__)) gnu_dev_minor (unsigned long long int __dev)
{
  return (__dev & 0xff) | ((unsigned int) (__dev >> 12) & ~0xff);
}
__extension__ extern __inline unsigned long long int
__attribute__ ((__nothrow__)) gnu_dev_makedev (unsigned int __major, unsigned int __minor)
{
  return ((__minor & 0xff) | ((__major & 0xfff) << 8)
   | (((unsigned long long int) (__minor & ~0xff)) << 12)
   | (((unsigned long long int) (__major & ~0xfff)) << 32));
}
typedef __blkcnt_t blkcnt_t;
typedef __fsblkcnt_t fsblkcnt_t;
typedef __fsfilcnt_t fsfilcnt_t;
struct __sched_param
  {
    int __sched_priority;
  };
typedef int __atomic_lock_t;
struct _pthread_fastlock
{
  long int __status;
  __atomic_lock_t __spinlock;
};
typedef struct _pthread_descr_struct *_pthread_descr;
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
__extension__ typedef long long __pthread_cond_align_t;
typedef struct
{
  struct _pthread_fastlock __c_lock;
  _pthread_descr __c_waiting;
  char __padding[48 - sizeof (struct _pthread_fastlock)
   - sizeof (_pthread_descr) - sizeof (__pthread_cond_align_t)];
  __pthread_cond_align_t __align;
} pthread_cond_t;
typedef struct
{
  int __dummy;
} pthread_condattr_t;
typedef unsigned int pthread_key_t;
typedef struct
{
  int __m_reserved;
  int __m_count;
  _pthread_descr __m_owner;
  int __m_kind;
  struct _pthread_fastlock __m_lock;
} pthread_mutex_t;
typedef struct
{
  int __mutexkind;
} pthread_mutexattr_t;
typedef int pthread_once_t;
typedef unsigned long int pthread_t;
typedef struct {
 unsigned long fds_bits [(1024/(8 * sizeof(unsigned long)))];
} __kernel_fd_set;
typedef void (*__kernel_sighandler_t)(int);
typedef int __kernel_key_t;
typedef int __kernel_mqd_t;
typedef unsigned long __kernel_ino_t;
typedef unsigned short __kernel_mode_t;
typedef unsigned short __kernel_nlink_t;
typedef long __kernel_off_t;
typedef int __kernel_pid_t;
typedef unsigned short __kernel_ipc_pid_t;
typedef unsigned short __kernel_uid_t;
typedef unsigned short __kernel_gid_t;
typedef unsigned int __kernel_size_t;
typedef int __kernel_ssize_t;
typedef int __kernel_ptrdiff_t;
typedef long __kernel_time_t;
typedef long __kernel_suseconds_t;
typedef long __kernel_clock_t;
typedef int __kernel_timer_t;
typedef int __kernel_clockid_t;
typedef int __kernel_daddr_t;
typedef char * __kernel_caddr_t;
typedef unsigned short __kernel_uid16_t;
typedef unsigned short __kernel_gid16_t;
typedef unsigned int __kernel_uid32_t;
typedef unsigned int __kernel_gid32_t;
typedef unsigned short __kernel_old_uid_t;
typedef unsigned short __kernel_old_gid_t;
typedef unsigned short __kernel_old_dev_t;
typedef long long __kernel_loff_t;
typedef struct {
 int __val[2];
} __kernel_fsid_t;
typedef unsigned short umode_t;
typedef __signed__ char __s8;
typedef unsigned char __u8;
typedef __signed__ short __s16;
typedef unsigned short __u16;
typedef __signed__ int __s32;
typedef unsigned int __u32;
typedef __signed__ long long __s64;
typedef unsigned long long __u64;
typedef __u16 __le16;
typedef __u16 __be16;
typedef __u32 __le32;
typedef __u32 __be32;
typedef __u64 __le64;
typedef __u64 __be64;
struct timezone
  {
    int tz_minuteswest;
    int tz_dsttime;
  };
typedef struct timezone *__restrict __timezone_ptr_t;
extern int gettimeofday (struct timeval *__restrict __tv,
    __timezone_ptr_t __tz) __attribute__ ((__nothrow__));
extern int settimeofday (__const struct timeval *__tv,
    __const struct timezone *__tz) __attribute__ ((__nothrow__));
extern int adjtime (__const struct timeval *__delta,
      struct timeval *__olddelta) __attribute__ ((__nothrow__));
enum __itimer_which
  {
    ITIMER_REAL = 0,
    ITIMER_VIRTUAL = 1,
    ITIMER_PROF = 2
  };
struct itimerval
  {
    struct timeval it_interval;
    struct timeval it_value;
  };
typedef int __itimer_which_t;
extern int getitimer (__itimer_which_t __which,
        struct itimerval *__value) __attribute__ ((__nothrow__));
extern int setitimer (__itimer_which_t __which,
        __const struct itimerval *__restrict __new,
        struct itimerval *__restrict __old) __attribute__ ((__nothrow__));
extern int utimes (__const char *__file, __const struct timeval __tvp[2])
     __attribute__ ((__nothrow__));
extern int lutimes (__const char *__file, __const struct timeval __tvp[2])
     __attribute__ ((__nothrow__));
extern int futimes (int __fd, __const struct timeval __tvp[2]) __attribute__ ((__nothrow__));
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
     DIMKI_SHIFT,
     DIMKI_CONTROL,
     DIMKI_ALT,
     DIMKI_ALTGR,
     DIMKI_META,
     DIMKI_SUPER,
     DIMKI_HYPER,
     DIMKI_FIRST = DIMKI_SHIFT,
     DIMKI_LAST = DIMKI_HYPER
} DFBInputDeviceModifierKeyIdentifier;
typedef enum {
     DIKI_UNKNOWN = ((DIKT_IDENTIFIER) | (0)),
     DIKI_A,
     DIKI_B,
     DIKI_C,
     DIKI_D,
     DIKI_E,
     DIKI_F,
     DIKI_G,
     DIKI_H,
     DIKI_I,
     DIKI_J,
     DIKI_K,
     DIKI_L,
     DIKI_M,
     DIKI_N,
     DIKI_O,
     DIKI_P,
     DIKI_Q,
     DIKI_R,
     DIKI_S,
     DIKI_T,
     DIKI_U,
     DIKI_V,
     DIKI_W,
     DIKI_X,
     DIKI_Y,
     DIKI_Z,
     DIKI_0,
     DIKI_1,
     DIKI_2,
     DIKI_3,
     DIKI_4,
     DIKI_5,
     DIKI_6,
     DIKI_7,
     DIKI_8,
     DIKI_9,
     DIKI_F1,
     DIKI_F2,
     DIKI_F3,
     DIKI_F4,
     DIKI_F5,
     DIKI_F6,
     DIKI_F7,
     DIKI_F8,
     DIKI_F9,
     DIKI_F10,
     DIKI_F11,
     DIKI_F12,
     DIKI_SHIFT_L,
     DIKI_SHIFT_R,
     DIKI_CONTROL_L,
     DIKI_CONTROL_R,
     DIKI_ALT_L,
     DIKI_ALT_R,
     DIKI_ALTGR,
     DIKI_META_L,
     DIKI_META_R,
     DIKI_SUPER_L,
     DIKI_SUPER_R,
     DIKI_HYPER_L,
     DIKI_HYPER_R,
     DIKI_CAPS_LOCK,
     DIKI_NUM_LOCK,
     DIKI_SCROLL_LOCK,
     DIKI_ESCAPE,
     DIKI_LEFT,
     DIKI_RIGHT,
     DIKI_UP,
     DIKI_DOWN,
     DIKI_TAB,
     DIKI_ENTER,
     DIKI_SPACE,
     DIKI_BACKSPACE,
     DIKI_INSERT,
     DIKI_DELETE,
     DIKI_HOME,
     DIKI_END,
     DIKI_PAGE_UP,
     DIKI_PAGE_DOWN,
     DIKI_PRINT,
     DIKI_PAUSE,
     DIKI_QUOTE_LEFT,
     DIKI_MINUS_SIGN,
     DIKI_EQUALS_SIGN,
     DIKI_BRACKET_LEFT,
     DIKI_BRACKET_RIGHT,
     DIKI_BACKSLASH,
     DIKI_SEMICOLON,
     DIKI_QUOTE_RIGHT,
     DIKI_COMMA,
     DIKI_PERIOD,
     DIKI_SLASH,
     DIKI_LESS_SIGN,
     DIKI_KP_DIV,
     DIKI_KP_MULT,
     DIKI_KP_MINUS,
     DIKI_KP_PLUS,
     DIKI_KP_ENTER,
     DIKI_KP_SPACE,
     DIKI_KP_TAB,
     DIKI_KP_F1,
     DIKI_KP_F2,
     DIKI_KP_F3,
     DIKI_KP_F4,
     DIKI_KP_EQUAL,
     DIKI_KP_SEPARATOR,
     DIKI_KP_DECIMAL,
     DIKI_KP_0,
     DIKI_KP_1,
     DIKI_KP_2,
     DIKI_KP_3,
     DIKI_KP_4,
     DIKI_KP_5,
     DIKI_KP_6,
     DIKI_KP_7,
     DIKI_KP_8,
     DIKI_KP_9,
     DIKI_KEYDEF_END,
     DIKI_NUMBER_OF_KEYS = DIKI_KEYDEF_END - ((DIKT_IDENTIFIER) | (0))
} DFBInputDeviceKeyIdentifier;
typedef enum {
     DIKS_NULL = ((DIKT_UNICODE) | (0x00)),
     DIKS_BACKSPACE = ((DIKT_UNICODE) | (0x08)),
     DIKS_TAB = ((DIKT_UNICODE) | (0x09)),
     DIKS_RETURN = ((DIKT_UNICODE) | (0x0D)),
     DIKS_CANCEL = ((DIKT_UNICODE) | (0x18)),
     DIKS_ESCAPE = ((DIKT_UNICODE) | (0x1B)),
     DIKS_SPACE = ((DIKT_UNICODE) | (0x20)),
     DIKS_EXCLAMATION_MARK = ((DIKT_UNICODE) | (0x21)),
     DIKS_QUOTATION = ((DIKT_UNICODE) | (0x22)),
     DIKS_NUMBER_SIGN = ((DIKT_UNICODE) | (0x23)),
     DIKS_DOLLAR_SIGN = ((DIKT_UNICODE) | (0x24)),
     DIKS_PERCENT_SIGN = ((DIKT_UNICODE) | (0x25)),
     DIKS_AMPERSAND = ((DIKT_UNICODE) | (0x26)),
     DIKS_APOSTROPHE = ((DIKT_UNICODE) | (0x27)),
     DIKS_PARENTHESIS_LEFT = ((DIKT_UNICODE) | (0x28)),
     DIKS_PARENTHESIS_RIGHT = ((DIKT_UNICODE) | (0x29)),
     DIKS_ASTERISK = ((DIKT_UNICODE) | (0x2A)),
     DIKS_PLUS_SIGN = ((DIKT_UNICODE) | (0x2B)),
     DIKS_COMMA = ((DIKT_UNICODE) | (0x2C)),
     DIKS_MINUS_SIGN = ((DIKT_UNICODE) | (0x2D)),
     DIKS_PERIOD = ((DIKT_UNICODE) | (0x2E)),
     DIKS_SLASH = ((DIKT_UNICODE) | (0x2F)),
     DIKS_0 = ((DIKT_UNICODE) | (0x30)),
     DIKS_1 = ((DIKT_UNICODE) | (0x31)),
     DIKS_2 = ((DIKT_UNICODE) | (0x32)),
     DIKS_3 = ((DIKT_UNICODE) | (0x33)),
     DIKS_4 = ((DIKT_UNICODE) | (0x34)),
     DIKS_5 = ((DIKT_UNICODE) | (0x35)),
     DIKS_6 = ((DIKT_UNICODE) | (0x36)),
     DIKS_7 = ((DIKT_UNICODE) | (0x37)),
     DIKS_8 = ((DIKT_UNICODE) | (0x38)),
     DIKS_9 = ((DIKT_UNICODE) | (0x39)),
     DIKS_COLON = ((DIKT_UNICODE) | (0x3A)),
     DIKS_SEMICOLON = ((DIKT_UNICODE) | (0x3B)),
     DIKS_LESS_THAN_SIGN = ((DIKT_UNICODE) | (0x3C)),
     DIKS_EQUALS_SIGN = ((DIKT_UNICODE) | (0x3D)),
     DIKS_GREATER_THAN_SIGN = ((DIKT_UNICODE) | (0x3E)),
     DIKS_QUESTION_MARK = ((DIKT_UNICODE) | (0x3F)),
     DIKS_AT = ((DIKT_UNICODE) | (0x40)),
     DIKS_CAPITAL_A = ((DIKT_UNICODE) | (0x41)),
     DIKS_CAPITAL_B = ((DIKT_UNICODE) | (0x42)),
     DIKS_CAPITAL_C = ((DIKT_UNICODE) | (0x43)),
     DIKS_CAPITAL_D = ((DIKT_UNICODE) | (0x44)),
     DIKS_CAPITAL_E = ((DIKT_UNICODE) | (0x45)),
     DIKS_CAPITAL_F = ((DIKT_UNICODE) | (0x46)),
     DIKS_CAPITAL_G = ((DIKT_UNICODE) | (0x47)),
     DIKS_CAPITAL_H = ((DIKT_UNICODE) | (0x48)),
     DIKS_CAPITAL_I = ((DIKT_UNICODE) | (0x49)),
     DIKS_CAPITAL_J = ((DIKT_UNICODE) | (0x4A)),
     DIKS_CAPITAL_K = ((DIKT_UNICODE) | (0x4B)),
     DIKS_CAPITAL_L = ((DIKT_UNICODE) | (0x4C)),
     DIKS_CAPITAL_M = ((DIKT_UNICODE) | (0x4D)),
     DIKS_CAPITAL_N = ((DIKT_UNICODE) | (0x4E)),
     DIKS_CAPITAL_O = ((DIKT_UNICODE) | (0x4F)),
     DIKS_CAPITAL_P = ((DIKT_UNICODE) | (0x50)),
     DIKS_CAPITAL_Q = ((DIKT_UNICODE) | (0x51)),
     DIKS_CAPITAL_R = ((DIKT_UNICODE) | (0x52)),
     DIKS_CAPITAL_S = ((DIKT_UNICODE) | (0x53)),
     DIKS_CAPITAL_T = ((DIKT_UNICODE) | (0x54)),
     DIKS_CAPITAL_U = ((DIKT_UNICODE) | (0x55)),
     DIKS_CAPITAL_V = ((DIKT_UNICODE) | (0x56)),
     DIKS_CAPITAL_W = ((DIKT_UNICODE) | (0x57)),
     DIKS_CAPITAL_X = ((DIKT_UNICODE) | (0x58)),
     DIKS_CAPITAL_Y = ((DIKT_UNICODE) | (0x59)),
     DIKS_CAPITAL_Z = ((DIKT_UNICODE) | (0x5A)),
     DIKS_SQUARE_BRACKET_LEFT = ((DIKT_UNICODE) | (0x5B)),
     DIKS_BACKSLASH = ((DIKT_UNICODE) | (0x5C)),
     DIKS_SQUARE_BRACKET_RIGHT = ((DIKT_UNICODE) | (0x5D)),
     DIKS_CIRCUMFLEX_ACCENT = ((DIKT_UNICODE) | (0x5E)),
     DIKS_UNDERSCORE = ((DIKT_UNICODE) | (0x5F)),
     DIKS_GRAVE_ACCENT = ((DIKT_UNICODE) | (0x60)),
     DIKS_SMALL_A = ((DIKT_UNICODE) | (0x61)),
     DIKS_SMALL_B = ((DIKT_UNICODE) | (0x62)),
     DIKS_SMALL_C = ((DIKT_UNICODE) | (0x63)),
     DIKS_SMALL_D = ((DIKT_UNICODE) | (0x64)),
     DIKS_SMALL_E = ((DIKT_UNICODE) | (0x65)),
     DIKS_SMALL_F = ((DIKT_UNICODE) | (0x66)),
     DIKS_SMALL_G = ((DIKT_UNICODE) | (0x67)),
     DIKS_SMALL_H = ((DIKT_UNICODE) | (0x68)),
     DIKS_SMALL_I = ((DIKT_UNICODE) | (0x69)),
     DIKS_SMALL_J = ((DIKT_UNICODE) | (0x6A)),
     DIKS_SMALL_K = ((DIKT_UNICODE) | (0x6B)),
     DIKS_SMALL_L = ((DIKT_UNICODE) | (0x6C)),
     DIKS_SMALL_M = ((DIKT_UNICODE) | (0x6D)),
     DIKS_SMALL_N = ((DIKT_UNICODE) | (0x6E)),
     DIKS_SMALL_O = ((DIKT_UNICODE) | (0x6F)),
     DIKS_SMALL_P = ((DIKT_UNICODE) | (0x70)),
     DIKS_SMALL_Q = ((DIKT_UNICODE) | (0x71)),
     DIKS_SMALL_R = ((DIKT_UNICODE) | (0x72)),
     DIKS_SMALL_S = ((DIKT_UNICODE) | (0x73)),
     DIKS_SMALL_T = ((DIKT_UNICODE) | (0x74)),
     DIKS_SMALL_U = ((DIKT_UNICODE) | (0x75)),
     DIKS_SMALL_V = ((DIKT_UNICODE) | (0x76)),
     DIKS_SMALL_W = ((DIKT_UNICODE) | (0x77)),
     DIKS_SMALL_X = ((DIKT_UNICODE) | (0x78)),
     DIKS_SMALL_Y = ((DIKT_UNICODE) | (0x79)),
     DIKS_SMALL_Z = ((DIKT_UNICODE) | (0x7A)),
     DIKS_CURLY_BRACKET_LEFT = ((DIKT_UNICODE) | (0x7B)),
     DIKS_VERTICAL_BAR = ((DIKT_UNICODE) | (0x7C)),
     DIKS_CURLY_BRACKET_RIGHT = ((DIKT_UNICODE) | (0x7D)),
     DIKS_TILDE = ((DIKT_UNICODE) | (0x7E)),
     DIKS_DELETE = ((DIKT_UNICODE) | (0x7F)),
     DIKS_ENTER = DIKS_RETURN,
     DIKS_CURSOR_LEFT = ((DIKT_SPECIAL) | (0x00)),
     DIKS_CURSOR_RIGHT = ((DIKT_SPECIAL) | (0x01)),
     DIKS_CURSOR_UP = ((DIKT_SPECIAL) | (0x02)),
     DIKS_CURSOR_DOWN = ((DIKT_SPECIAL) | (0x03)),
     DIKS_INSERT = ((DIKT_SPECIAL) | (0x04)),
     DIKS_HOME = ((DIKT_SPECIAL) | (0x05)),
     DIKS_END = ((DIKT_SPECIAL) | (0x06)),
     DIKS_PAGE_UP = ((DIKT_SPECIAL) | (0x07)),
     DIKS_PAGE_DOWN = ((DIKT_SPECIAL) | (0x08)),
     DIKS_PRINT = ((DIKT_SPECIAL) | (0x09)),
     DIKS_PAUSE = ((DIKT_SPECIAL) | (0x0A)),
     DIKS_OK = ((DIKT_SPECIAL) | (0x0B)),
     DIKS_SELECT = ((DIKT_SPECIAL) | (0x0C)),
     DIKS_GOTO = ((DIKT_SPECIAL) | (0x0D)),
     DIKS_CLEAR = ((DIKT_SPECIAL) | (0x0E)),
     DIKS_POWER = ((DIKT_SPECIAL) | (0x0F)),
     DIKS_POWER2 = ((DIKT_SPECIAL) | (0x10)),
     DIKS_OPTION = ((DIKT_SPECIAL) | (0x11)),
     DIKS_MENU = ((DIKT_SPECIAL) | (0x12)),
     DIKS_HELP = ((DIKT_SPECIAL) | (0x13)),
     DIKS_INFO = ((DIKT_SPECIAL) | (0x14)),
     DIKS_TIME = ((DIKT_SPECIAL) | (0x15)),
     DIKS_VENDOR = ((DIKT_SPECIAL) | (0x16)),
     DIKS_ARCHIVE = ((DIKT_SPECIAL) | (0x17)),
     DIKS_PROGRAM = ((DIKT_SPECIAL) | (0x18)),
     DIKS_CHANNEL = ((DIKT_SPECIAL) | (0x19)),
     DIKS_FAVORITES = ((DIKT_SPECIAL) | (0x1A)),
     DIKS_EPG = ((DIKT_SPECIAL) | (0x1B)),
     DIKS_PVR = ((DIKT_SPECIAL) | (0x1C)),
     DIKS_MHP = ((DIKT_SPECIAL) | (0x1D)),
     DIKS_LANGUAGE = ((DIKT_SPECIAL) | (0x1E)),
     DIKS_TITLE = ((DIKT_SPECIAL) | (0x1F)),
     DIKS_SUBTITLE = ((DIKT_SPECIAL) | (0x20)),
     DIKS_ANGLE = ((DIKT_SPECIAL) | (0x21)),
     DIKS_ZOOM = ((DIKT_SPECIAL) | (0x22)),
     DIKS_MODE = ((DIKT_SPECIAL) | (0x23)),
     DIKS_KEYBOARD = ((DIKT_SPECIAL) | (0x24)),
     DIKS_PC = ((DIKT_SPECIAL) | (0x25)),
     DIKS_SCREEN = ((DIKT_SPECIAL) | (0x26)),
     DIKS_TV = ((DIKT_SPECIAL) | (0x27)),
     DIKS_TV2 = ((DIKT_SPECIAL) | (0x28)),
     DIKS_VCR = ((DIKT_SPECIAL) | (0x29)),
     DIKS_VCR2 = ((DIKT_SPECIAL) | (0x2A)),
     DIKS_SAT = ((DIKT_SPECIAL) | (0x2B)),
     DIKS_SAT2 = ((DIKT_SPECIAL) | (0x2C)),
     DIKS_CD = ((DIKT_SPECIAL) | (0x2D)),
     DIKS_TAPE = ((DIKT_SPECIAL) | (0x2E)),
     DIKS_RADIO = ((DIKT_SPECIAL) | (0x2F)),
     DIKS_TUNER = ((DIKT_SPECIAL) | (0x30)),
     DIKS_PLAYER = ((DIKT_SPECIAL) | (0x31)),
     DIKS_TEXT = ((DIKT_SPECIAL) | (0x32)),
     DIKS_DVD = ((DIKT_SPECIAL) | (0x33)),
     DIKS_AUX = ((DIKT_SPECIAL) | (0x34)),
     DIKS_MP3 = ((DIKT_SPECIAL) | (0x35)),
     DIKS_PHONE = ((DIKT_SPECIAL) | (0x36)),
     DIKS_AUDIO = ((DIKT_SPECIAL) | (0x37)),
     DIKS_VIDEO = ((DIKT_SPECIAL) | (0x38)),
     DIKS_INTERNET = ((DIKT_SPECIAL) | (0x39)),
     DIKS_MAIL = ((DIKT_SPECIAL) | (0x3A)),
     DIKS_NEWS = ((DIKT_SPECIAL) | (0x3B)),
     DIKS_DIRECTORY = ((DIKT_SPECIAL) | (0x3C)),
     DIKS_LIST = ((DIKT_SPECIAL) | (0x3D)),
     DIKS_CALCULATOR = ((DIKT_SPECIAL) | (0x3E)),
     DIKS_MEMO = ((DIKT_SPECIAL) | (0x3F)),
     DIKS_CALENDAR = ((DIKT_SPECIAL) | (0x40)),
     DIKS_EDITOR = ((DIKT_SPECIAL) | (0x41)),
     DIKS_RED = ((DIKT_SPECIAL) | (0x42)),
     DIKS_GREEN = ((DIKT_SPECIAL) | (0x43)),
     DIKS_YELLOW = ((DIKT_SPECIAL) | (0x44)),
     DIKS_BLUE = ((DIKT_SPECIAL) | (0x45)),
     DIKS_CHANNEL_UP = ((DIKT_SPECIAL) | (0x46)),
     DIKS_CHANNEL_DOWN = ((DIKT_SPECIAL) | (0x47)),
     DIKS_BACK = ((DIKT_SPECIAL) | (0x48)),
     DIKS_FORWARD = ((DIKT_SPECIAL) | (0x49)),
     DIKS_FIRST = ((DIKT_SPECIAL) | (0x4A)),
     DIKS_LAST = ((DIKT_SPECIAL) | (0x4B)),
     DIKS_VOLUME_UP = ((DIKT_SPECIAL) | (0x4C)),
     DIKS_VOLUME_DOWN = ((DIKT_SPECIAL) | (0x4D)),
     DIKS_MUTE = ((DIKT_SPECIAL) | (0x4E)),
     DIKS_AB = ((DIKT_SPECIAL) | (0x4F)),
     DIKS_PLAYPAUSE = ((DIKT_SPECIAL) | (0x50)),
     DIKS_PLAY = ((DIKT_SPECIAL) | (0x51)),
     DIKS_STOP = ((DIKT_SPECIAL) | (0x52)),
     DIKS_RESTART = ((DIKT_SPECIAL) | (0x53)),
     DIKS_SLOW = ((DIKT_SPECIAL) | (0x54)),
     DIKS_FAST = ((DIKT_SPECIAL) | (0x55)),
     DIKS_RECORD = ((DIKT_SPECIAL) | (0x56)),
     DIKS_EJECT = ((DIKT_SPECIAL) | (0x57)),
     DIKS_SHUFFLE = ((DIKT_SPECIAL) | (0x58)),
     DIKS_REWIND = ((DIKT_SPECIAL) | (0x59)),
     DIKS_FASTFORWARD = ((DIKT_SPECIAL) | (0x5A)),
     DIKS_PREVIOUS = ((DIKT_SPECIAL) | (0x5B)),
     DIKS_NEXT = ((DIKT_SPECIAL) | (0x5C)),
     DIKS_BEGIN = ((DIKT_SPECIAL) | (0x5D)),
     DIKS_DIGITS = ((DIKT_SPECIAL) | (0x5E)),
     DIKS_TEEN = ((DIKT_SPECIAL) | (0x5F)),
     DIKS_TWEN = ((DIKT_SPECIAL) | (0x60)),
     DIKS_BREAK = ((DIKT_SPECIAL) | (0x61)),
     DIKS_EXIT = ((DIKT_SPECIAL) | (0x62)),
     DIKS_SETUP = ((DIKT_SPECIAL) | (0x63)),
     DIKS_CURSOR_LEFT_UP = ((DIKT_SPECIAL) | (0x64)),
     DIKS_CURSOR_LEFT_DOWN = ((DIKT_SPECIAL) | (0x65)),
     DIKS_CURSOR_UP_RIGHT = ((DIKT_SPECIAL) | (0x66)),
     DIKS_CURSOR_DOWN_RIGHT = ((DIKT_SPECIAL) | (0x67)),
     DIKS_F1 = (((DIKT_FUNCTION) | (1))),
     DIKS_F2 = (((DIKT_FUNCTION) | (2))),
     DIKS_F3 = (((DIKT_FUNCTION) | (3))),
     DIKS_F4 = (((DIKT_FUNCTION) | (4))),
     DIKS_F5 = (((DIKT_FUNCTION) | (5))),
     DIKS_F6 = (((DIKT_FUNCTION) | (6))),
     DIKS_F7 = (((DIKT_FUNCTION) | (7))),
     DIKS_F8 = (((DIKT_FUNCTION) | (8))),
     DIKS_F9 = (((DIKT_FUNCTION) | (9))),
     DIKS_F10 = (((DIKT_FUNCTION) | (10))),
     DIKS_F11 = (((DIKT_FUNCTION) | (11))),
     DIKS_F12 = (((DIKT_FUNCTION) | (12))),
     DIKS_SHIFT = (((DIKT_MODIFIER) | ((1 << DIMKI_SHIFT)))),
     DIKS_CONTROL = (((DIKT_MODIFIER) | ((1 << DIMKI_CONTROL)))),
     DIKS_ALT = (((DIKT_MODIFIER) | ((1 << DIMKI_ALT)))),
     DIKS_ALTGR = (((DIKT_MODIFIER) | ((1 << DIMKI_ALTGR)))),
     DIKS_META = (((DIKT_MODIFIER) | ((1 << DIMKI_META)))),
     DIKS_SUPER = (((DIKT_MODIFIER) | ((1 << DIMKI_SUPER)))),
     DIKS_HYPER = (((DIKT_MODIFIER) | ((1 << DIMKI_HYPER)))),
     DIKS_CAPS_LOCK = ((DIKT_LOCK) | (0x00)),
     DIKS_NUM_LOCK = ((DIKT_LOCK) | (0x01)),
     DIKS_SCROLL_LOCK = ((DIKT_LOCK) | (0x02)),
     DIKS_DEAD_ABOVEDOT = ((DIKT_DEAD) | (0x00)),
     DIKS_DEAD_ABOVERING = ((DIKT_DEAD) | (0x01)),
     DIKS_DEAD_ACUTE = ((DIKT_DEAD) | (0x02)),
     DIKS_DEAD_BREVE = ((DIKT_DEAD) | (0x03)),
     DIKS_DEAD_CARON = ((DIKT_DEAD) | (0x04)),
     DIKS_DEAD_CEDILLA = ((DIKT_DEAD) | (0x05)),
     DIKS_DEAD_CIRCUMFLEX = ((DIKT_DEAD) | (0x06)),
     DIKS_DEAD_DIAERESIS = ((DIKT_DEAD) | (0x07)),
     DIKS_DEAD_DOUBLEACUTE = ((DIKT_DEAD) | (0x08)),
     DIKS_DEAD_GRAVE = ((DIKT_DEAD) | (0x09)),
     DIKS_DEAD_IOTA = ((DIKT_DEAD) | (0x0A)),
     DIKS_DEAD_MACRON = ((DIKT_DEAD) | (0x0B)),
     DIKS_DEAD_OGONEK = ((DIKT_DEAD) | (0x0C)),
     DIKS_DEAD_SEMIVOICED_SOUND = ((DIKT_DEAD) | (0x0D)),
     DIKS_DEAD_TILDE = ((DIKT_DEAD) | (0x0E)),
     DIKS_DEAD_VOICED_SOUND = ((DIKT_DEAD) | (0x0F)),
     DIKS_CUSTOM0 = (((DIKT_CUSTOM) | (0))),
     DIKS_CUSTOM1 = (((DIKT_CUSTOM) | (1))),
     DIKS_CUSTOM2 = (((DIKT_CUSTOM) | (2))),
     DIKS_CUSTOM3 = (((DIKT_CUSTOM) | (3))),
     DIKS_CUSTOM4 = (((DIKT_CUSTOM) | (4))),
     DIKS_CUSTOM5 = (((DIKT_CUSTOM) | (5))),
     DIKS_CUSTOM6 = (((DIKT_CUSTOM) | (6))),
     DIKS_CUSTOM7 = (((DIKT_CUSTOM) | (7))),
     DIKS_CUSTOM8 = (((DIKT_CUSTOM) | (8))),
     DIKS_CUSTOM9 = (((DIKT_CUSTOM) | (9)))
} DFBInputDeviceKeySymbol;
typedef enum {
     DILS_SCROLL = 0x00000001,
     DILS_NUM = 0x00000002,
     DILS_CAPS = 0x00000004
} DFBInputDeviceLockState;
typedef enum {
     DIKSI_BASE = 0x00,
     DIKSI_BASE_SHIFT = 0x01,
     DIKSI_ALT = 0x02,
     DIKSI_ALT_SHIFT = 0x03,
     DIKSI_LAST = DIKSI_ALT_SHIFT
} DFBInputDeviceKeymapSymbolIndex;
typedef struct {
     int code;
     DFBInputDeviceLockState locks;
     DFBInputDeviceKeyIdentifier identifier;
     DFBInputDeviceKeySymbol symbols[DIKSI_LAST+1];
} DFBInputDeviceKeymapEntry;
extern const unsigned int directfb_major_version;
extern const unsigned int directfb_minor_version;
extern const unsigned int directfb_micro_version;
extern const unsigned int directfb_binary_age;
extern const unsigned int directfb_interface_age;
const char * DirectFBCheckVersion( unsigned int required_major,
                                   unsigned int required_minor,
                                   unsigned int required_micro );
typedef struct _IDirectFB IDirectFB;
typedef struct _IDirectFBScreen IDirectFBScreen;
typedef struct _IDirectFBDisplayLayer IDirectFBDisplayLayer;
typedef struct _IDirectFBSurface IDirectFBSurface;
typedef struct _IDirectFBPalette IDirectFBPalette;
typedef struct _IDirectFBWindow IDirectFBWindow;
typedef struct _IDirectFBInputDevice IDirectFBInputDevice;
typedef struct _IDirectFBEventBuffer IDirectFBEventBuffer;
typedef struct _IDirectFBFont IDirectFBFont;
typedef struct _IDirectFBImageProvider IDirectFBImageProvider;
typedef struct _IDirectFBVideoProvider IDirectFBVideoProvider;
typedef struct _IDirectFBDataBuffer IDirectFBDataBuffer;
typedef struct _IDirectFBGL IDirectFBGL;
typedef enum {
     DFB_OK,
     DFB_FAILURE,
     DFB_INIT,
     DFB_BUG,
     DFB_DEAD,
     DFB_UNSUPPORTED,
     DFB_UNIMPLEMENTED,
     DFB_ACCESSDENIED,
     DFB_INVARG,
     DFB_NOSYSTEMMEMORY,
     DFB_NOVIDEOMEMORY,
     DFB_LOCKED,
     DFB_BUFFEREMPTY,
     DFB_FILENOTFOUND,
     DFB_IO,
     DFB_BUSY,
     DFB_NOIMPL,
     DFB_MISSINGFONT,
     DFB_TIMEOUT,
     DFB_MISSINGIMAGE,
     DFB_THIZNULL,
     DFB_IDNOTFOUND,
     DFB_INVAREA,
     DFB_DESTROYED,
     DFB_FUSION,
     DFB_BUFFERTOOLARGE,
     DFB_INTERRUPTED,
     DFB_NOCONTEXT,
     DFB_TEMPUNAVAIL,
     DFB_LIMITEXCEEDED,
     DFB_NOSUCHMETHOD,
     DFB_NOSUCHINSTANCE,
     DFB_ITEMNOTFOUND,
     DFB_VERSIONMISMATCH,
     DFB_NOSHAREDMEMORY
} DFBResult;
typedef enum {
     DFB_FALSE = 0,
     DFB_TRUE = !DFB_FALSE
} DFBBoolean;
typedef struct {
     int x;
     int y;
} DFBPoint;
typedef struct {
     int x;
     int w;
} DFBSpan;
typedef struct {
     int w;
     int h;
} DFBDimension;
typedef struct {
     int x;
     int y;
     int w;
     int h;
} DFBRectangle;
typedef struct {
     float x;
     float y;
     float w;
     float h;
} DFBLocation;
typedef struct {
     int x1;
     int y1;
     int x2;
     int y2;
} DFBRegion;
typedef struct {
     int l;
     int t;
     int r;
     int b;
} DFBInsets;
typedef struct {
     int x1;
     int y1;
     int x2;
     int y2;
     int x3;
     int y3;
} DFBTriangle;
typedef struct {
     __u8 a;
     __u8 r;
     __u8 g;
     __u8 b;
} DFBColor;
DFBResult DirectFBError(
                             const char *msg,
                             DFBResult result
                       );
DFBResult DirectFBErrorFatal(
                             const char *msg,
                             DFBResult result
                            );
const char *DirectFBErrorString(
                         DFBResult result
                      );
const char *DirectFBUsageString( void );
DFBResult DirectFBInit(
                         int *argc,
                         char **argv[]
                      );
DFBResult DirectFBSetOption(
                         const char *name,
                         const char *value
                      );
DFBResult DirectFBCreate(
                          IDirectFB **interface
                        );
typedef unsigned int DFBScreenID;
typedef unsigned int DFBDisplayLayerID;
typedef unsigned int DFBDisplayLayerSourceID;
typedef unsigned int DFBWindowID;
typedef unsigned int DFBInputDeviceID;
typedef __u32 DFBDisplayLayerIDs;
typedef enum {
     DFSCL_NORMAL = 0x00000000,
     DFSCL_FULLSCREEN,
     DFSCL_EXCLUSIVE
} DFBCooperativeLevel;
typedef enum {
     DLCAPS_NONE = 0x00000000,
     DLCAPS_SURFACE = 0x00000001,
     DLCAPS_OPACITY = 0x00000002,
     DLCAPS_ALPHACHANNEL = 0x00000004,
     DLCAPS_SCREEN_LOCATION = 0x00000008,
     DLCAPS_FLICKER_FILTERING = 0x00000010,
     DLCAPS_DEINTERLACING = 0x00000020,
     DLCAPS_SRC_COLORKEY = 0x00000040,
     DLCAPS_DST_COLORKEY = 0x00000080,
     DLCAPS_BRIGHTNESS = 0x00000100,
     DLCAPS_CONTRAST = 0x00000200,
     DLCAPS_HUE = 0x00000400,
     DLCAPS_SATURATION = 0x00000800,
     DLCAPS_LEVELS = 0x00001000,
     DLCAPS_FIELD_PARITY = 0x00002000,
     DLCAPS_WINDOWS = 0x00004000,
     DLCAPS_SOURCES = 0x00008000,
     DLCAPS_ALPHA_RAMP = 0x00010000,
     DLCAPS_PREMULTIPLIED = 0x00020000,
     DLCAPS_SCREEN_POSITION = 0x00100000,
     DLCAPS_SCREEN_SIZE = 0x00200000,
     DLCAPS_ALL = 0x0033FFFF
} DFBDisplayLayerCapabilities;
typedef enum {
     DSCCAPS_NONE = 0x00000000,
     DSCCAPS_VSYNC = 0x00000001,
     DSCCAPS_POWER_MANAGEMENT = 0x00000002,
     DSCCAPS_MIXERS = 0x00000010,
     DSCCAPS_ENCODERS = 0x00000020,
     DSCCAPS_OUTPUTS = 0x00000040,
     DSCCAPS_ALL = 0x00000073
} DFBScreenCapabilities;
typedef enum {
     DLOP_NONE = 0x00000000,
     DLOP_ALPHACHANNEL = 0x00000001,
     DLOP_FLICKER_FILTERING = 0x00000002,
     DLOP_DEINTERLACING = 0x00000004,
     DLOP_SRC_COLORKEY = 0x00000008,
     DLOP_DST_COLORKEY = 0x00000010,
     DLOP_OPACITY = 0x00000020,
     DLOP_FIELD_PARITY = 0x00000040
} DFBDisplayLayerOptions;
typedef enum {
     DLBM_UNKNOWN = 0x00000000,
     DLBM_FRONTONLY = 0x00000001,
     DLBM_BACKVIDEO = 0x00000002,
     DLBM_BACKSYSTEM = 0x00000004,
     DLBM_TRIPLE = 0x00000008,
     DLBM_WINDOWS = 0x00000010
} DFBDisplayLayerBufferMode;
typedef enum {
     DSDESC_CAPS = 0x00000001,
     DSDESC_WIDTH = 0x00000002,
     DSDESC_HEIGHT = 0x00000004,
     DSDESC_PIXELFORMAT = 0x00000008,
     DSDESC_PREALLOCATED = 0x00000010,
     DSDESC_PALETTE = 0x00000020
} DFBSurfaceDescriptionFlags;
typedef enum {
     DPDESC_CAPS = 0x00000001,
     DPDESC_SIZE = 0x00000002,
     DPDESC_ENTRIES = 0x00000004
} DFBPaletteDescriptionFlags;
typedef enum {
     DSCAPS_NONE = 0x00000000,
     DSCAPS_PRIMARY = 0x00000001,
     DSCAPS_SYSTEMONLY = 0x00000002,
     DSCAPS_VIDEOONLY = 0x00000004,
     DSCAPS_DOUBLE = 0x00000010,
     DSCAPS_SUBSURFACE = 0x00000020,
     DSCAPS_INTERLACED = 0x00000040,
     DSCAPS_SEPARATED = 0x00000080,
     DSCAPS_STATIC_ALLOC = 0x00000100,
     DSCAPS_TRIPLE = 0x00000200,
     DSCAPS_PREMULTIPLIED = 0x00001000,
     DSCAPS_DEPTH = 0x00010000,
     DSCAPS_ALL = 0x000113F7,
     DSCAPS_FLIPPING = DSCAPS_DOUBLE | DSCAPS_TRIPLE
} DFBSurfaceCapabilities;
typedef enum {
     DPCAPS_NONE = 0x00000000
} DFBPaletteCapabilities;
typedef enum {
     DSDRAW_NOFX = 0x00000000,
     DSDRAW_BLEND = 0x00000001,
     DSDRAW_DST_COLORKEY = 0x00000002,
     DSDRAW_SRC_PREMULTIPLY = 0x00000004,
     DSDRAW_DST_PREMULTIPLY = 0x00000008,
     DSDRAW_DEMULTIPLY = 0x00000010,
     DSDRAW_XOR = 0x00000020
} DFBSurfaceDrawingFlags;
typedef enum {
     DSBLIT_NOFX = 0x00000000,
     DSBLIT_BLEND_ALPHACHANNEL = 0x00000001,
     DSBLIT_BLEND_COLORALPHA = 0x00000002,
     DSBLIT_COLORIZE = 0x00000004,
     DSBLIT_SRC_COLORKEY = 0x00000008,
     DSBLIT_DST_COLORKEY = 0x00000010,
     DSBLIT_SRC_PREMULTIPLY = 0x00000020,
     DSBLIT_DST_PREMULTIPLY = 0x00000040,
     DSBLIT_DEMULTIPLY = 0x00000080,
     DSBLIT_DEINTERLACE = 0x00000100
} DFBSurfaceBlittingFlags;
typedef enum {
     DFXL_NONE = 0x00000000,
     DFXL_FILLRECTANGLE = 0x00000001,
     DFXL_DRAWRECTANGLE = 0x00000002,
     DFXL_DRAWLINE = 0x00000004,
     DFXL_FILLTRIANGLE = 0x00000008,
     DFXL_BLIT = 0x00010000,
     DFXL_STRETCHBLIT = 0x00020000,
     DFXL_TEXTRIANGLES = 0x00040000,
     DFXL_DRAWSTRING = 0x01000000,
     DFXL_ALL = 0x0107000F
} DFBAccelerationMask;
typedef struct {
     DFBAccelerationMask acceleration_mask;
     DFBSurfaceDrawingFlags drawing_flags;
     DFBSurfaceBlittingFlags blitting_flags;
     unsigned int video_memory;
} DFBCardCapabilities;
typedef enum {
     DLTF_NONE = 0x00000000,
     DLTF_GRAPHICS = 0x00000001,
     DLTF_VIDEO = 0x00000002,
     DLTF_STILL_PICTURE = 0x00000004,
     DLTF_BACKGROUND = 0x00000008,
     DLTF_ALL = 0x0000000F
} DFBDisplayLayerTypeFlags;
typedef enum {
     DIDTF_NONE = 0x00000000,
     DIDTF_KEYBOARD = 0x00000001,
     DIDTF_MOUSE = 0x00000002,
     DIDTF_JOYSTICK = 0x00000004,
     DIDTF_REMOTE = 0x00000008,
     DIDTF_VIRTUAL = 0x00000010,
     DIDTF_ALL = 0x0000001F
} DFBInputDeviceTypeFlags;
typedef enum {
     DICAPS_KEYS = 0x00000001,
     DICAPS_AXES = 0x00000002,
     DICAPS_BUTTONS = 0x00000004,
     DICAPS_ALL = 0x00000007
} DFBInputDeviceCapabilities;
typedef enum {
     DIBI_LEFT = 0x00000000,
     DIBI_RIGHT = 0x00000001,
     DIBI_MIDDLE = 0x00000002,
     DIBI_FIRST = DIBI_LEFT,
     DIBI_LAST = 0x0000001F
} DFBInputDeviceButtonIdentifier;
typedef enum {
     DIAI_X = 0x00000000,
     DIAI_Y = 0x00000001,
     DIAI_Z = 0x00000002,
     DIAI_FIRST = DIAI_X,
     DIAI_LAST = 0x0000001F
} DFBInputDeviceAxisIdentifier;
typedef enum {
     DWDESC_CAPS = 0x00000001,
     DWDESC_WIDTH = 0x00000002,
     DWDESC_HEIGHT = 0x00000004,
     DWDESC_PIXELFORMAT = 0x00000008,
     DWDESC_POSX = 0x00000010,
     DWDESC_POSY = 0x00000020,
     DWDESC_SURFACE_CAPS = 0x00000040
} DFBWindowDescriptionFlags;
typedef enum {
     DBDESC_FILE = 0x00000001,
     DBDESC_MEMORY = 0x00000002
} DFBDataBufferDescriptionFlags;
typedef enum {
     DWCAPS_NONE = 0x00000000,
     DWCAPS_ALPHACHANNEL = 0x00000001,
     DWCAPS_DOUBLEBUFFER = 0x00000002,
     DWCAPS_INPUTONLY = 0x00000004,
     DWCAPS_NODECORATION = 0x00000008,
     DWCAPS_ALL = 0x0000000F
} DFBWindowCapabilities;
typedef enum {
     DFFA_NONE = 0x00000000,
     DFFA_NOKERNING = 0x00000001,
     DFFA_NOHINTING = 0x00000002,
     DFFA_MONOCHROME = 0x00000004,
     DFFA_NOCHARMAP = 0x00000008
} DFBFontAttributes;
typedef enum {
     DFDESC_ATTRIBUTES = 0x00000001,
     DFDESC_HEIGHT = 0x00000002,
     DFDESC_WIDTH = 0x00000004,
     DFDESC_INDEX = 0x00000008,
     DFDESC_FIXEDADVANCE = 0x00000010
} DFBFontDescriptionFlags;
typedef struct {
     DFBFontDescriptionFlags flags;
     DFBFontAttributes attributes;
     int height;
     int width;
     unsigned int index;
     int fixed_advance;
} DFBFontDescription;
typedef enum {
     DSPF_UNKNOWN = 0x00000000,
     DSPF_ARGB1555 = ( (((0 ) & 0x7F) ) | (((15) & 0x1F) << 7) | (((1) & 0x0F) << 12) | (((1 ) ? 1 :0) << 16) | (((0 ) & 0x07) << 17) | (((2 ) & 0x07) << 20) | (((0 ) & 0x07) << 23) | (((0 ) & 0x03) << 26) | (((0 ) & 0x03) << 28) | (((0 ) ? 1 :0) << 30) | (((0 ) ? 1 :0) << 31) ),
     DSPF_RGB16 = ( (((1 ) & 0x7F) ) | (((16) & 0x1F) << 7) | (((0) & 0x0F) << 12) | (((0 ) ? 1 :0) << 16) | (((0 ) & 0x07) << 17) | (((2 ) & 0x07) << 20) | (((0 ) & 0x07) << 23) | (((0 ) & 0x03) << 26) | (((0 ) & 0x03) << 28) | (((0 ) ? 1 :0) << 30) | (((0 ) ? 1 :0) << 31) ),
     DSPF_RGB24 = ( (((2 ) & 0x7F) ) | (((24) & 0x1F) << 7) | (((0) & 0x0F) << 12) | (((0 ) ? 1 :0) << 16) | (((0 ) & 0x07) << 17) | (((3 ) & 0x07) << 20) | (((0 ) & 0x07) << 23) | (((0 ) & 0x03) << 26) | (((0 ) & 0x03) << 28) | (((0 ) ? 1 :0) << 30) | (((0 ) ? 1 :0) << 31) ),
     DSPF_RGB32 = ( (((3 ) & 0x7F) ) | (((24) & 0x1F) << 7) | (((0) & 0x0F) << 12) | (((0 ) ? 1 :0) << 16) | (((0 ) & 0x07) << 17) | (((4 ) & 0x07) << 20) | (((0 ) & 0x07) << 23) | (((0 ) & 0x03) << 26) | (((0 ) & 0x03) << 28) | (((0 ) ? 1 :0) << 30) | (((0 ) ? 1 :0) << 31) ),
     DSPF_ARGB = ( (((4 ) & 0x7F) ) | (((24) & 0x1F) << 7) | (((8) & 0x0F) << 12) | (((1 ) ? 1 :0) << 16) | (((0 ) & 0x07) << 17) | (((4 ) & 0x07) << 20) | (((0 ) & 0x07) << 23) | (((0 ) & 0x03) << 26) | (((0 ) & 0x03) << 28) | (((0 ) ? 1 :0) << 30) | (((0 ) ? 1 :0) << 31) ),
     DSPF_A8 = ( (((5 ) & 0x7F) ) | (((0) & 0x1F) << 7) | (((8) & 0x0F) << 12) | (((1 ) ? 1 :0) << 16) | (((0 ) & 0x07) << 17) | (((1 ) & 0x07) << 20) | (((0 ) & 0x07) << 23) | (((0 ) & 0x03) << 26) | (((0 ) & 0x03) << 28) | (((0 ) ? 1 :0) << 30) | (((0 ) ? 1 :0) << 31) ),
     DSPF_YUY2 = ( (((6 ) & 0x7F) ) | (((16) & 0x1F) << 7) | (((0) & 0x0F) << 12) | (((0 ) ? 1 :0) << 16) | (((0 ) & 0x07) << 17) | (((2 ) & 0x07) << 20) | (((0 ) & 0x07) << 23) | (((0 ) & 0x03) << 26) | (((0 ) & 0x03) << 28) | (((0 ) ? 1 :0) << 30) | (((0 ) ? 1 :0) << 31) ),
     DSPF_RGB332 = ( (((7 ) & 0x7F) ) | (((8) & 0x1F) << 7) | (((0) & 0x0F) << 12) | (((0 ) ? 1 :0) << 16) | (((0 ) & 0x07) << 17) | (((1 ) & 0x07) << 20) | (((0 ) & 0x07) << 23) | (((0 ) & 0x03) << 26) | (((0 ) & 0x03) << 28) | (((0 ) ? 1 :0) << 30) | (((0 ) ? 1 :0) << 31) ),
     DSPF_UYVY = ( (((8 ) & 0x7F) ) | (((16) & 0x1F) << 7) | (((0) & 0x0F) << 12) | (((0 ) ? 1 :0) << 16) | (((0 ) & 0x07) << 17) | (((2 ) & 0x07) << 20) | (((0 ) & 0x07) << 23) | (((0 ) & 0x03) << 26) | (((0 ) & 0x03) << 28) | (((0 ) ? 1 :0) << 30) | (((0 ) ? 1 :0) << 31) ),
     DSPF_I420 = ( (((9 ) & 0x7F) ) | (((12) & 0x1F) << 7) | (((0) & 0x0F) << 12) | (((0 ) ? 1 :0) << 16) | (((0 ) & 0x07) << 17) | (((1 ) & 0x07) << 20) | (((0 ) & 0x07) << 23) | (((2 ) & 0x03) << 26) | (((0 ) & 0x03) << 28) | (((0 ) ? 1 :0) << 30) | (((0 ) ? 1 :0) << 31) ),
     DSPF_YV12 = ( (((10 ) & 0x7F) ) | (((12) & 0x1F) << 7) | (((0) & 0x0F) << 12) | (((0 ) ? 1 :0) << 16) | (((0 ) & 0x07) << 17) | (((1 ) & 0x07) << 20) | (((0 ) & 0x07) << 23) | (((2 ) & 0x03) << 26) | (((0 ) & 0x03) << 28) | (((0 ) ? 1 :0) << 30) | (((0 ) ? 1 :0) << 31) ),
     DSPF_LUT8 = ( (((11 ) & 0x7F) ) | (((8) & 0x1F) << 7) | (((0) & 0x0F) << 12) | (((1 ) ? 1 :0) << 16) | (((0 ) & 0x07) << 17) | (((1 ) & 0x07) << 20) | (((0 ) & 0x07) << 23) | (((0 ) & 0x03) << 26) | (((0 ) & 0x03) << 28) | (((1 ) ? 1 :0) << 30) | (((0 ) ? 1 :0) << 31) ),
     DSPF_ALUT44 = ( (((12 ) & 0x7F) ) | (((4) & 0x1F) << 7) | (((4) & 0x0F) << 12) | (((1 ) ? 1 :0) << 16) | (((0 ) & 0x07) << 17) | (((1 ) & 0x07) << 20) | (((0 ) & 0x07) << 23) | (((0 ) & 0x03) << 26) | (((0 ) & 0x03) << 28) | (((1 ) ? 1 :0) << 30) | (((0 ) ? 1 :0) << 31) ),
     DSPF_AiRGB = ( (((13 ) & 0x7F) ) | (((24) & 0x1F) << 7) | (((8) & 0x0F) << 12) | (((1 ) ? 1 :0) << 16) | (((0 ) & 0x07) << 17) | (((4 ) & 0x07) << 20) | (((0 ) & 0x07) << 23) | (((0 ) & 0x03) << 26) | (((0 ) & 0x03) << 28) | (((0 ) ? 1 :0) << 30) | (((1 ) ? 1 :0) << 31) ),
     DSPF_A1 = ( (((14 ) & 0x7F) ) | (((0) & 0x1F) << 7) | (((1) & 0x0F) << 12) | (((1 ) ? 1 :0) << 16) | (((1 ) & 0x07) << 17) | (((0 ) & 0x07) << 20) | (((7 ) & 0x07) << 23) | (((0 ) & 0x03) << 26) | (((0 ) & 0x03) << 28) | (((0 ) ? 1 :0) << 30) | (((0 ) ? 1 :0) << 31) ),
     DSPF_NV12 = ( (((15 ) & 0x7F) ) | (((12) & 0x1F) << 7) | (((0) & 0x0F) << 12) | (((0 ) ? 1 :0) << 16) | (((0 ) & 0x07) << 17) | (((1 ) & 0x07) << 20) | (((0 ) & 0x07) << 23) | (((2 ) & 0x03) << 26) | (((0 ) & 0x03) << 28) | (((0 ) ? 1 :0) << 30) | (((0 ) ? 1 :0) << 31) ),
     DSPF_NV16 = ( (((16 ) & 0x7F) ) | (((24) & 0x1F) << 7) | (((0) & 0x0F) << 12) | (((0 ) ? 1 :0) << 16) | (((0 ) & 0x07) << 17) | (((1 ) & 0x07) << 20) | (((0 ) & 0x07) << 23) | (((0 ) & 0x03) << 26) | (((2 ) & 0x03) << 28) | (((0 ) ? 1 :0) << 30) | (((0 ) ? 1 :0) << 31) ),
     DSPF_ARGB2554 = ( (((17 ) & 0x7F) ) | (((14) & 0x1F) << 7) | (((2) & 0x0F) << 12) | (((1 ) ? 1 :0) << 16) | (((0 ) & 0x07) << 17) | (((2 ) & 0x07) << 20) | (((0 ) & 0x07) << 23) | (((0 ) & 0x03) << 26) | (((0 ) & 0x03) << 28) | (((0 ) ? 1 :0) << 30) | (((0 ) ? 1 :0) << 31) ),
     DSPF_ARGB4444 = ( (((18 ) & 0x7F) ) | (((12) & 0x1F) << 7) | (((4) & 0x0F) << 12) | (((1 ) ? 1 :0) << 16) | (((0 ) & 0x07) << 17) | (((2 ) & 0x07) << 20) | (((0 ) & 0x07) << 23) | (((0 ) & 0x03) << 26) | (((0 ) & 0x03) << 28) | (((0 ) ? 1 :0) << 30) | (((0 ) ? 1 :0) << 31) ),
     DSPF_NV21 = ( (((19 ) & 0x7F) ) | (((12) & 0x1F) << 7) | (((0) & 0x0F) << 12) | (((0 ) ? 1 :0) << 16) | (((0 ) & 0x07) << 17) | (((1 ) & 0x07) << 20) | (((0 ) & 0x07) << 23) | (((2 ) & 0x03) << 26) | (((0 ) & 0x03) << 28) | (((0 ) ? 1 :0) << 30) | (((0 ) ? 1 :0) << 31) )
} DFBSurfacePixelFormat;
typedef struct {
     DFBSurfaceDescriptionFlags flags;
     DFBSurfaceCapabilities caps;
     int width;
     int height;
     DFBSurfacePixelFormat pixelformat;
     struct {
          void *data;
          int pitch;
     } preallocated[2];
     struct {
          DFBColor *entries;
          unsigned int size;
     } palette;
} DFBSurfaceDescription;
typedef struct {
     DFBPaletteDescriptionFlags flags;
     DFBPaletteCapabilities caps;
     unsigned int size;
     DFBColor *entries;
} DFBPaletteDescription;
typedef struct {
     DFBDisplayLayerTypeFlags type;
     DFBDisplayLayerCapabilities caps;
     char name[32];
     int level;
     int regions;
     int sources;
} DFBDisplayLayerDescription;
typedef struct {
     DFBDisplayLayerSourceID source_id;
     char name[24];
} DFBDisplayLayerSourceDescription;
typedef struct {
     DFBScreenCapabilities caps;
     char name[32];
     int mixers;
     int encoders;
     int outputs;
} DFBScreenDescription;
typedef struct {
     DFBInputDeviceTypeFlags type;
     DFBInputDeviceCapabilities caps;
     int min_keycode;
     int max_keycode;
     DFBInputDeviceAxisIdentifier max_axis;
     DFBInputDeviceButtonIdentifier max_button;
     char name[32];
     char vendor[40];
} DFBInputDeviceDescription;
typedef struct {
     DFBWindowDescriptionFlags flags;
     DFBWindowCapabilities caps;
     int width;
     int height;
     DFBSurfacePixelFormat pixelformat;
     int posx;
     int posy;
     DFBSurfaceCapabilities surface_caps;
} DFBWindowDescription;
typedef struct {
     DFBDataBufferDescriptionFlags flags;
     const char *file;
     struct {
          const void *data;
          unsigned int length;
     } memory;
} DFBDataBufferDescription;
typedef enum {
     DFENUM_OK = 0x00000000,
     DFENUM_CANCEL = 0x00000001
} DFBEnumerationResult;
typedef DFBEnumerationResult (*DFBVideoModeCallback) (
     int width,
     int height,
     int bpp,
     void *callbackdata
);
typedef DFBEnumerationResult (*DFBScreenCallback) (
     DFBScreenID screen_id,
     DFBScreenDescription desc,
     void *callbackdata
);
typedef DFBEnumerationResult (*DFBDisplayLayerCallback) (
     DFBDisplayLayerID layer_id,
     DFBDisplayLayerDescription desc,
     void *callbackdata
);
typedef DFBEnumerationResult (*DFBInputDeviceCallback) (
     DFBInputDeviceID device_id,
     DFBInputDeviceDescription desc,
     void *callbackdata
);
typedef int (*DFBGetDataCallback) (
     void *buffer,
     unsigned int length,
     void *callbackdata
);
typedef enum {
     DVCAPS_BASIC = 0x00000000,
     DVCAPS_SEEK = 0x00000001,
     DVCAPS_SCALE = 0x00000002,
     DVCAPS_INTERLACED = 0x00000004,
     DVCAPS_BRIGHTNESS = 0x00000010,
     DVCAPS_CONTRAST = 0x00000020,
     DVCAPS_HUE = 0x00000040,
     DVCAPS_SATURATION = 0x00000080
} DFBVideoProviderCapabilities;
typedef enum {
     DCAF_NONE = 0x00000000,
     DCAF_BRIGHTNESS = 0x00000001,
     DCAF_CONTRAST = 0x00000002,
     DCAF_HUE = 0x00000004,
     DCAF_SATURATION = 0x00000008
} DFBColorAdjustmentFlags;
typedef struct {
     DFBColorAdjustmentFlags flags;
     __u16 brightness;
     __u16 contrast;
     __u16 hue;
     __u16 saturation;
} DFBColorAdjustment;
struct _IDirectFB { void *priv; int magic; DFBResult (*AddRef)( IDirectFB *thiz ); DFBResult (*Release)( IDirectFB *thiz ); DFBResult (*SetCooperativeLevel) ( IDirectFB *thiz, DFBCooperativeLevel level ); DFBResult (*SetVideoMode) ( IDirectFB *thiz, int width, int height, int bpp ); DFBResult (*GetCardCapabilities) ( IDirectFB *thiz, DFBCardCapabilities *ret_caps ); DFBResult (*EnumVideoModes) ( IDirectFB *thiz, DFBVideoModeCallback callback, void *callbackdata ); DFBResult (*CreateSurface) ( IDirectFB *thiz, const DFBSurfaceDescription *desc, IDirectFBSurface **ret_interface ); DFBResult (*CreatePalette) ( IDirectFB *thiz, const DFBPaletteDescription *desc, IDirectFBPalette **ret_interface ); DFBResult (*EnumScreens) ( IDirectFB *thiz, DFBScreenCallback callback, void *callbackdata ); DFBResult (*GetScreen) ( IDirectFB *thiz, DFBScreenID screen_id, IDirectFBScreen **ret_interface ); DFBResult (*EnumDisplayLayers) ( IDirectFB *thiz, DFBDisplayLayerCallback callback, void *callbackdata ); DFBResult (*GetDisplayLayer) ( IDirectFB *thiz, DFBDisplayLayerID layer_id, IDirectFBDisplayLayer **ret_interface ); DFBResult (*EnumInputDevices) ( IDirectFB *thiz, DFBInputDeviceCallback callback, void *callbackdata ); DFBResult (*GetInputDevice) ( IDirectFB *thiz, DFBInputDeviceID device_id, IDirectFBInputDevice **ret_interface ); DFBResult (*CreateEventBuffer) ( IDirectFB *thiz, IDirectFBEventBuffer **ret_buffer ); DFBResult (*CreateInputEventBuffer) ( IDirectFB *thiz, DFBInputDeviceCapabilities caps, DFBBoolean global, IDirectFBEventBuffer **ret_buffer ); DFBResult (*CreateImageProvider) ( IDirectFB *thiz, const char *filename, IDirectFBImageProvider **ret_interface ); DFBResult (*CreateVideoProvider) ( IDirectFB *thiz, const char *filename, IDirectFBVideoProvider **ret_interface ); DFBResult (*CreateFont) ( IDirectFB *thiz, const char *filename, const DFBFontDescription *desc, IDirectFBFont **ret_interface ); DFBResult (*CreateDataBuffer) ( IDirectFB *thiz, const DFBDataBufferDescription *desc, IDirectFBDataBuffer **ret_interface ); DFBResult (*SetClipboardData) ( IDirectFB *thiz, const char *mime_type, const void *data, unsigned int size, struct timeval *ret_timestamp ); DFBResult (*GetClipboardData) ( IDirectFB *thiz, char **ret_mimetype, void **ret_data, unsigned int *ret_size ); DFBResult (*GetClipboardTimeStamp) ( IDirectFB *thiz, struct timeval *ret_timestamp ); DFBResult (*Suspend) ( IDirectFB *thiz ); DFBResult (*Resume) ( IDirectFB *thiz ); DFBResult (*WaitIdle) ( IDirectFB *thiz ); DFBResult (*WaitForSync) ( IDirectFB *thiz ); DFBResult (*GetInterface) ( IDirectFB *thiz, const char *type, const char *implementation, void *arg, void **ret_interface ); };
typedef enum {
     DLSCL_SHARED = 0,
     DLSCL_EXCLUSIVE,
     DLSCL_ADMINISTRATIVE
} DFBDisplayLayerCooperativeLevel;
typedef enum {
     DLBM_DONTCARE = 0,
     DLBM_COLOR,
     DLBM_IMAGE,
     DLBM_TILE
} DFBDisplayLayerBackgroundMode;
typedef enum {
     DLCONF_NONE = 0x00000000,
     DLCONF_WIDTH = 0x00000001,
     DLCONF_HEIGHT = 0x00000002,
     DLCONF_PIXELFORMAT = 0x00000004,
     DLCONF_BUFFERMODE = 0x00000008,
     DLCONF_OPTIONS = 0x00000010,
     DLCONF_SOURCE = 0x00000020,
     DLCONF_SURFACE_CAPS = 0x00000040,
     DLCONF_ALL = 0x0000007F
} DFBDisplayLayerConfigFlags;
typedef struct {
     DFBDisplayLayerConfigFlags flags;
     int width;
     int height;
     DFBSurfacePixelFormat pixelformat;
     DFBDisplayLayerBufferMode buffermode;
     DFBDisplayLayerOptions options;
     DFBDisplayLayerSourceID source;
     DFBSurfaceCapabilities surface_caps;
} DFBDisplayLayerConfig;
typedef enum {
     DSPM_ON = 0,
     DSPM_STANDBY,
     DSPM_SUSPEND,
     DSPM_OFF
} DFBScreenPowerMode;
typedef enum {
     DSMCAPS_NONE = 0x00000000,
     DSMCAPS_FULL = 0x00000001,
     DSMCAPS_SUB_LEVEL = 0x00000002,
     DSMCAPS_SUB_LAYERS = 0x00000004,
     DSMCAPS_BACKGROUND = 0x00000008
} DFBScreenMixerCapabilities;
typedef struct {
     DFBScreenMixerCapabilities caps;
     DFBDisplayLayerIDs layers;
     int sub_num;
     DFBDisplayLayerIDs sub_layers;
     char name[24];
} DFBScreenMixerDescription;
typedef enum {
     DSMCONF_NONE = 0x00000000,
     DSMCONF_TREE = 0x00000001,
     DSMCONF_LEVEL = 0x00000002,
     DSMCONF_LAYERS = 0x00000004,
     DSMCONF_BACKGROUND = 0x00000010,
     DSMCONF_ALL = 0x00000017
} DFBScreenMixerConfigFlags;
typedef enum {
     DSMT_UNKNOWN = 0x00000000,
     DSMT_FULL = 0x00000001,
     DSMT_SUB_LEVEL = 0x00000002,
     DSMT_SUB_LAYERS = 0x00000003
} DFBScreenMixerTree;
typedef struct {
     DFBScreenMixerConfigFlags flags;
     DFBScreenMixerTree tree;
     int level;
     DFBDisplayLayerIDs layers;
     DFBColor background;
} DFBScreenMixerConfig;
typedef enum {
     DSOCAPS_NONE = 0x00000000,
     DSOCAPS_CONNECTORS = 0x00000001,
     DSOCAPS_ENCODER_SEL = 0x00000010,
     DSOCAPS_SIGNAL_SEL = 0x00000020,
     DSOCAPS_CONNECTOR_SEL = 0x00000040,
     DSOCAPS_ALL = 0x00000071
} DFBScreenOutputCapabilities;
typedef enum {
     DSOC_UNKNOWN = 0x00000000,
     DSOC_VGA = 0x00000001,
     DSOC_SCART = 0x00000002,
     DSOC_YC = 0x00000004,
     DSOC_CVBS = 0x00000008
} DFBScreenOutputConnectors;
typedef enum {
     DSOS_NONE = 0x00000000,
     DSOS_VGA = 0x00000001,
     DSOS_YC = 0x00000002,
     DSOS_CVBS = 0x00000004,
     DSOS_RGB = 0x00000008,
     DSOS_YCBCR = 0x00000010
} DFBScreenOutputSignals;
typedef struct {
     DFBScreenOutputCapabilities caps;
     DFBScreenOutputConnectors all_connectors;
     DFBScreenOutputSignals all_signals;
     char name[24];
} DFBScreenOutputDescription;
typedef enum {
     DSOCONF_NONE = 0x00000000,
     DSOCONF_ENCODER = 0x00000001,
     DSOCONF_SIGNALS = 0x00000002,
     DSOCONF_CONNECTORS = 0x00000004,
     DSOCONF_ALL = 0x00000007
} DFBScreenOutputConfigFlags;
typedef struct {
     DFBScreenOutputConfigFlags flags;
     int encoder;
     DFBScreenOutputSignals out_signals;
     DFBScreenOutputConnectors out_connectors;
} DFBScreenOutputConfig;
typedef enum {
     DSECAPS_NONE = 0x00000000,
     DSECAPS_TV_STANDARDS = 0x00000001,
     DSECAPS_TEST_PICTURE = 0x00000002,
     DSECAPS_MIXER_SEL = 0x00000004,
     DSECAPS_OUT_SIGNALS = 0x00000008,
     DSECAPS_SCANMODE = 0x00000010,
     DSECAPS_BRIGHTNESS = 0x00000100,
     DSECAPS_CONTRAST = 0x00000200,
     DSECAPS_HUE = 0x00000400,
     DSECAPS_SATURATION = 0x00000800,
     DSECAPS_ALL = 0x00000f1f
} DFBScreenEncoderCapabilities;
typedef enum {
     DSET_UNKNOWN = 0x00000000,
     DSET_CRTC = 0x00000001,
     DSET_TV = 0x00000002
} DFBScreenEncoderType;
typedef enum {
     DSETV_UNKNOWN = 0x00000000,
     DSETV_PAL = 0x00000001,
     DSETV_NTSC = 0x00000002,
     DSETV_SECAM = 0x00000004
} DFBScreenEncoderTVStandards;
typedef enum {
     DSESM_UNKNOWN = 0x00000000,
     DSESM_INTERLACED = 0x00000001,
     DSESM_PROGRESSIVE = 0x00000002
} DFBScreenEncoderScanMode;
typedef struct {
     DFBScreenEncoderCapabilities caps;
     DFBScreenEncoderType type;
     DFBScreenEncoderTVStandards tv_standards;
     DFBScreenOutputSignals out_signals;
     char name[24];
} DFBScreenEncoderDescription;
typedef enum {
     DSECONF_NONE = 0x00000000,
     DSECONF_TV_STANDARD = 0x00000001,
     DSECONF_TEST_PICTURE = 0x00000002,
     DSECONF_MIXER = 0x00000004,
     DSECONF_OUT_SIGNALS = 0x00000008,
     DSECONF_SCANMODE = 0x00000010,
     DSECONF_TEST_COLOR = 0x00000020,
     DSECONF_ADJUSTMENT = 0x00000040,
     DSECONF_ALL = 0x0000007F
} DFBScreenEncoderConfigFlags;
typedef enum {
     DSETP_OFF = 0x00000000,
     DSETP_MULTI = 0x00000001,
     DSETP_SINGLE = 0x00000002,
     DSETP_WHITE = 0x00000010,
     DSETP_YELLOW = 0x00000020,
     DSETP_CYAN = 0x00000030,
     DSETP_GREEN = 0x00000040,
     DSETP_MAGENTA = 0x00000050,
     DSETP_RED = 0x00000060,
     DSETP_BLUE = 0x00000070,
     DSETP_BLACK = 0x00000080
} DFBScreenEncoderTestPicture;
typedef struct {
     DFBScreenEncoderConfigFlags flags;
     DFBScreenEncoderTVStandards tv_standard;
     DFBScreenEncoderTestPicture test_picture;
     int mixer;
     DFBScreenOutputSignals out_signals;
     DFBScreenEncoderScanMode scanmode;
     DFBColor test_color;
     DFBColorAdjustment adjustment;
} DFBScreenEncoderConfig;
struct _IDirectFBScreen { void *priv; int magic; DFBResult (*AddRef)( IDirectFBScreen *thiz ); DFBResult (*Release)( IDirectFBScreen *thiz ); DFBResult (*GetID) ( IDirectFBScreen *thiz, DFBScreenID *ret_screen_id ); DFBResult (*GetDescription) ( IDirectFBScreen *thiz, DFBScreenDescription *ret_desc ); DFBResult (*EnumDisplayLayers) ( IDirectFBScreen *thiz, DFBDisplayLayerCallback callback, void *callbackdata ); DFBResult (*SetPowerMode) ( IDirectFBScreen *thiz, DFBScreenPowerMode mode ); DFBResult (*WaitForSync) ( IDirectFBScreen *thiz ); DFBResult (*GetMixerDescriptions) ( IDirectFBScreen *thiz, DFBScreenMixerDescription *ret_descriptions ); DFBResult (*GetMixerConfiguration) ( IDirectFBScreen *thiz, int mixer, DFBScreenMixerConfig *ret_config ); DFBResult (*TestMixerConfiguration) ( IDirectFBScreen *thiz, int mixer, const DFBScreenMixerConfig *config, DFBScreenMixerConfigFlags *ret_failed ); DFBResult (*SetMixerConfiguration) ( IDirectFBScreen *thiz, int mixer, const DFBScreenMixerConfig *config ); DFBResult (*GetEncoderDescriptions) ( IDirectFBScreen *thiz, DFBScreenEncoderDescription *ret_descriptions ); DFBResult (*GetEncoderConfiguration) ( IDirectFBScreen *thiz, int encoder, DFBScreenEncoderConfig *ret_config ); DFBResult (*TestEncoderConfiguration) ( IDirectFBScreen *thiz, int encoder, const DFBScreenEncoderConfig *config, DFBScreenEncoderConfigFlags *ret_failed ); DFBResult (*SetEncoderConfiguration) ( IDirectFBScreen *thiz, int encoder, const DFBScreenEncoderConfig *config ); DFBResult (*GetOutputDescriptions) ( IDirectFBScreen *thiz, DFBScreenOutputDescription *ret_descriptions ); DFBResult (*GetOutputConfiguration) ( IDirectFBScreen *thiz, int output, DFBScreenOutputConfig *ret_config ); DFBResult (*TestOutputConfiguration) ( IDirectFBScreen *thiz, int output, const DFBScreenOutputConfig *config, DFBScreenOutputConfigFlags *ret_failed ); DFBResult (*SetOutputConfiguration) ( IDirectFBScreen *thiz, int output, const DFBScreenOutputConfig *config ); };
struct _IDirectFBDisplayLayer { void *priv; int magic; DFBResult (*AddRef)( IDirectFBDisplayLayer *thiz ); DFBResult (*Release)( IDirectFBDisplayLayer *thiz ); DFBResult (*GetID) ( IDirectFBDisplayLayer *thiz, DFBDisplayLayerID *ret_layer_id ); DFBResult (*GetDescription) ( IDirectFBDisplayLayer *thiz, DFBDisplayLayerDescription *ret_desc ); DFBResult (*GetSourceDescriptions) ( IDirectFBDisplayLayer *thiz, DFBDisplayLayerSourceDescription *ret_descriptions ); DFBResult (*GetCurrentOutputField) ( IDirectFBDisplayLayer *thiz, int *ret_field ); DFBResult (*GetSurface) ( IDirectFBDisplayLayer *thiz, IDirectFBSurface **ret_interface ); DFBResult (*GetScreen) ( IDirectFBDisplayLayer *thiz, IDirectFBScreen **ret_interface ); DFBResult (*SetCooperativeLevel) ( IDirectFBDisplayLayer *thiz, DFBDisplayLayerCooperativeLevel level ); DFBResult (*GetConfiguration) ( IDirectFBDisplayLayer *thiz, DFBDisplayLayerConfig *ret_config ); DFBResult (*TestConfiguration) ( IDirectFBDisplayLayer *thiz, const DFBDisplayLayerConfig *config, DFBDisplayLayerConfigFlags *ret_failed ); DFBResult (*SetConfiguration) ( IDirectFBDisplayLayer *thiz, const DFBDisplayLayerConfig *config ); DFBResult (*SetScreenLocation) ( IDirectFBDisplayLayer *thiz, float x, float y, float width, float height ); DFBResult (*SetScreenPosition) ( IDirectFBDisplayLayer *thiz, int x, int y ); DFBResult (*SetScreenRectangle) ( IDirectFBDisplayLayer *thiz, int x, int y, int width, int height ); DFBResult (*SetOpacity) ( IDirectFBDisplayLayer *thiz, __u8 opacity ); DFBResult (*SetSourceRectangle) ( IDirectFBDisplayLayer *thiz, int x, int y, int width, int height ); DFBResult (*SetFieldParity) ( IDirectFBDisplayLayer *thiz, int field ); DFBResult (*SetSrcColorKey) ( IDirectFBDisplayLayer *thiz, __u8 r, __u8 g, __u8 b ); DFBResult (*SetDstColorKey) ( IDirectFBDisplayLayer *thiz, __u8 r, __u8 g, __u8 b ); DFBResult (*GetLevel) ( IDirectFBDisplayLayer *thiz, int *ret_level ); DFBResult (*SetLevel) ( IDirectFBDisplayLayer *thiz, int level ); DFBResult (*SetBackgroundMode) ( IDirectFBDisplayLayer *thiz, DFBDisplayLayerBackgroundMode mode ); DFBResult (*SetBackgroundImage) ( IDirectFBDisplayLayer *thiz, IDirectFBSurface *surface ); DFBResult (*SetBackgroundColor) ( IDirectFBDisplayLayer *thiz, __u8 r, __u8 g, __u8 b, __u8 a ); DFBResult (*GetColorAdjustment) ( IDirectFBDisplayLayer *thiz, DFBColorAdjustment *ret_adj ); DFBResult (*SetColorAdjustment) ( IDirectFBDisplayLayer *thiz, const DFBColorAdjustment *adj ); DFBResult (*CreateWindow) ( IDirectFBDisplayLayer *thiz, const DFBWindowDescription *desc, IDirectFBWindow **ret_interface ); DFBResult (*GetWindow) ( IDirectFBDisplayLayer *thiz, DFBWindowID window_id, IDirectFBWindow **ret_interface ); DFBResult (*EnableCursor) ( IDirectFBDisplayLayer *thiz, int enable ); DFBResult (*GetCursorPosition) ( IDirectFBDisplayLayer *thiz, int *ret_x, int *ret_y ); DFBResult (*WarpCursor) ( IDirectFBDisplayLayer *thiz, int x, int y ); DFBResult (*SetCursorAcceleration) ( IDirectFBDisplayLayer *thiz, int numerator, int denominator, int threshold ); DFBResult (*SetCursorShape) ( IDirectFBDisplayLayer *thiz, IDirectFBSurface *shape, int hot_x, int hot_y ); DFBResult (*SetCursorOpacity) ( IDirectFBDisplayLayer *thiz, __u8 opacity ); DFBResult (*WaitForSync) ( IDirectFBDisplayLayer *thiz ); };
typedef enum {
     DSFLIP_NONE = 0x00000000,
     DSFLIP_WAIT = 0x00000001,
     DSFLIP_BLIT = 0x00000002,
     DSFLIP_ONSYNC = 0x00000004,
     DSFLIP_PIPELINE = 0x00000008,
     DSFLIP_WAITFORSYNC = DSFLIP_WAIT | DSFLIP_ONSYNC
} DFBSurfaceFlipFlags;
typedef enum {
     DSTF_LEFT = 0x00000000,
     DSTF_CENTER = 0x00000001,
     DSTF_RIGHT = 0x00000002,
     DSTF_TOP = 0x00000004,
     DSTF_BOTTOM = 0x00000008,
     DSTF_TOPLEFT = DSTF_TOP | DSTF_LEFT,
     DSTF_TOPCENTER = DSTF_TOP | DSTF_CENTER,
     DSTF_TOPRIGHT = DSTF_TOP | DSTF_RIGHT,
     DSTF_BOTTOMLEFT = DSTF_BOTTOM | DSTF_LEFT,
     DSTF_BOTTOMCENTER = DSTF_BOTTOM | DSTF_CENTER,
     DSTF_BOTTOMRIGHT = DSTF_BOTTOM | DSTF_RIGHT
} DFBSurfaceTextFlags;
typedef enum {
     DSLF_READ = 0x00000001,
     DSLF_WRITE = 0x00000002
} DFBSurfaceLockFlags;
typedef enum {
     DSPD_NONE = 0,
     DSPD_CLEAR = 1,
     DSPD_SRC = 2,
     DSPD_SRC_OVER = 3,
     DSPD_DST_OVER = 4,
     DSPD_SRC_IN = 5,
     DSPD_DST_IN = 6,
     DSPD_SRC_OUT = 7,
     DSPD_DST_OUT = 8
} DFBSurfacePorterDuffRule;
typedef enum {
     DSBF_ZERO = 1,
     DSBF_ONE = 2,
     DSBF_SRCCOLOR = 3,
     DSBF_INVSRCCOLOR = 4,
     DSBF_SRCALPHA = 5,
     DSBF_INVSRCALPHA = 6,
     DSBF_DESTALPHA = 7,
     DSBF_INVDESTALPHA = 8,
     DSBF_DESTCOLOR = 9,
     DSBF_INVDESTCOLOR = 10,
     DSBF_SRCALPHASAT = 11
} DFBSurfaceBlendFunction;
typedef struct {
     float x;
     float y;
     float z;
     float w;
     float s;
     float t;
} DFBVertex;
typedef enum {
     DTTF_LIST,
     DTTF_STRIP,
     DTTF_FAN
} DFBTriangleFormation;
struct _IDirectFBSurface { void *priv; int magic; DFBResult (*AddRef)( IDirectFBSurface *thiz ); DFBResult (*Release)( IDirectFBSurface *thiz ); DFBResult (*GetCapabilities) ( IDirectFBSurface *thiz, DFBSurfaceCapabilities *ret_caps ); DFBResult (*GetSize) ( IDirectFBSurface *thiz, int *ret_width, int *ret_height ); DFBResult (*GetVisibleRectangle) ( IDirectFBSurface *thiz, DFBRectangle *ret_rect ); DFBResult (*GetPixelFormat) ( IDirectFBSurface *thiz, DFBSurfacePixelFormat *ret_format ); DFBResult (*GetAccelerationMask) ( IDirectFBSurface *thiz, IDirectFBSurface *source, DFBAccelerationMask *ret_mask ); DFBResult (*GetPalette) ( IDirectFBSurface *thiz, IDirectFBPalette **ret_interface ); DFBResult (*SetPalette) ( IDirectFBSurface *thiz, IDirectFBPalette *palette ); DFBResult (*SetAlphaRamp) ( IDirectFBSurface *thiz, __u8 a0, __u8 a1, __u8 a2, __u8 a3 ); DFBResult (*Lock) ( IDirectFBSurface *thiz, DFBSurfaceLockFlags flags, void **ret_ptr, int *ret_pitch ); DFBResult (*Unlock) ( IDirectFBSurface *thiz ); DFBResult (*Flip) ( IDirectFBSurface *thiz, const DFBRegion *region, DFBSurfaceFlipFlags flags ); DFBResult (*SetField) ( IDirectFBSurface *thiz, int field ); DFBResult (*Clear) ( IDirectFBSurface *thiz, __u8 r, __u8 g, __u8 b, __u8 a ); DFBResult (*SetClip) ( IDirectFBSurface *thiz, const DFBRegion *clip ); DFBResult (*SetColor) ( IDirectFBSurface *thiz, __u8 r, __u8 g, __u8 b, __u8 a ); DFBResult (*SetColorIndex) ( IDirectFBSurface *thiz, unsigned int index ); DFBResult (*SetSrcBlendFunction) ( IDirectFBSurface *thiz, DFBSurfaceBlendFunction function ); DFBResult (*SetDstBlendFunction) ( IDirectFBSurface *thiz, DFBSurfaceBlendFunction function ); DFBResult (*SetPorterDuff) ( IDirectFBSurface *thiz, DFBSurfacePorterDuffRule rule ); DFBResult (*SetSrcColorKey) ( IDirectFBSurface *thiz, __u8 r, __u8 g, __u8 b ); DFBResult (*SetSrcColorKeyIndex) ( IDirectFBSurface *thiz, unsigned int index ); DFBResult (*SetDstColorKey) ( IDirectFBSurface *thiz, __u8 r, __u8 g, __u8 b ); DFBResult (*SetDstColorKeyIndex) ( IDirectFBSurface *thiz, unsigned int index ); DFBResult (*SetBlittingFlags) ( IDirectFBSurface *thiz, DFBSurfaceBlittingFlags flags ); DFBResult (*Blit) ( IDirectFBSurface *thiz, IDirectFBSurface *source, const DFBRectangle *source_rect, int x, int y ); DFBResult (*TileBlit) ( IDirectFBSurface *thiz, IDirectFBSurface *source, const DFBRectangle *source_rect, int x, int y ); DFBResult (*BatchBlit) ( IDirectFBSurface *thiz, IDirectFBSurface *source, const DFBRectangle *source_rects, const DFBPoint *dest_points, int num ); DFBResult (*StretchBlit) ( IDirectFBSurface *thiz, IDirectFBSurface *source, const DFBRectangle *source_rect, const DFBRectangle *destination_rect ); DFBResult (*TextureTriangles) ( IDirectFBSurface *thiz, IDirectFBSurface *texture, const DFBVertex *vertices, const int *indices, int num, DFBTriangleFormation formation ); DFBResult (*SetDrawingFlags) ( IDirectFBSurface *thiz, DFBSurfaceDrawingFlags flags ); DFBResult (*FillRectangle) ( IDirectFBSurface *thiz, int x, int y, int w, int h ); DFBResult (*DrawRectangle) ( IDirectFBSurface *thiz, int x, int y, int w, int h ); DFBResult (*DrawLine) ( IDirectFBSurface *thiz, int x1, int y1, int x2, int y2 ); DFBResult (*DrawLines) ( IDirectFBSurface *thiz, const DFBRegion *lines, unsigned int num_lines ); DFBResult (*FillTriangle) ( IDirectFBSurface *thiz, int x1, int y1, int x2, int y2, int x3, int y3 ); DFBResult (*FillRectangles) ( IDirectFBSurface *thiz, const DFBRectangle *rects, unsigned int num ); DFBResult (*FillSpans) ( IDirectFBSurface *thiz, int y, const DFBSpan *spans, unsigned int num ); DFBResult (*SetFont) ( IDirectFBSurface *thiz, IDirectFBFont *font ); DFBResult (*GetFont) ( IDirectFBSurface *thiz, IDirectFBFont **ret_font ); DFBResult (*DrawString) ( IDirectFBSurface *thiz, const char *text, int bytes, int x, int y, DFBSurfaceTextFlags flags ); DFBResult (*DrawGlyph) ( IDirectFBSurface *thiz, unsigned int index, int x, int y, DFBSurfaceTextFlags flags ); DFBResult (*GetSubSurface) ( IDirectFBSurface *thiz, const DFBRectangle *rect, IDirectFBSurface **ret_interface ); DFBResult (*GetGL) ( IDirectFBSurface *thiz, IDirectFBGL **ret_interface ); DFBResult (*Dump) ( IDirectFBSurface *thiz, const char *directory, const char *prefix ); };
struct _IDirectFBPalette { void *priv; int magic; DFBResult (*AddRef)( IDirectFBPalette *thiz ); DFBResult (*Release)( IDirectFBPalette *thiz ); DFBResult (*GetCapabilities) ( IDirectFBPalette *thiz, DFBPaletteCapabilities *ret_caps ); DFBResult (*GetSize) ( IDirectFBPalette *thiz, unsigned int *ret_size ); DFBResult (*SetEntries) ( IDirectFBPalette *thiz, const DFBColor *entries, unsigned int num_entries, unsigned int offset ); DFBResult (*GetEntries) ( IDirectFBPalette *thiz, DFBColor *ret_entries, unsigned int num_entries, unsigned int offset ); DFBResult (*FindBestMatch) ( IDirectFBPalette *thiz, __u8 r, __u8 g, __u8 b, __u8 a, unsigned int *ret_index ); DFBResult (*CreateCopy) ( IDirectFBPalette *thiz, IDirectFBPalette **ret_interface ); };
typedef enum {
     DIKS_UP = 0x00000000,
     DIKS_DOWN = 0x00000001
} DFBInputDeviceKeyState;
typedef enum {
     DIBS_UP = 0x00000000,
     DIBS_DOWN = 0x00000001
} DFBInputDeviceButtonState;
typedef enum {
     DIBM_LEFT = 0x00000001,
     DIBM_RIGHT = 0x00000002,
     DIBM_MIDDLE = 0x00000004
} DFBInputDeviceButtonMask;
typedef enum {
     DIMM_SHIFT = (1 << DIMKI_SHIFT),
     DIMM_CONTROL = (1 << DIMKI_CONTROL),
     DIMM_ALT = (1 << DIMKI_ALT),
     DIMM_ALTGR = (1 << DIMKI_ALTGR),
     DIMM_META = (1 << DIMKI_META),
     DIMM_SUPER = (1 << DIMKI_SUPER),
     DIMM_HYPER = (1 << DIMKI_HYPER)
} DFBInputDeviceModifierMask;
struct _IDirectFBInputDevice { void *priv; int magic; DFBResult (*AddRef)( IDirectFBInputDevice *thiz ); DFBResult (*Release)( IDirectFBInputDevice *thiz ); DFBResult (*GetID) ( IDirectFBInputDevice *thiz, DFBInputDeviceID *ret_device_id ); DFBResult (*GetDescription) ( IDirectFBInputDevice *thiz, DFBInputDeviceDescription *ret_desc ); DFBResult (*GetKeymapEntry) ( IDirectFBInputDevice *thiz, int keycode, DFBInputDeviceKeymapEntry *ret_entry ); DFBResult (*CreateEventBuffer) ( IDirectFBInputDevice *thiz, IDirectFBEventBuffer **ret_buffer ); DFBResult (*AttachEventBuffer) ( IDirectFBInputDevice *thiz, IDirectFBEventBuffer *buffer ); DFBResult (*GetKeyState) ( IDirectFBInputDevice *thiz, DFBInputDeviceKeyIdentifier key_id, DFBInputDeviceKeyState *ret_state ); DFBResult (*GetModifiers) ( IDirectFBInputDevice *thiz, DFBInputDeviceModifierMask *ret_modifiers ); DFBResult (*GetLockState) ( IDirectFBInputDevice *thiz, DFBInputDeviceLockState *ret_locks ); DFBResult (*GetButtons) ( IDirectFBInputDevice *thiz, DFBInputDeviceButtonMask *ret_buttons ); DFBResult (*GetButtonState) ( IDirectFBInputDevice *thiz, DFBInputDeviceButtonIdentifier button, DFBInputDeviceButtonState *ret_state ); DFBResult (*GetAxis) ( IDirectFBInputDevice *thiz, DFBInputDeviceAxisIdentifier axis, int *ret_pos ); DFBResult (*GetXY) ( IDirectFBInputDevice *thiz, int *ret_x, int *ret_y ); };
typedef enum {
     DFEC_NONE = 0x00,
     DFEC_INPUT = 0x01,
     DFEC_WINDOW = 0x02,
     DFEC_USER = 0x03
} DFBEventClass;
typedef enum {
     DIET_UNKNOWN = 0,
     DIET_KEYPRESS,
     DIET_KEYRELEASE,
     DIET_BUTTONPRESS,
     DIET_BUTTONRELEASE,
     DIET_AXISMOTION
} DFBInputEventType;
typedef enum {
     DIEF_NONE = 0x000,
     DIEF_TIMESTAMP = 0x001,
     DIEF_AXISABS = 0x002,
     DIEF_AXISREL = 0x004,
     DIEF_KEYCODE = 0x008,
     DIEF_KEYID = 0x010,
     DIEF_KEYSYMBOL = 0x020,
     DIEF_MODIFIERS = 0x040,
     DIEF_LOCKS = 0x080,
     DIEF_BUTTONS = 0x100,
     DIEF_GLOBAL = 0x200
} DFBInputEventFlags;
typedef struct {
     DFBEventClass clazz;
     DFBInputEventType type;
     DFBInputDeviceID device_id;
     DFBInputEventFlags flags;
     struct timeval timestamp;
     int key_code;
     DFBInputDeviceKeyIdentifier key_id;
     DFBInputDeviceKeySymbol key_symbol;
     DFBInputDeviceModifierMask modifiers;
     DFBInputDeviceLockState locks;
     DFBInputDeviceButtonIdentifier button;
     DFBInputDeviceButtonMask buttons;
     DFBInputDeviceAxisIdentifier axis;
     int axisabs;
     int axisrel;
} DFBInputEvent;
typedef enum {
     DWET_NONE = 0x00000000,
     DWET_POSITION = 0x00000001,
     DWET_SIZE = 0x00000002,
     DWET_CLOSE = 0x00000004,
     DWET_DESTROYED = 0x00000008,
     DWET_GOTFOCUS = 0x00000010,
     DWET_LOSTFOCUS = 0x00000020,
     DWET_KEYDOWN = 0x00000100,
     DWET_KEYUP = 0x00000200,
     DWET_BUTTONDOWN = 0x00010000,
     DWET_BUTTONUP = 0x00020000,
     DWET_MOTION = 0x00040000,
     DWET_ENTER = 0x00080000,
     DWET_LEAVE = 0x00100000,
     DWET_WHEEL = 0x00200000,
     DWET_POSITION_SIZE = DWET_POSITION | DWET_SIZE,
     DWET_ALL = 0x003F033F
} DFBWindowEventType;
typedef struct {
     DFBEventClass clazz;
     DFBWindowEventType type;
     DFBWindowID window_id;
     int x;
     int y;
     int cx;
     int cy;
     int step;
     int w;
     int h;
     int key_code;
     DFBInputDeviceKeyIdentifier key_id;
     DFBInputDeviceKeySymbol key_symbol;
     DFBInputDeviceModifierMask modifiers;
     DFBInputDeviceLockState locks;
     DFBInputDeviceButtonIdentifier button;
     DFBInputDeviceButtonMask buttons;
     struct timeval timestamp;
} DFBWindowEvent;
typedef struct {
     DFBEventClass clazz;
     unsigned int type;
     void *data;
} DFBUserEvent;
typedef union {
     DFBEventClass clazz;
     DFBInputEvent input;
     DFBWindowEvent window;
     DFBUserEvent user;
} DFBEvent;
struct _IDirectFBEventBuffer { void *priv; int magic; DFBResult (*AddRef)( IDirectFBEventBuffer *thiz ); DFBResult (*Release)( IDirectFBEventBuffer *thiz ); DFBResult (*Reset) ( IDirectFBEventBuffer *thiz ); DFBResult (*WaitForEvent) ( IDirectFBEventBuffer *thiz ); DFBResult (*WaitForEventWithTimeout) ( IDirectFBEventBuffer *thiz, unsigned int seconds, unsigned int milli_seconds ); DFBResult (*GetEvent) ( IDirectFBEventBuffer *thiz, DFBEvent *ret_event ); DFBResult (*PeekEvent) ( IDirectFBEventBuffer *thiz, DFBEvent *ret_event ); DFBResult (*HasEvent) ( IDirectFBEventBuffer *thiz ); DFBResult (*PostEvent) ( IDirectFBEventBuffer *thiz, const DFBEvent *event ); DFBResult (*WakeUp) ( IDirectFBEventBuffer *thiz ); DFBResult (*CreateFileDescriptor) ( IDirectFBEventBuffer *thiz, int *ret_fd ); };
typedef enum {
     DWOP_NONE = 0x00000000,
     DWOP_COLORKEYING = 0x00000001,
     DWOP_ALPHACHANNEL = 0x00000002,
     DWOP_OPAQUE_REGION = 0x00000004,
     DWOP_SHAPED = 0x00000008,
     DWOP_KEEP_POSITION = 0x00000010,
     DWOP_KEEP_SIZE = 0x00000020,
     DWOP_KEEP_STACKING = 0x00000040,
     DWOP_GHOST = 0x00001000,
     DWOP_INDESTRUCTIBLE = 0x00002000,
     DWOP_ALL = 0x0000307F
} DFBWindowOptions;
typedef enum {
     DWSC_MIDDLE = 0x00000000,
     DWSC_UPPER = 0x00000001,
     DWSC_LOWER = 0x00000002
} DFBWindowStackingClass;
struct _IDirectFBWindow { void *priv; int magic; DFBResult (*AddRef)( IDirectFBWindow *thiz ); DFBResult (*Release)( IDirectFBWindow *thiz ); DFBResult (*GetID) ( IDirectFBWindow *thiz, DFBWindowID *ret_window_id ); DFBResult (*GetPosition) ( IDirectFBWindow *thiz, int *ret_x, int *ret_y ); DFBResult (*GetSize) ( IDirectFBWindow *thiz, int *ret_width, int *ret_height ); DFBResult (*CreateEventBuffer) ( IDirectFBWindow *thiz, IDirectFBEventBuffer **ret_buffer ); DFBResult (*AttachEventBuffer) ( IDirectFBWindow *thiz, IDirectFBEventBuffer *buffer ); DFBResult (*EnableEvents) ( IDirectFBWindow *thiz, DFBWindowEventType mask ); DFBResult (*DisableEvents) ( IDirectFBWindow *thiz, DFBWindowEventType mask ); DFBResult (*GetSurface) ( IDirectFBWindow *thiz, IDirectFBSurface **ret_surface ); DFBResult (*SetOptions) ( IDirectFBWindow *thiz, DFBWindowOptions options ); DFBResult (*GetOptions) ( IDirectFBWindow *thiz, DFBWindowOptions *ret_options ); DFBResult (*SetColorKey) ( IDirectFBWindow *thiz, __u8 r, __u8 g, __u8 b ); DFBResult (*SetColorKeyIndex) ( IDirectFBWindow *thiz, unsigned int index ); DFBResult (*SetOpacity) ( IDirectFBWindow *thiz, __u8 opacity ); DFBResult (*SetOpaqueRegion) ( IDirectFBWindow *thiz, int x1, int y1, int x2, int y2 ); DFBResult (*GetOpacity) ( IDirectFBWindow *thiz, __u8 *ret_opacity ); DFBResult (*SetCursorShape) ( IDirectFBWindow *thiz, IDirectFBSurface *shape, int hot_x, int hot_y ); DFBResult (*RequestFocus) ( IDirectFBWindow *thiz ); DFBResult (*GrabKeyboard) ( IDirectFBWindow *thiz ); DFBResult (*UngrabKeyboard) ( IDirectFBWindow *thiz ); DFBResult (*GrabPointer) ( IDirectFBWindow *thiz ); DFBResult (*UngrabPointer) ( IDirectFBWindow *thiz ); DFBResult (*GrabKey) ( IDirectFBWindow *thiz, DFBInputDeviceKeySymbol symbol, DFBInputDeviceModifierMask modifiers ); DFBResult (*UngrabKey) ( IDirectFBWindow *thiz, DFBInputDeviceKeySymbol symbol, DFBInputDeviceModifierMask modifiers ); DFBResult (*Move) ( IDirectFBWindow *thiz, int dx, int dy ); DFBResult (*MoveTo) ( IDirectFBWindow *thiz, int x, int y ); DFBResult (*Resize) ( IDirectFBWindow *thiz, int width, int height ); DFBResult (*SetStackingClass) ( IDirectFBWindow *thiz, DFBWindowStackingClass stacking_class ); DFBResult (*Raise) ( IDirectFBWindow *thiz ); DFBResult (*Lower) ( IDirectFBWindow *thiz ); DFBResult (*RaiseToTop) ( IDirectFBWindow *thiz ); DFBResult (*LowerToBottom) ( IDirectFBWindow *thiz ); DFBResult (*PutAtop) ( IDirectFBWindow *thiz, IDirectFBWindow *lower ); DFBResult (*PutBelow) ( IDirectFBWindow *thiz, IDirectFBWindow *upper ); DFBResult (*Close) ( IDirectFBWindow *thiz ); DFBResult (*Destroy) ( IDirectFBWindow *thiz ); };
struct _IDirectFBFont { void *priv; int magic; DFBResult (*AddRef)( IDirectFBFont *thiz ); DFBResult (*Release)( IDirectFBFont *thiz ); DFBResult (*GetAscender) ( IDirectFBFont *thiz, int *ret_ascender ); DFBResult (*GetDescender) ( IDirectFBFont *thiz, int *ret_descender ); DFBResult (*GetHeight) ( IDirectFBFont *thiz, int *ret_height ); DFBResult (*GetMaxAdvance) ( IDirectFBFont *thiz, int *ret_maxadvance ); DFBResult (*GetKerning) ( IDirectFBFont *thiz, unsigned int prev_index, unsigned int current_index, int *ret_kern_x, int *ret_kern_y ); DFBResult (*GetStringWidth) ( IDirectFBFont *thiz, const char *text, int bytes, int *ret_width ); DFBResult (*GetStringExtents) ( IDirectFBFont *thiz, const char *text, int bytes, DFBRectangle *ret_logical_rect, DFBRectangle *ret_ink_rect ); DFBResult (*GetGlyphExtents) ( IDirectFBFont *thiz, unsigned int index, DFBRectangle *ret_rect, int *ret_advance ); };
typedef enum {
     DICAPS_NONE = 0x00000000,
     DICAPS_ALPHACHANNEL = 0x00000001,
     DICAPS_COLORKEY = 0x00000002
} DFBImageCapabilities;
typedef struct {
     DFBImageCapabilities caps;
     __u8 colorkey_r;
     __u8 colorkey_g;
     __u8 colorkey_b;
} DFBImageDescription;
typedef void (*DIRenderCallback)(DFBRectangle *rect, void *ctx);
struct _IDirectFBImageProvider { void *priv; int magic; DFBResult (*AddRef)( IDirectFBImageProvider *thiz ); DFBResult (*Release)( IDirectFBImageProvider *thiz ); DFBResult (*GetSurfaceDescription) ( IDirectFBImageProvider *thiz, DFBSurfaceDescription *ret_dsc ); DFBResult (*GetImageDescription) ( IDirectFBImageProvider *thiz, DFBImageDescription *ret_dsc ); DFBResult (*RenderTo) ( IDirectFBImageProvider *thiz, IDirectFBSurface *destination, const DFBRectangle *destination_rect ); DFBResult (*SetRenderCallback) ( IDirectFBImageProvider *thiz, DIRenderCallback callback, void *callback_data ); };
typedef int (*DVFrameCallback)(void *ctx);
struct _IDirectFBVideoProvider { void *priv; int magic; DFBResult (*AddRef)( IDirectFBVideoProvider *thiz ); DFBResult (*Release)( IDirectFBVideoProvider *thiz ); DFBResult (*GetCapabilities) ( IDirectFBVideoProvider *thiz, DFBVideoProviderCapabilities *ret_caps ); DFBResult (*GetSurfaceDescription) ( IDirectFBVideoProvider *thiz, DFBSurfaceDescription *ret_dsc ); DFBResult (*PlayTo) ( IDirectFBVideoProvider *thiz, IDirectFBSurface *destination, const DFBRectangle *destination_rect, DVFrameCallback callback, void *ctx ); DFBResult (*Stop) ( IDirectFBVideoProvider *thiz ); DFBResult (*SeekTo) ( IDirectFBVideoProvider *thiz, double seconds ); DFBResult (*GetPos) ( IDirectFBVideoProvider *thiz, double *ret_seconds ); DFBResult (*GetLength) ( IDirectFBVideoProvider *thiz, double *ret_seconds ); DFBResult (*GetColorAdjustment) ( IDirectFBVideoProvider *thiz, DFBColorAdjustment *ret_adj ); DFBResult (*SetColorAdjustment) ( IDirectFBVideoProvider *thiz, const DFBColorAdjustment *adj ); };
struct _IDirectFBDataBuffer { void *priv; int magic; DFBResult (*AddRef)( IDirectFBDataBuffer *thiz ); DFBResult (*Release)( IDirectFBDataBuffer *thiz ); DFBResult (*Flush) ( IDirectFBDataBuffer *thiz ); DFBResult (*SeekTo) ( IDirectFBDataBuffer *thiz, unsigned int offset ); DFBResult (*GetPosition) ( IDirectFBDataBuffer *thiz, unsigned int *ret_offset ); DFBResult (*GetLength) ( IDirectFBDataBuffer *thiz, unsigned int *ret_length ); DFBResult (*WaitForData) ( IDirectFBDataBuffer *thiz, unsigned int length ); DFBResult (*WaitForDataWithTimeout) ( IDirectFBDataBuffer *thiz, unsigned int length, unsigned int seconds, unsigned int milli_seconds ); DFBResult (*GetData) ( IDirectFBDataBuffer *thiz, unsigned int length, void *ret_data, unsigned int *ret_read ); DFBResult (*PeekData) ( IDirectFBDataBuffer *thiz, unsigned int length, int offset, void *ret_data, unsigned int *ret_read ); DFBResult (*HasData) ( IDirectFBDataBuffer *thiz ); DFBResult (*PutData) ( IDirectFBDataBuffer *thiz, const void *data, unsigned int length ); DFBResult (*CreateImageProvider) ( IDirectFBDataBuffer *thiz, IDirectFBImageProvider **interface ); };
