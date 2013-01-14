#include "CI2C.h"
#include "CLogging.h"

class Ctmp100
{
  public:
    Ctmp100(CI2C *i2c, uint8_t address);
    ~Ctmp100();
    
    bool set_12bit_resolution();
    bool get_temperature(double *temperature);
    
  private:
    CI2C *_i2c_device;
    uint8_t _i2c_address;

};
