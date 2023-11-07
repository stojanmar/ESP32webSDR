#pragma once
#include "AudioTools/AudioTypes.h"
#include "AudioBasic/Collections.h"
#include "AudioFilter/Filter.h"

#ifdef __cplusplus
extern "C"
{
#endif

#ifdef __cplusplus
}
#endif


namespace audio_tools {



/**
 * @brief Converter for n Channels which applies the indicated Filter
 * @author pschatzmann
 * @tparam T
 */

template <typename T>
class MixerConverter : public BaseConverter<T> {
 public:

  MixerConverter()
  {
	  this->channels = 2;
	  int n = DEFAULT_BUFFER_SIZE;

	  srcLeft = (float*)calloc( n, sizeof(float) );
	  srcRight = (float*)calloc( n, sizeof(float) );
  }

  ~MixerConverter()
  {
	  delete srcLeft;
	  delete srcRight;
  }


  size_t convert(uint8_t *src, size_t size) {

    int count = size / channels / sizeof(T);
    T *sample = (T *)src;

    for ( int i = 0 ; i < count ; i++ )
    {
    	int mult1 = ( i / 2 ) % 2 == 0 ? 1 : -1;
    	int mult2 = ( (i+1) / 2 ) % 2 == 0 ? 1 : -1;

    	srcLeft[i] = mult1 * sample[i*2];
    	srcRight[i] = mult2 * sample[i*2+1];
    }

    write( sample, count );

    return size;
  }

  virtual void write( T* sample, int count )
  {
    for ( int i = 0 ; i < count ; i++ )
    {
    	sample[i*2] = srcLeft[i];
    	sample[i*2+1] = srcRight[i];
    }
  }


 protected:
  float*	srcLeft;
  float*	srcRight;

  int		channels;

  int cnt = 0;
};




}
