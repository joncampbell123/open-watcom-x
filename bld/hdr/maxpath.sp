:: In this include file all file/path names length limit macros are defined
::
:: FILENAME_MAX macro
::
:segment FILENAME
::#ifndef FILENAME_MAX
:segment LINUX | QNX
#define FILENAME_MAX    255
:elsesegment
:segment RDOS
 #define FILENAME_MAX   260
:elsesegment
#if defined(__OS2__) || defined(__NT__) || defined(__WATCOM_LFN__) && defined(__DOS__) || defined(__OSI__)
 #define FILENAME_MAX   260
#else
 #define FILENAME_MAX   144
#endif
:endsegment
:endsegment
::#endif
:elsesegment NAMEMAX
::
:: NAME_MAX macro
::
:segment DOS | RDOS
#ifndef NAME_MAX
:segment RDOS
  #define NAME_MAX    255     /* maximum filename for HPFS and RDOS */
:elsesegment
 #if defined(__OS2__)
  #define NAME_MAX    255     /* maximum filename for HPFS and RDOS */
 #elif defined(__NT__) || defined(__WATCOM_LFN__) && defined(__DOS__) || defined(__OSI__)
  #define NAME_MAX    259     /* maximum filename for NTFS, FAT LFN, DOS LFN and OSI */
 #else
  #define NAME_MAX    12      /* 8 chars + '.' +  3 chars */
 #endif
:endsegment
#endif
:endsegment
:elsesegment PATHMAX
::
:: PATH_MAX macro
::
:segment DOS | RDOS
:include ext.sp
::#ifndef PATH_MAX
:segment RDOS
  #define PATH_MAX      259 /* maximum length of full pathname excl. '\0' */
:elsesegment
 #if defined(__OS2__) || defined(__NT__) || defined(__WATCOM_LFN__) && defined(__DOS__) || defined(__OSI__)
  #define PATH_MAX      259 /* maximum length of full pathname excl. '\0' */
 #else
  #define PATH_MAX      143 /* maximum length of full pathname excl. '\0' */
 #endif
:endsegment
::#endif
:include extepi.sp
:endsegment
:elsesegment
::
:: _MAX_PATH macro
::
#ifndef _MAX_PATH
:segment LINUX | QNX
 #define _MAX_PATH    256 /* maximum length of full pathname */
:elsesegment
 #if defined(__OS2__) || defined(__NT__) || defined(__WATCOM_LFN__) && defined(__DOS__) || defined(__OSI__)
  #define _MAX_PATH   260 /* maximum length of full pathname */
 #else
  #define _MAX_PATH   144 /* maximum length of full pathname */
 #endif
:endsegment
#endif
:endsegment
