// ATtiny167 doesn't have quite the same UART as most people are 
// expecting, instead it is part of the LIN/UART (see datasheet
// chapter 15) so this is a crummy fillin for the usual debugging
// gumpf, configures pin A1 as TX at 38400 baud and logs messages
// to it.

#ifdef DEBUG
#define DEBUG_INIT() \
    do { \
	DDRA = (1 << DDA1); \
	LINCR = (1 << LENA) | (1 << LCMD2) | (1 << LCMD0); \
	LINBTR = 8; \
	LINBRR = 12; \
    } while (0)

#define DEBUG_STR(buf) \
    for ( char *x = buf; *x; x++) { \
       	LINDAT = *x; \
	while (!(LINSIR & (1 << LTXOK))); \
    }

#define DEBUG_BYTES(buf, len) \
    for (int i = 0; i<len; i++) { \
	LINDAT = "0123456789ABCDEF"[buf[i]>>4]; \
	while (!(LINSIR & (1 << LTXOK))); \
	LINDAT = "0123456789ABCDEF"[buf[i]&15]; \
	while (!(LINSIR & (1 << LTXOK))); \
	LINDAT = (i == len-1) ? '\n' : ' '; \
	while (!(LINSIR & (1 << LTXOK))); \
    }


#else 

#define DEBUG_INIT() do { } while (0)
#define DEBUG_STR(buf) do { } while (0)
#define DEBUG_BYTES(buf, len) do { } while (0)

#endif

