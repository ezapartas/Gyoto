/*
    Copyright 2011 Thibaut Paumard, Frederic Vincent

    This file is part of Gyoto.

    Gyoto is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Gyoto is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Gyoto.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "GyotoUtils.h"
#include "GyotoUniformSphere.h"
#include "GyotoPhoton.h"
#include "GyotoPowerLawSpectrum.h"
#include "GyotoBlackBodySpectrum.h"
#include "GyotoFactoryMessenger.h"
#include "GyotoConverters.h"

#include <iostream>
#include <cmath>
#include <string>
#include <cstdlib>
#include <float.h>
#include <sstream>
#include <string.h>

using namespace std;
using namespace Gyoto;
using namespace Gyoto::Astrobj;

UniformSphere::UniformSphere(string kind) :
  Astrobj::Standard(kind),
  spectrum_(NULL),
  opacity_(NULL)
{
# if GYOTO_DEBUG_ENABLED
  GYOTO_DEBUG << endl;
# endif

  setRadius(0.);

  spectrum_ = new Spectrum::BlackBody(); 
  opacity_ = new Spectrum::PowerLaw(0., 1.); 
}

UniformSphere::UniformSphere(string kind,
			     SmartPointer<Metric::Generic> met, double rad) :
  Astrobj::Standard(kind),
  radius_(rad),
  spectrum_(NULL), opacity_(NULL)
{
  critical_value_ = radius_*radius_;
  safety_value_ = critical_value_*1.1 + 0.1;
  spectrum_ = new Spectrum::BlackBody(); 
  opacity_ = new Spectrum::PowerLaw(0., 1.); 

  gg_=met;

}

UniformSphere::UniformSphere(const UniformSphere& orig) :
  Astrobj::Standard(orig),
  radius_(orig.radius_),
  spectrum_(NULL), opacity_(NULL)
{
# if GYOTO_DEBUG_ENABLED
  GYOTO_DEBUG << endl;
# endif
  if (orig.spectrum_()) spectrum_=orig.spectrum_->clone();
  if (orig.opacity_()) opacity_=orig.opacity_->clone();
}

UniformSphere::~UniformSphere() {
# if GYOTO_DEBUG_ENABLED
  GYOTO_DEBUG << endl;
# endif
}

string UniformSphere::className() const { return  string("UniformSphere"); }
string UniformSphere::className_l() const { return  string("uniformsphere"); }

SmartPointer<Spectrum::Generic> UniformSphere::getSpectrum() const { return spectrum_; }
void UniformSphere::setSpectrum(SmartPointer<Spectrum::Generic> sp) {spectrum_=sp;}

SmartPointer<Spectrum::Generic> UniformSphere::getOpacity() const { return opacity_; }
void UniformSphere::setOpacity(SmartPointer<Spectrum::Generic> sp) {opacity_=sp;}


double UniformSphere::operator()(double const coord[4]) {
  double coord_st[4] = {coord[0]};
  double coord_ph[4] = {coord[0]};
  double sintheta;
  getCartesian(coord_st, 1, coord_st+1, coord_st+2, coord_st+3);
  switch (gg_->getCoordKind()) {
  case GYOTO_COORDKIND_CARTESIAN:
    memcpy(coord_ph+1, coord+1, 3*sizeof(double));
    break;
  case GYOTO_COORDKIND_SPHERICAL:
    coord_ph[1] = coord[1] * (sintheta=sin(coord[2])) * cos(coord[3]);
    coord_ph[2] = coord[1] * sintheta * sin(coord[3]);
    coord_ph[3] = coord[1] * cos(coord[2]) ;
    break;
  default:
    throwError("unsupported coordkind");
  }
  double dx = coord_ph[1]-coord_st[1];
  double dy = coord_ph[2]-coord_st[2];
  double dz = coord_ph[3]-coord_st[3];

  return dx*dx + dy*dy + dz*dz;
}


double UniformSphere::emission(double nu_em, double dsem, double *, double *) const {
  if (flag_radtransf_) return (*spectrum_)(nu_em, (*opacity_)(nu_em), dsem);
  return (*spectrum_)(nu_em);
}

double UniformSphere::transmission(double nuem, double dsem, double*) const {
  if (!flag_radtransf_) return 0.;
  double opacity = (*opacity_)(nuem);

# if GYOTO_DEBUG_ENABLED
  GYOTO_DEBUG <<  "(nuem="    << nuem
	      << ", dsem="    << dsem
	      << "), opacity=" << opacity << endl;
# endif

  if (!opacity) return 1.;
  return exp(-opacity*dsem);
}

double UniformSphere::integrateEmission(double nu1, double nu2, double dsem,
			       double *, double *) const {
  if (flag_radtransf_)
    return spectrum_->integrate(nu1, nu2, opacity_(), dsem);
  return spectrum_->integrate(nu1, nu2);
}


double UniformSphere::getRadius() const {
  return radius_;
}

void UniformSphere::setRadius(double r) {
  radius_=r;
  critical_value_ = r*r;
  safety_value_ = critical_value_*1.1+0.1;
}

double UniformSphere::getRadius(std::string unit) const {
  return Units::FromGeometrical(getRadius(), unit, gg_);
}

void UniformSphere::setRadius(double r, std::string unit) {
  setRadius(Units::ToGeometrical(r, unit, gg_));
}

int UniformSphere::setParameter(string name, string content, string unit) {
  if (name=="Radius") setRadius(atof(content.c_str()), unit);
  else return Standard::setParameter(name, content, unit);
  return 0;
}

#ifdef GYOTO_USE_XERCES
void UniformSphere::fillElement(FactoryMessenger *fmp) const {
  FactoryMessenger * childfmp=NULL;

  fmp -> setMetric (getMetric()) ;
  fmp -> setParameter ("Radius", getRadius());

  childfmp = fmp -> makeChild ( "Spectrum" );
  spectrum_ -> fillElement(childfmp);
  delete childfmp;

  childfmp = fmp -> makeChild ( "Opacity" );
  opacity_ -> fillElement(childfmp);
  delete childfmp;

  Astrobj::Generic::fillElement(fmp);
}

void Gyoto::Astrobj::UniformSphere::setParameters(FactoryMessenger* fmp){
  setFlag_radtransf(0);
  if (!fmp) return;

  string name="", content="", unit="";
  FactoryMessenger * child = NULL;

# if GYOTO_DEBUG_ENABLED
  GYOTO_DEBUG << "setMetric()" << endl;
# endif
  setMetric(fmp->getMetric());

  while (fmp->getNextParameter(&name, &content, &unit)) {
    if (name=="Spectrum") {
      content = fmp -> getAttribute("kind");
      child = fmp -> getChild();
#     if GYOTO_DEBUG_ENABLED
      GYOTO_DEBUG << "setSpectrum()" << endl;
#     endif
      setSpectrum((*Spectrum::getSubcontractor(content))(child));
      delete child;
    }
    else if (name=="Opacity") {
      content = fmp -> getAttribute("kind");
      child = fmp -> getChild();
#     if GYOTO_DEBUG_ENABLED
      GYOTO_DEBUG << "setOpacity()" << endl;
#     endif
      setOpacity((*Spectrum::getSubcontractor(content))(child));
      setFlag_radtransf(1);
      delete child;
    } else {
#     if GYOTO_DEBUG_ENABLED
      GYOTO_DEBUG << "setParameter("<<name<<", "<<content<<")\n";
#     endif
      setParameter(name, content, unit);
    }
  }

}

#endif
