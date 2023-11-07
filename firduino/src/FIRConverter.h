#pragma once
#include "AudioTools/AudioTypes.h"
#include "AudioBasic/Collections.h"
#include "AudioFilter/Filter.h"

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct fir_f32_s {
    float  *coeffs;     /*!< Pointer to the coefficient buffer.*/
    float  *delay;      /*!< Pointer to the delay line buffer.*/
    int     N;          /*!< FIR filter coefficients amount.*/
    int     pos;        /*!< Position in delay line.*/
    int     decim;      /*!< Decimation factor.*/
    int     d_pos;      /*!< Actual decimation counter.*/
} fir_f32_t;

int dsps_fir_f32_ae32(fir_f32_t* fir, const float* input, float* output, int len);

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
class FIRConverter : public BaseConverter<T> {
 public:

  FIRConverter( float* coeffsLeft, float* coeffsRight, int firLen ) {

    this->channels = 2;
    this->direction = 1;
    this->gain = 1;
    this->correction = 1;

    float* delayLeft = (float*)calloc( firLen, sizeof(float) );
    float* delayRight = (float*)calloc( firLen, sizeof(float) );

    fir_init( &firLeft, coeffsLeft, delayLeft, firLen );
    fir_init( &firRight, coeffsRight, delayRight, firLen );

    int n = DEFAULT_BUFFER_SIZE;

    srcLeft = (float*)calloc( n, sizeof(float) );
    srcRight = (float*)calloc( n, sizeof(float) );
	destLeft = (float*)calloc( n, sizeof(float) );
	destRight = (float*)calloc( n, sizeof(float) );

  }

  void setDirection( int direction ) {
	  this->direction = direction;
  }

  void setGain( int gain ) {
	  this->gain = gain;
  }

  void setCorrection( float correction ) {
	  this->correction = correction;
  }

  float getCorrection() { return this->correction; }
	  
  ~FIRConverter() {
  }

  void fir_init( fir_f32_t *fir, float *coeffs, float *delay, int N )
  {
      fir->coeffs = coeffs;
      fir->delay = delay;
      fir->N = N;
      fir->pos = 0;

      for (int i = 0 ; i < N; i++) {
          fir->delay[i] = 0;
      }
  }

  void fir(fir_f32_t *fir, float *input, float *output, int len)
  {
      for (int i = 0 ; i < len ; i++) {
          float acc = 0;
          int coeff_pos = fir->N - 1;
          fir->delay[fir->pos] = input[i];
          fir->pos++;
          if (fir->pos >= fir->N) {
              fir->pos = 0;
          }
          for (int n = fir->pos; n < fir->N ; n++) {
              acc += fir->coeffs[coeff_pos--] * fir->delay[n];
          }
          for (int n = 0; n < fir->pos ; n++) {
              acc += fir->coeffs[coeff_pos--] * fir->delay[n];
          }
          output[i] = acc;
      }
  }

  size_t convert(uint8_t *src, size_t size) {

    int count = size / channels / sizeof(T);
    T *sample = (T *)src;

	getSrc( sample, count );
	
    dsps_fir_f32_ae32(&firLeft, srcLeft, destLeft, count);
    dsps_fir_f32_ae32(&firRight, srcRight, destRight, count);

    write( sample, count );

    return size;
  }

  virtual void getSrc( T* sample, int count )
  {
    for ( int i = 0 ; i < count ; i++ )
    {
    	srcLeft[i] = sample[i*2] * correction;
    	srcRight[i] = sample[i*2+1];
    }
  }

  virtual void write( T* sample, int count )
  {
    for ( int i = 0 ; i < count ; i++ )
    {
    	sample[i*2] = gain * destLeft[i];
    	sample[i*2+1] = gain * destRight[i];
    }
  }


 protected:
  int direction;
  int gain;
  int channels;
  float correction;

  float* 	coeffsLeft;
  float* 	coeffsRight;
  int		firLen;

  float*	delayLeft;
  float*	delayRight;

  float*	srcLeft;
  float*	srcRight;
  float*	destLeft;
  float*	destRight;

  fir_f32_t firLeft;
  fir_f32_t firRight;

  int loop = 0;
};

template <typename T>
class FIRAddConverter : public FIRConverter<T> {

public:
	  FIRAddConverter( float* coeffsLeft, float* coeffsRight, int firLen ) :
	  	  FIRConverter<T>( coeffsLeft, coeffsRight, firLen )
	  {}

	  virtual void write( T* sample, int count )
	  {
	    for ( int i = 0 ; i < count ; i++ )
	    {
	    	T out = this->gain * ( this->destLeft[i] + this->direction * this->destRight[i] );

	    	sample[i*2] = out;
	    	sample[i*2+1] = out;
	    }
	  }
};

template <typename T>
class FIRDemConverter : public FIRConverter<T> {

/*// vrinem za FM
	#define __UA   256
inline int16_t _arctan3(int16_t q, int16_t i)
     {
#define __atan2(z)  (__UA/8  + __UA/22) * z  // very much of a simplification...not accurate at all, but fast
	//#define __atan2(z)  (__UA/8 - __UA/22 * z + __UA/22) * z  //derived from (5) [1]
	int16_t r;
	if (abs(q) > abs(i))
		r = __UA / 4 - __atan2(abs(i) / abs(q));        // arctan(z) = 90-arctan(1/z)
	else
		r = (i == 0) ? 0 : __atan2(abs(q) / abs(i));   // arctan(z)
	r = (i < 0) ? __UA / 2 - r : r;                  // arctan(-z) = -arctan(z)
	return (q < 0) ? -r : r;                        // arctan(-z) = -arctan(z)
     }	
	// konec vrinka*/

public:
	  FIRDemConverter( float* coeffsLeft, float* coeffsRight, int firLen ) :
	  	  FIRConverter<T>( coeffsLeft, coeffsRight, firLen )
	  {}

	  virtual void write( T* sample, int count )
	  {
		if(this->direction == 1)
		{  
	    for ( int i = 0 ; i < count ; i++ )
	      {
	    	T out = this->gain * ( this->destLeft[i] + this->destRight[i] );

	    	sample[i*2] = out;
	    	sample[i*2+1] = out;
	      }
		}
		
		if(this->direction == 2)
		{  
	    for ( int i = 0 ; i < count ; i++ )
	      {
	    	T out = this->gain * ( this->destLeft[i] - this->destRight[i] );

	    	sample[i*2] = out;
	    	sample[i*2+1] = out;
	      }
		}
		
		if(this->direction == 3)
		{  
	    for ( int i = 0 ; i < count ; i++ )
	      {
	    	T out = this->gain * 2 * sqrt( this->destLeft[i] * this->destLeft[i] + this->destRight[i] * this->destRight[i]);
			
			/*static int16_t as_last;   // GW8RDI mod - replaced LP filter: DC removal done in sdr_rx()
		    int16_t as = out + (int16_t)((float)as_last * 0.9999f); // Reduce from 0.9999f for less bass response
		    out = as - as_last;
		    as_last = as;*/
			
			static int16_t wold;   // prenesel iz Twnsy convolution SDR
		    int16_t w = out + wold * 0.9999f; // yes, I want a superb bass response
		    this->destLeft[i] = w - wold;  // to je v input audiosamplih levega kanala ???
            wold = w;
		   
			sample[i*2] = out;
	    	sample[i*2+1] = out;
						
	      }
		}
		
		if(this->direction == 4)
			
		// Perform FM demodulation		
		{  
	    for ( int i = 0 ; i < count ; i++ )
	      { static float prevPhase = 0.0;
			float currentPhase = atan2(this->destLeft[i], this->destRight[i]);
            float phaseDiff = currentPhase - prevPhase;  
			  
	    // Adjust the phase difference to ensure it stays within a reasonable range
        if (phaseDiff > M_PI) {
        phaseDiff -= 2 * M_PI;
        } else if (phaseDiff < -M_PI) {
        phaseDiff += 2 * M_PI;
        }
        T out =  (int16_t)(phaseDiff * 10000); // You can adjust the scaling factor
		
		// Update the previous phase
        prevPhase = currentPhase;
			  
	    	/*// G8RDI mod - enabled FM differentiator
			int16_t z0 = _arctan3(this->destLeft[i], this->destRight[i]);
			static int16_t z1 = z0; // G8RDI mod - initialised static
			T out = z0 - z1; // Differentiator
			z1 = z0;*/
		   
			sample[i*2] = out;
	    	sample[i*2+1] = out;
						
	      }
		}
			
	  }
	  
};

template <typename T>
class FIRSplitterConverter : public FIRConverter<T> {

public:
	  FIRSplitterConverter( float* coeffsLeft, float* coeffsRight, int firLen, bool src ) :
	  	  FIRConverter<T>( coeffsLeft, coeffsRight, firLen )
	  {
		  this->srcIsLeft = src;
	  }

  virtual void getSrc( T* sample, int count )
  {
    for ( int i = 0 ; i < count ; i++ )
    {
		T src = srcIsLeft ? sample[i*2] : sample[i*2+1];
    	this->srcLeft[i] = src * this->getCorrection();
    	this->srcRight[i] = src;
    }
  }

private:
	bool srcIsLeft;

};


}
