#ifndef __I2C_DEF_H__
#define __I2C_DEF_H__

/* the following are a workaround for possible loader bug.. 
   WATCH function types ! */
#if XFree86LOADER

#define CreateI2CBusRec    ((pointer (*)(void))LoaderSymbol("xf86CreateI2CBusRec"))
#define DestroyI2CBusRec   ((pointer (*)(I2CBusPtr, Bool, Bool))LoaderSymbol("xf86DestroyI2CBusRec"))
#define I2CBusInit         ((Bool (*)(pointer))LoaderSymbol("xf86I2CBusInit"))
#define I2C_WriteRead      ((Bool (*)(I2CDevPtr, I2CByte *, int, I2CByte *, int))LoaderSymbol("xf86I2CWriteRead"))
#define CreateI2CDevRec    ((pointer (*)(void))LoaderSymbol("xf86CreateI2CDevRec"))
#define I2CDevInit         ((Bool (*)(I2CDevPtr))LoaderSymbol("xf86I2CDevInit"))
#define I2CProbeAddress    ((Bool (*)(I2CBusPtr,I2CSlaveAddr))LoaderSymbol("xf86I2CProbeAddress"))

#else

#define CreateI2CBusRec    xf86CreateI2CBusRec
#define DestroyI2CBusRec   xf86DestroyI2CBusRec
#define I2CBusInit         xf86I2CBusInit
#define I2C_WriteRead      xf86I2CWriteRead
#define CreateI2CDevRec    xf86CreateI2CDevRec
#define I2CDevInit         xf86I2CDevInit 
#define I2CProbeAddress    xf86I2CProbeAddress

#endif


#endif
