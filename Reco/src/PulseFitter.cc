#include <HGCal/Reco/interface/PulseFitter.h>

#include <limits>

#include <Math/Minimizer.h>
#include <Math/Factory.h>
#include <Math/Functor.h>

double _time[7],_energy[7];double _maxTime=225.; //seems to be mandatory since we need static function
double _alpha=10.;
double _trise=50.;
double _noise=8.;
double pulseShape_fcn(double t, double tmax, double amp)
{
  if( t>tmax-_trise ) return amp*std::pow( (t-(tmax-_trise))/_trise,_alpha )*std::exp(-_alpha*(t-tmax)/_trise);
  else return 0;
}
double pulseShape_chi2(const double *x)
{
  double sum = 0.0;
  for(size_t i=0; i<7; i++){
    if( _energy[i]<0 || _time[i]>_maxTime ) continue;
    double zero = _energy[i]-pulseShape_fcn( _time[i],
					     x[0],x[1] );
    sum += zero * zero / _noise / _noise;
  }
  return sum;
}

PulseFitter::PulseFitter( int printLevel, double maxTime , double alpha , double trise ) : m_printLevel(printLevel)
{							     
  _maxTime=maxTime;
  _alpha=alpha;
  _trise=trise;
}

void PulseFitter::run(std::vector<double> &time, std::vector<double> &energy, PulseFitterResult &fit, double noise)
{
  if( time.size()!=energy.size() ){
    std::cout << "ERROR : we should have the same vector size in PulseFitter::run(std::vector<double> time, std::vector<double> energy, PulseFitterResult fit) -> return without fitting" << std::endl;
    return;
  }
  if( time.size()!=11 ){
    std::cout << "ERROR : we should have less than 13 time sample in PulseFitter::run(std::vector<double> time, std::vector<double> energy, PulseFitterResult fit) -> return without fitting" << std::endl;
    return;
  }
  for( uint16_t i=0; i<7; i++ ){
    _time[i] = time[i];
    _energy[i] = energy[i];
  }

  if( noise>0 )
    _noise=noise;
  
  ROOT::Math::Minimizer* m = ROOT::Math::Factory::CreateMinimizer("Minuit2", "Migrad");
  m->SetMaxFunctionCalls(m_fitterParameter.nMaxIterations);
  m->SetMaxIterations(m_fitterParameter.nMaxIterations);
  m->SetTolerance(0.001);
  m->SetPrintLevel(m_printLevel);
  ROOT::Math::Functor f(&pulseShape_chi2, 2);

  m->SetFunction(f);

  m->Clear(); // just a precaution

  m->SetVariable(0, "tmax", m_fitterParameter.tmax0, 0.001);
  m->SetVariableLimits(0,
		       m_fitterParameter.tmaxRangeDown,
		       m_fitterParameter.tmaxRangeUp);
  m->SetVariable(1, "amp", _energy[3], 0.001);
  m->SetVariableLimits(1,0,10000);

  m->Minimize();

  const double *xm = m->X();
  const double *errors = m->Errors();
  fit.tmax=xm[0];
  fit.trise=_trise;
  fit.amplitude=xm[1];
  fit.errortmax=errors[0];
  fit.erroramplitude=errors[1];
  fit.chi2=m->MinValue();
  fit.status=(fabs(xm[0]-m_fitterParameter.tmaxRangeDown)>std::numeric_limits<double>::epsilon()&&
	      fabs(xm[0]-m_fitterParameter.tmaxRangeUp)>std::numeric_limits<double>::epsilon()) ? m->Status() : 6;
  fit.ncalls=m->NCalls();
}
