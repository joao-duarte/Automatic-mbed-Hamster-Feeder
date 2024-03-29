
/*
Copyright (c) 2010 Donatien Garnier (donatiengar [at] gmail [dot] com)
 
Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:
 
The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.
 
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/

#ifndef GPRSMODULENETIF_H
#define GPRSMODULENETIF_H

#include "mbed.h"

#include "core/net.h"
#include "if/ppp/PPPNetIf.h"

#include "drv/gprs/GPRSModem.h"

class GPRSModuleNetIf : protected PPPNetIf
{
public:
  GPRSModuleNetIf(PinName tx, PinName rx, int baud = 115200); 
  virtual ~GPRSModuleNetIf();
  
  PPPErr connect(const char* apn = NULL, const char* userId = NULL, const char* password = NULL); //Connect using GPRS
  PPPErr disconnect();
  
protected:
  virtual bool setOn() = 0; //True on success
  virtual bool setOff() = 0; //True on success
  
private:
  Serial m_serial;

};

#endif

