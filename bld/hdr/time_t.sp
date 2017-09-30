:segment CNAME
#ifndef _STDTIME_T_DEFINED
#define _STDTIME_T_DEFINED
 namespace std {
:segment QNX | LINUX
   typedef signed long time_t;
:elsesegment
   typedef unsigned long time_t;
:endsegment
 }
 typedef std::time_t __w_time_t;
#endif
:elsesegment
:segment !CONLY
#ifdef __cplusplus
 #ifndef _STDTIME_T_DEFINED
 #define _STDTIME_T_DEFINED
  namespace std {
:segment QNX | LINUX
    typedef signed long time_t;
:elsesegment
    typedef unsigned long time_t;
:endsegment
  }
  typedef std::time_t __w_time_t;
 #endif
 #ifndef _TIME_T_DEFINED
 #define _TIME_T_DEFINED
  #define _TIME_T_DEFINED_
  using std::time_t;
 #endif
#else  /* __cplusplus not defined */
:endsegment
 #ifndef _TIME_T_DEFINED
 #define _TIME_T_DEFINED
  #define _TIME_T_DEFINED_
:segment QNX | LINUX
  typedef signed long time_t;
:elsesegment
  typedef unsigned long time_t;
:endsegment
  typedef time_t __w_time_t;
 #endif
:segment !CONLY
#endif /* __cplusplus not defined */
:endsegment
:endsegment
