/*
    Copyright 2011, 2013 Thibaut Paumard, Frederic Vincent

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
#include "GyotoFactoryMessenger.h"
#include "GyotoScreen.h"
#include "GyotoConverters.h"

#include <iostream>
#include <cstdlib>
#include <cmath>
#include <string>
#include <sstream>

using namespace std ; 
using namespace Gyoto;

// Default constructor
Screen::Screen() : 
  //tobs_(0.), fov_(M_PI*0.1), tmin_(0.), npix_(1001),
  tobs_(0.), fov_(M_PI*0.1), npix_(1001),
  alpha0_(0.), delta0_(0.),
  anglekind_(0),
  distance_(1.), dmax_(GYOTO_SCREEN_DMAX), gg_(NULL), spectro_(NULL),
  freq_obs_(1.)
{
# if GYOTO_DEBUG_ENABLED
  GYOTO_DEBUG_EXPR(dmax_);
# endif
  euler_[0]=euler_[1]=euler_[2]=0.;
  setProjection(0., 0., 0.);
  for (int ii=0;ii<4;ii++) {
    fourvel_[ii]=0.;
    screen1_[ii]=0.;
    screen2_[ii]=0.;
    screen3_[ii]=0.;
  }
}

Screen::Screen(const Screen& o) :
  SmartPointee(o),
  tobs_(o.tobs_), fov_(o.fov_), npix_(o.npix_), distance_(o.distance_),
  alpha0_(o.alpha0_), delta0_(o.delta0_),
  anglekind_(o.anglekind_),
  dmax_(o.dmax_), gg_(NULL), spectro_(NULL), freq_obs_(o.freq_obs_)
{
  if (o.gg_()) gg_=o.gg_->clone();
  if (o.spectro_()) spectro_ = o.spectro_ -> clone();
  int i;
  for (i=0; i<3; ++i) {
    euler_[i] = o.euler_[i];
    ex_[i]=o.ex_[i];
    ey_[i]=o.ey_[i];
    ez_[i]=o.ez_[i];
  }
  for (int ii=0;ii<4;ii++) {
    fourvel_[ii]=o.fourvel_[ii];
    screen1_[ii]=o.screen1_[ii];
    screen2_[ii]=o.screen2_[ii];
    screen3_[ii]=o.screen3_[ii];
  }
}
Screen * Screen::clone() const { return new Screen(*this); }

Screen::~Screen(){}

std::ostream& Screen::print( std::ostream& o) const {
  o << "distance="    << distance_ << ", " ;
  o << "paln="        << euler_[0] << ", " ;
  o << "inclination=" << euler_[1] << ", " ;
  o << "argument="    << euler_[2] << "." << endl;
  return o;
}

/***************Definition of the physical scene**************/

void Screen::setProjection(const double paln,
			   const double incl,
			   const double arg) {
  euler_[0]=paln;
  euler_[1]=incl;
  euler_[2]=arg;
  computeBaseVectors();
}

void Screen::setProjection(const double dist,
			   const double paln,
			   const double incl,
			   const double arg) {
  distance_=dist;
  setProjection(paln, incl, arg);
}

void Screen::setDistance(double dist, const string unit)    {
  setDistance(Units::ToMeters(dist, unit, gg_));
}
void Screen::setDistance(double dist)    {
  distance_=dist;
  computeBaseVectors();
}

void Screen::setDmax(double dist) {
  dmax_ = dist;
# if GYOTO_DEBUG_ENABLED
  GYOTO_DEBUG_EXPR(dmax_);
# endif
}
double Screen::getDmax() const { return dmax_; }


void Screen::setPALN(double paln, const string &unit)        {
  if ((unit=="") || (unit=="rad"));
# ifdef HAVE_UDUNITS
  else paln = Units::Converter(unit, "rad")(paln);
# else
  else if (unit=="degree" || unit=="°") paln *= GYOTO_DEGRAD;
# endif
  setPALN(paln);
}
void Screen::setPALN(double paln)        {
  euler_[0]=paln; computeBaseVectors();
}

void Screen::setInclination(double incl, const string &unit) { 
  if ((unit=="") || (unit=="rad"));
# ifdef HAVE_UDUNITS
  else incl = Units::Converter(unit, "rad")(incl);
# else
  else if (unit=="degree" || unit=="°") incl *= GYOTO_DEGRAD;
# endif
  setInclination(incl);
}
void Screen::setInclination(double incl) {
  euler_[1]=incl; computeBaseVectors();
}

void Screen::setArgument(double arg, const string &unit) { 
  if (unit=="" || unit=="rad");
# ifdef HAVE_UDUNITS
  else arg = Units::Converter(unit, "rad")(arg);
# else
  else if (unit=="degree" || unit=="°") arg *= GYOTO_DEGRAD;
# endif
  setArgument(arg);
}
void Screen::setArgument(double arg)     {
  euler_[2]=arg;  computeBaseVectors();
}

void Screen::setFreqObs(double fo, const string &unit) { 
  setFreqObs(Units::ToHerz(fo, unit));
}
void Screen::setFreqObs(double fo)     {
  freq_obs_=fo;
  GYOTO_DEBUG_EXPR(freq_obs_);
}
double Screen::getFreqObs() const {
  return freq_obs_;
}
double Screen::getFreqObs(const string &unit) const {
  return Units::FromHerz(freq_obs_, unit);
}


void Screen::setMetric(SmartPointer<Metric::Generic> gg) { gg_ = gg; computeBaseVectors(); }

int Screen::getCoordKind()      const { return gg_ -> getCoordKind(); }
double Screen::getDistance()    const { return distance_; }
double Screen::getDistance(const std::string& unit)    const {
  return Units::FromMeters(getDistance(), unit, gg_); 
}
double Screen::getPALN()        const { return euler_[0]; }
double Screen::getPALN(const string &unit) const {
  double paln = getPALN();
  if (unit != "" && unit != "rad" ) {
# ifdef HAVE_UDUNITS
    paln = Units::Converter(unit, "rad")(paln);
#else
    GYOTO_WARNING << "unit ignored, please recompile with --with-udunits\n";
#endif
  }
  return paln;
}

double Screen::getInclination() const { return euler_[1]; }
double Screen::getInclination(const string &unit) const {
  double incl = getInclination();
  if (unit != "" && unit != "rad" ) {
# ifdef HAVE_UDUNITS
    incl = Units::Converter(unit, "rad")(incl);
#else
    GYOTO_WARNING << "unit ignored, please recompile with --with-udunits\n";
#endif
  }
  return incl;
}

double Screen::getArgument()    const { return euler_[2]; }
double Screen::getArgument(const string &unit) const {
  double arg = getArgument();
  if (unit != "" && unit != "rad" ) {
# ifdef HAVE_UDUNITS
    arg = Units::Converter(unit, "rad")(arg);
#else
    GYOTO_WARNING << "unit ignored, please recompile with --with-udunits\n";
#endif
  }
  return arg;
}

SmartPointer<Metric::Generic> Screen::getMetric() const { return gg_; }

void Screen::setObserverPos(const double coord[4]) {
  tobs_ = coord[0] * gg_ -> unitLength() / GYOTO_C;
  euler_[0]=M_PI;//Par défaut, A CHANGER SI BESOIN
  //dans le cas standard ex=-ephi et ephi dirige la ligne des noeuds d'où le pi
  //NB: c'est -pi dans mes notes, donc OK [2pi]
  //NB : ne décrit que la rotation de la caméra dans le plan x,y

  int coordkind=gg_ -> getCoordKind();
  switch (coordkind) {
  case GYOTO_COORDKIND_SPHERICAL:
    distance_=coord[1]* gg_ -> unitLength();
    //distance_=coord[1]; DEBUG
    euler_[1]=M_PI-coord[2];
    euler_[2]=-M_PI/2-coord[3];
    break;
  case GYOTO_COORDKIND_CARTESIAN:
    {
      double rks=sqrt(coord[1]*coord[1]+coord[2]*coord[2]+coord[3]*coord[3]);
      double thetaks=acos(coord[3]/rks);
      double phiks=atan2(coord[2],coord[1]);
      distance_=rks * gg_ -> unitLength();
      euler_[1]=M_PI-thetaks;
      euler_[2]=-M_PI/2-phiks;
    }
    break;
  default:
    throwError("Incompatible coordinate kind in Screen::setObserverCoord");
  }
  computeBaseVectors();
}

void Screen::setFourVel(const double coord[4]) {
  for (int ii=0;ii<4;ii++)
    fourvel_[ii]=coord[ii];
}

void Screen::setScreen1(const double coord[4]) {
  for (int ii=0;ii<4;ii++)
    screen1_[ii]=coord[ii];
}

void Screen::setScreen2(const double coord[4]) {
  for (int ii=0;ii<4;ii++)
    screen2_[ii]=coord[ii];
}

void Screen::setScreen3(const double coord[4]) {
  for (int ii=0;ii<4;ii++)
    screen3_[ii]=coord[ii];
}

void Screen::getObserverPos(double coord[]) const
{
  double r0     = distance_ / gg_ -> unitLength();
  //remark : Pb here if mass=0 (unitLength=0...) -> pb for flat metric

  //if (debug()) cout << "distance_ in screen= " << distance_ << endl;
  //double r0     = distance_ ;DEBUG
  double theta0 = M_PI-euler_[1];
  double phi0 = -M_PI/2-euler_[2];
    
  int coordkind = gg_ -> getCoordKind();

  stringstream ss;

  switch (coordkind) {
  case GYOTO_COORDKIND_CARTESIAN:
    coord[0] = tobs_ * GYOTO_C / gg_ -> unitLength();
    coord[1] = r0*cos(phi0)*sin(theta0);
    coord[2] = r0*sin(phi0)*sin(theta0);
    coord[3] = r0*cos(theta0);
    break;
  case GYOTO_COORDKIND_SPHERICAL:
    coord[0] = tobs_ * GYOTO_C / gg_ -> unitLength();
    coord[1] = r0;
    coord[2] = theta0;
    coord[3] = phi0;
    break;
  default:
    ss << "Incompatible coordinate kind in Screen::getObserverPos: "
       << coordkind;
    throwError(ss.str());
  }
}

void Screen::getFourVel(double fourvel[]) const{
  for (int ii=0;ii<4;ii++) fourvel[ii]=fourvel_[ii];
}

void Screen::getScreen1(double output[]) const{
  for (int ii=0;ii<4;ii++) output[ii]=screen1_[ii];
}

void Screen::getScreen2(double output[]) const{
  for (int ii=0;ii<4;ii++) output[ii]=screen2_[ii];
}

void Screen::getScreen3(double output[]) const{
  for (int ii=0;ii<4;ii++) output[ii]=screen3_[ii];
}

/* SPECTROMETER */

void Screen::setSpectrometer(SmartPointer<Spectrometer::Generic> spr) { spectro_=spr; }
SmartPointer<Spectrometer::Generic> Screen::getSpectrometer() const { return spectro_; }

void Screen::getRayCoord(const size_t i, const size_t j, double coord[]) const {
  const double delta= fov_/double(npix_);
  double xscr, yscr;
# if GYOTO_DEBUG_ENABLED
  GYOTO_DEBUG << "(i=" << i << ", j=" << j << ", coord)" << endl;
# endif
  if (anglekind_){
    /*
      GYOTO screen labelled by spherical
      angles a and b (see Fig. in user guide)
     */
    xscr = double(i-1)*fov_/(2.*double(npix_-1));
    yscr = double(j-1)*2.*M_PI/double(npix_-1);
    getRayCoord(xscr, yscr, coord);
  }else{
    /*
      GYOTO screen labelled by equatorial
      angles alpha and delta (see Fig. in user guide)
     */
    yscr=delta*(double(j)-double(npix_+1)/2.);
    xscr=delta*(double(i)-double(npix_+1)/2.);
    getRayCoord(-xscr, yscr, coord); // -xscr to have East on the left
  }
}

void Screen::getRayCoord(double alpha, double delta,
		      double coord[]) const

{
  alpha+=alpha0_; delta+=delta0_; // Screen orientation
  
  int i; // dimension : 0, 1, 2
  double pos[4];
# if GYOTO_DEBUG_ENABLED
  GYOTO_DEBUG << "(alpha="<<alpha<<",delta="<<delta<<",coord)" << endl;
# endif
  getObserverPos(coord);

  if (coord[1] > dmax_) {
    // scale
    coord[0] -= coord[1] - dmax_;
    double scale = coord[1] / dmax_;
    coord[1] = dmax_;
    alpha *= scale;
    delta *= scale;
  }

  coord[4]=coord[5]=coord[6]=coord[7]=0.;//initializing 4-velocity

  double spherical_angle_a,
    spherical_angle_b;
  
  if (anglekind_){
    /*
      GYOTO screen labelled by spherical
      angles a and b (see Fig. in user guide)
     */

    spherical_angle_a = alpha;
    spherical_angle_b = delta;
  }else{
    /*
      GYOTO screen labelled by equatorial
      angles alpha and delta (see Fig. in user guide)
      Must compute spherical angles a and b
      from these.
     */
    
    /*
      NB: spherical_angles = spherical angles associated with the
      orthonormal 3-frame of the observer's local rest space
    */
    
    /*
      NBB: for spherical_angle_a, the relation comes from the spherical
      law of cosines of spherical trigonometry, the arcs
      spherical_angle_a, alpha and delta forming a spherical triangle,
      with alpha othogonal to delta.
      NBBB: for spherical_angle_b, the relation also comes from the spherical
      law of cosine. 
      --> Following transformations are OK even for non-small alpha, delta
    */
    
    spherical_angle_a = acos(cos(alpha)*cos(delta));
    spherical_angle_b = 
      (alpha==0. && delta==0.) ? 0. : atan2(tan(delta),sin(alpha));
    
    // Move these two angles to [0,pi], [0,2pi]
    double s1tmp=spherical_angle_a, s2tmp=spherical_angle_b;
    while (s1tmp>M_PI) s1tmp-=2.*M_PI;
    while (s1tmp<-M_PI) s1tmp+=2.*M_PI;//then s1 in [-pi,pi]
    if (s1tmp<0.) {
      s1tmp=-s1tmp;//then s1 in [0,pi]
      s2tmp+=M_PI;//thus, same direction
    }
    while (s2tmp>2.*M_PI) s2tmp-=2.*M_PI;
    while (s2tmp<0.) s2tmp+=2.*M_PI;//then s2 in [0,2pi]
    spherical_angle_a=s1tmp;
    spherical_angle_b=s2tmp;
  }

  /* 
     Tangent vector of incident photon in observer's local frame
     vel is thus orthogonal to observer's 4-velocity
     vel = vel[0]*screen1_+vel[1]*screen2_+vel[2]*screen3_
     where screen1,2,3_ is a triad in observer's rest space
  */

  //NB: following minus signs because the photon doesn't leave the screen, but
  //is heading towards it!
  double vel[3]={-sin(spherical_angle_a)*cos(spherical_angle_b),
		 -sin(spherical_angle_a)*sin(spherical_angle_b),
		 -cos(spherical_angle_a)};
  
  // 4-vector tangent to photon geodesic
  
  if (fourvel_[0]==0.){
    /* 
       ---> Observer local frame not given in XML <---
       Assume observer static at infinity ("standard Gyoto")
       Treatment depending on coordinate system
    */
    switch (gg_ -> getCoordKind()) {
    case GYOTO_COORDKIND_CARTESIAN:
      {
	double rr=coord[1]*coord[1]+
	  coord[2]*coord[2]+
	  coord[3]*coord[3],
	  rinf=20.; // this rinf is just a very crude test
               	    // I take rinf=10*rhor in Sch metric
	if (rr<rinf)
	  throwError("In Screen::getRayCoord: "
		     "observer is not at spatial infinity "
		     "and should be");

	//Transforming to KS:
	for (i=0;i<3;++i) {
	  coord[5]+=vel[i]*ex_[i];//xdot0
	  coord[6]+=vel[i]*ey_[i];//ydot0
	  coord[7]+=vel[i]*ez_[i];//zdot0
	}
	
      }
      break;
    case GYOTO_COORDKIND_SPHERICAL:
      {
	if (coord[2]==0. || coord[2]==M_PI)
	  throwError("Please move Screen away from z-axis");

	double rr=coord[1],
	  rinf=20.; // this rinf is just a very crude test
               	    // I take rinf=10*rhor in Sch metric
	if (rr<rinf)
	  throwError("In Screen::getRayCoord: "
		     "observer is not at spatial infinity "
		     "and should be");

	pos[0]=coord[0];pos[1]=coord[1];pos[2]=coord[2];pos[3]=coord[3];
	double grr=gg_->gmunu(pos,1,1), 
	  gthth=gg_->gmunu(pos,2,2), gphph=gg_->gmunu(pos,3,3);
	coord[5]=-vel[2]/sqrt(grr);
	double sp=sin(euler_[0]);
	double cp=cos(euler_[0]);
	coord[6]=(-sp*vel[0]+cp*vel[1])/sqrt(gthth);
	coord[7]=( cp*vel[0]+sp*vel[1])/sqrt(gphph);
      }
      break;
    default:
      throwError("Incompatible coordinate kind in Screen::getRayCoord()");
      break;
    }

    // 0-component of photon tangent 4-vector found by normalizing
    gg_ -> nullifyCoord(coord);
    
  }else{
    /* 
       ---> Observer local frame given in XML <---
       Express photon tangent 4-vector in the observer basis
       Treatment is coordinate independent 
       (except for z-axis check right below)
    */

    if (gg_ -> getCoordKind() == GYOTO_COORDKIND_SPHERICAL){
      if (coord[2]==0. || coord[2]==M_PI)
	throwError("Please move Screen away from z-axis");
    }
    
    //Check orthonormal basis
    double normtol=1e-10;
    if (fabs(gg_->ScalarProd(coord,fourvel_,fourvel_)+1.)>normtol ||
	fabs(gg_->ScalarProd(coord,screen1_,screen1_)-1.)>normtol ||
	fabs(gg_->ScalarProd(coord,screen2_,screen2_)-1.)>normtol ||
	fabs(gg_->ScalarProd(coord,screen3_,screen3_)-1.)>normtol){
      throwError("In Screen:getRayCoord: observer's local"
		 " basis is not properly normalized");
    }
    
    if (fabs(gg_->ScalarProd(coord,fourvel_,screen1_))>normtol ||
	fabs(gg_->ScalarProd(coord,fourvel_,screen2_))>normtol ||
	fabs(gg_->ScalarProd(coord,fourvel_,screen3_))>normtol ||
	fabs(gg_->ScalarProd(coord,screen1_,screen2_))>normtol ||
	fabs(gg_->ScalarProd(coord,screen1_,screen3_))>normtol ||
	fabs(gg_->ScalarProd(coord,screen2_,screen3_))>normtol)
      throwError("In Screen:getRayCoord: observer's local"
		 " basis is not orthogonal");
    
    /* Photon tagent 4-vector l defined by: l = p + fourvel_
       where p gives the direction of the photon
       in the observer's rest space (orthogonal to fourvel).
       Here we choose the particular tangent 4-vector l
       that satisfies l.fourvel_=-1
       Then l = fourvel_ + (orthogonal proj of l onto rest space)
       = fourvel_ + p
       and p = vel[0]*screen1_ + vel[1]*screen2_ + vel[2]*screen3_ 
       with p.p = 1, thus l.l = 0 as it should
    */
    coord[4]=vel[0]*screen1_[0]
      +vel[1]*screen2_[0]
      +vel[2]*screen3_[0]
      +fourvel_[0];
    coord[5]=vel[0]*screen1_[1]
      +vel[1]*screen2_[1]
      +vel[2]*screen3_[1]
      +fourvel_[1];
    coord[6]=vel[0]*screen1_[2]
      +vel[1]*screen2_[2]
      +vel[2]*screen3_[2]
      +fourvel_[2];
    coord[7]=vel[0]*screen1_[3]
      +vel[1]*screen2_[3]
      +vel[2]*screen3_[3]
      +fourvel_[3];
    if (fabs(gg_->ScalarProd(coord,coord+4,coord+4))>normtol){
      throwError("In Screen::getRayCoord: "
		 " tangent 4-vector to photon not properly normalized");
    }
    
  }
  
}

void Screen::computeBaseVectors() {
  // See http://en.wikipedia.org/wiki/Euler_angles

  //  if (debug()) cout << "In compute base vectore" << endl;
  double ca, sa; sincos(euler_[0], &sa, &ca); // paln
  double cb, sb; sincos(euler_[1], &sb, &cb); // inclination
  double cc, sc; sincos(euler_[2], &sc, &cc); // argument
  double unit  = 1.;//1./distance_;

  //NB: here x,y,z are KS coordinates
  ex_[0] = unit*( ca*cc - sa*cb*sc);//component of KS vector eX along observer East direction ex
  ex_[1] = unit*( sa*cc + ca*cb*sc);//component of KS vector eX along observer North direction ey
  ex_[2] = unit*( sb*sc);//etc...
  ey_[0] = unit*(-ca*sc - sa*cb*cc);
  ey_[1] = unit*(-sa*sc + ca*cb*cc);
  ey_[2] = unit*( sb*cc);
  ez_[0] = unit*( sb*sa);
  ez_[1] = unit*(-sb*ca);
  ez_[2] = unit*( cb);

}

void Screen::coordToSky(const double pos[4], double skypos[3]) const {
  double xyz[3];
  coordToXYZ(pos, xyz);
  double ul = gg_ -> unitLength();

  skypos[0]=(xyz[0]*ex_[0]+xyz[1]*ey_[0]+xyz[2]*ez_[0]) * ul;
  skypos[1]=(xyz[0]*ex_[1]+xyz[1]*ey_[1]+xyz[2]*ez_[1]) * ul;
  skypos[2]=(xyz[0]*ex_[2]+xyz[1]*ey_[2]+xyz[2]*ez_[2]) * ul;
}

std::ostream & Screen::printBaseVectors(std::ostream &o) const {
 
  o << endl;
  o << setprecision(3) << setw(8) << ex_[0] << ", "
    << setprecision(3) << setw(8) << ey_[0] << ", "
    << setprecision(3) << setw(8) << ez_[0] << endl;
  o << setprecision(3) << setw(8) << ex_[1] << ", "
    << setprecision(3) << setw(8) << ey_[1] << ", "
    << setprecision(3) << setw(8) << ez_[1] << endl;
  o << setprecision(3) << setw(8) << ex_[2] << ", "
    << setprecision(3) << setw(8) << ey_[2] << ", "
    << setprecision(3) << setw(8) << ez_[2] << endl;
  return o ;
} 


/***************Useful functions**************/


void Screen::coordToXYZ(const double pos[4], double xyz[3]) const {
  int coordkind = gg_ -> getCoordKind();
  switch(coordkind) {
  case GYOTO_COORDKIND_SPHERICAL:
    xyz[0] = pos[1]*sin(pos[2])*cos(pos[3]);
    xyz[1] = pos[1]*sin(pos[2])*sin(pos[3]);
    xyz[2] = pos[1]*cos(pos[2]);
    break;
  case GYOTO_COORDKIND_CARTESIAN:
    xyz[0] = pos[1];
    xyz[1] = pos[2];
    xyz[2] = pos[3];
    break;
  default:
    throwError("Incompatible coordinate kind in Screen::coordToXYZ");
    break;
  }
}

double Screen::getTime() { return tobs_ ; }
double Screen::getTime(const string &unit) {
  return Units::FromSeconds(getTime(), unit, gg_) ;
}

void Screen::setTime(double tobs, const string &unit) {
# ifdef GYOTO_DEBUG_ENABLED
  GYOTO_DEBUG_EXPR(tobs);
  GYOTO_DEBUG_EXPR(unit);
# endif
  setTime(Units::ToSeconds(tobs, unit, gg_));
}
void Screen::setTime(double tobs) {
  tobs_ = tobs;
# ifdef GYOTO_DEBUG_ENABLED
  GYOTO_DEBUG_EXPR(tobs_);
# endif
}

//double Screen::getMinimumTime() { return tmin_; }
//void Screen::setMinimumTime(double tmin) { tmin_ = tmin; }
double Screen::getFieldOfView() { return fov_; }

double Screen::getFieldOfView(string unit) {
  double fov = getFieldOfView();
  if (unit=="" || unit=="rad") ;
  else if (unit=="geometrical") fov *= distance_ / gg_ -> unitLength();
# ifdef HAVE_UDUNITS
  else if (Units::areConvertible(unit, "m"))
    fov = Units::FromMeters(fov*distance_, unit) ;
  else fov = Units::Converter("rad", unit)(fov);
# else
  else if (unit=="degree") fov /= GYOTO_DEGRAD;
  else if (unit=="arcmin") fov /= GYOTO_MINRAD;
  else if (unit=="arcsec") fov /= GYOTO_SECRAD;
  else if (unit=="milliarcsec") fov /= GYOTO_MASRAD;
  else if (unit=="microarcsec") fov /= GYOTO_MUASRAD;
  else {
    stringstream ss;
    ss << "Screen::setFieldOfView(): unknown unit: \"" << unit << "\""
       << " (you may have more chance compiling gyoto with --with-udunits)";
    throwError(ss.str());
  }
# endif
  return fov;
}

void Screen::setFieldOfView(double fov, const string &unit) {
  if (unit=="" || unit=="rad") ;
  else if (unit=="geometrical") fov *= gg_ -> unitLength() / distance_ ;
# ifdef HAVE_UDUNITS
  else {
    Units::Unit from (unit);
    if (Units::areConvertible(from, "m"))
      fov = Units::ToMeters(fov, from) / distance_;
    else fov = Units::Converter(from, "rad")(fov);
  }
# else
  else if (unit=="degree" || unit=="°")        fov *= GYOTO_DEGRAD;
  else if (unit=="arcmin")                     fov *= GYOTO_MINRAD;
  else if (unit=="arcsec" || unit=="as")       fov *= GYOTO_SECRAD;
  else if (unit=="milliarcsec" || unit=="mas") fov *= GYOTO_MASRAD;
  else if (unit=="microarcsec" || unit=="µas" || unit=="uas")
                                               fov *= GYOTO_MUASRAD;
  else {
    stringstream ss;
    ss << "Screen::setFieldOfView(): unknown unit: \"" << unit << "\""
       << " (you may have more chance compiling gyoto with --with-udunits)";
    throwError(ss.str());
  }
# endif
  setFieldOfView(fov);
}
void Screen::setFieldOfView(double fov) { fov_ = fov; }

void Screen::setAlpha0(double alpha) { alpha0_ = alpha; }
void Screen::setDelta0(double delta) { delta0_ = delta; }

void Screen::setAnglekind(int kind) { anglekind_ = kind; }

size_t Screen::getResolution() { return npix_; }
void Screen::setResolution(size_t n) { npix_ = n; }

#ifdef HAVE_UDUNITS
void Gyoto::Screen::mapPixUnit() {
  Units::Unit radian ("radian");
  double delta = fov_/double(npix_);
  ut_unit * pix = ut_scale(delta, radian);
  ut_status status = UT_BAD_ARG;
  status = ut_map_name_to_unit("pixel", UT_ASCII, pix);
  switch (status) {
  case UT_SUCCESS:
    break;
  case UT_EXISTS:
    throwError("name \"pixel\" already registered");
    break;
  default:
    throwError("error initializing \"pixel\" unit");
  }
  status = ut_map_symbol_to_unit("pix", UT_ASCII, pix);
  switch (status) {
  case UT_SUCCESS:
    break;
  case UT_EXISTS:
    throwError("symbol \"pix\" already registered");
    break;
  default:
    throwError("error initializing \"pixel\" unit");
  }
  ut_free(pix);
# if GYOTO_DEBUG_ENABLED
  GYOTO_DEBUG << "\"pix\" unit mapped\n";
# endif
}
void Gyoto::Screen::unmapPixUnit() {
  ut_system * sys = Units::getSystem();
  if ((ut_unmap_name_to_unit(sys, "pixel", UT_ASCII) != UT_SUCCESS) ||
      (ut_unmap_symbol_to_unit(sys, "pix", UT_ASCII) != UT_SUCCESS))
    throwError("Error unmapping \"pixel\" unit");
# if GYOTO_DEBUG_ENABLED
  GYOTO_DEBUG << "\"pix\" unit unmapped\n";
# endif
}
#endif

#ifdef GYOTO_USE_XERCES
void Screen::fillElement(FactoryMessenger *fmp) {
  FactoryMessenger* child = NULL;
  if (gg_) fmp -> setMetric (gg_) ;
  fmp -> setParameter ("Time", tobs_);
  fmp -> setParameter ("FieldOfView", fov_);
  fmp -> setParameter ("Alpha0", alpha0_);
  fmp -> setParameter ("Delta0", delta0_);
  fmp -> setParameter ("Resolution", npix_);
  double d = getDistance();
  if (gg_() && (gg_->getMass() == 1.)) {
    d /=  gg_->unitLength();
    fmp -> setParameter ("Distance", &d, 1, &child);
    child -> setSelfAttribute("unit", "geometrical");
  } else {
    string unit = "m";
    if (     d >= GYOTO_KPC*1e3   ) { d /= (1000.*GYOTO_KPC); unit = "Mpc"; }
    else if (d >= GYOTO_KPC       ) { d /= GYOTO_KPC;         unit = "kpc"; }
    else if (d >= GYOTO_KPC*1e-3  ) { d /= (GYOTO_KPC*1e-3);  unit = "pc"; }
    else if (d >= GYOTO_LIGHT_YEAR) { d /= GYOTO_LIGHT_YEAR;  unit = "ly"; }
    else if (d >= GYOTO_ASTRONOMICAL_UNIT)
                                  { d /= GYOTO_ASTRONOMICAL_UNIT; unit = "AU"; }
    else if (d >= GYOTO_SUN_RADIUS)
                                  { d /= GYOTO_SUN_RADIUS;  unit = "sunradius"; }
    else if (d >= 1e3)              { d *= 1e-3;              unit = "km";}
    else if (d >= 1.) ;//           { d *= 1.;                unit = "m";}
    else if (d >= 1e-2)             { d *= 1e2;               unit = "cm";}
    //else ;                        { d *= 1.;                unit = "m";}
    fmp -> setParameter ("Distance", &d, 1, &child);
    child -> setSelfAttribute("unit", unit);
  }
  if (dmax_ != GYOTO_SCREEN_DMAX) child -> setSelfAttribute("dmax", dmax_);
  delete child; child = NULL;
  fmp -> setParameter ("PALN", getPALN());
  fmp -> setParameter ("Inclination", getInclination());
  fmp -> setParameter ("Argument", getArgument());
  if (spectro_) {
    child = fmp -> makeChild("Spectrometer");
    spectro_ -> fillElement(child) ;
    delete child; child = NULL;
  }
  fmp -> setParameter ("FreqObs", freq_obs_);
  fmp -> setParameter ( anglekind_? "SphericalAngles" : "EquatorialAngles");
}

SmartPointer<Screen> Screen::Subcontractor(FactoryMessenger* fmp) {
  string name="", content="", unit="", tunit="";
  SmartPointer<Screen> scr = new Screen();
  scr -> setMetric(fmp->getMetric());
  int tobs_found=0;
  double tobs_tmp, pos[4] ;
  char * tc;

  // Deal with fov later as we need Inclination
  double fov; string fov_unit; int fov_found=0;
  double alpha0; int alpha0_found=0; 
  double delta0; int delta0_found=0;

  int fourvel_found=0, screen1_found=0,
    screen2_found=0, screen3_found=0;

  while (fmp->getNextParameter(&name, &content, &unit)) {
    tc = const_cast<char*>(content.c_str());
#   ifdef GYOTO_DEBUG_ENABLED
    GYOTO_IF_DEBUG
    GYOTO_DEBUG_EXPR(name);
    GYOTO_DEBUG_EXPR(content);
    GYOTO_DEBUG_EXPR(unit);
    GYOTO_ENDIF_DEBUG
#   endif
    if      (name=="Time")     {tobs_tmp = atof(tc); tunit=unit; tobs_found=1;}
    else if (name=="Position") {for (int i=0;i<4;++i) pos[i] = strtod(tc, &tc);
                                  scr -> setObserverPos (pos); }
    else if (name=="FourVelocity") {
      fourvel_found=1;
      for (int i=0;i<4;++i) pos[i] = strtod(tc, &tc);
      scr -> setFourVel (pos); 
    }
    else if (name=="ScreenVector1") {
      screen1_found=1;
      for (int i=0;i<4;++i) pos[i] = strtod(tc, &tc);
      scr -> setScreen1 (pos); 
    }
    else if (name=="ScreenVector2") {
      screen2_found=1;
      for (int i=0;i<4;++i) pos[i] = strtod(tc, &tc);
      scr -> setScreen2 (pos); 
    }
    else if (name=="ScreenVector3") {
      screen3_found=1;
      for (int i=0;i<4;++i) pos[i] = strtod(tc, &tc);
      scr -> setScreen3 (pos); 
    }
    else if (name=="Distance")    
      {
	scr -> setDistance    ( atof(tc), unit );
	string dmax = fmp -> getAttribute("dmax");
	if (dmax != "") scr -> setDmax(atof(dmax.c_str()));
      }
    else if (name=="PALN")        scr -> setPALN        ( atof(tc), unit );
    else if (name=="Inclination") scr -> setInclination ( atof(tc), unit );
    else if (name=="Argument")    scr -> setArgument    ( atof(tc), unit );
    else if (name=="FieldOfView") {
      fov = atof(tc); fov_unit=unit; fov_found=1;
    }
    else if (name=="Resolution")  scr -> setResolution  ( atoi(tc) );
    else if (name=="Spectrometer") {
      scr -> setSpectrometer ((Spectrometer::getSubcontractor(fmp->getAttribute("kind")))(fmp->getChild()));
    }
    else if (name=="Alpha0"){
      alpha0 = atof(tc); alpha0_found=1;
    }
    else if (name=="Delta0"){
      delta0 = atof(tc); delta0_found=1;
    }
    else if (name=="FreqObs") scr -> setFreqObs(atof(tc), unit);
    else if (name=="SphericalAngles")  scr -> setAnglekind(1);
    else if (name=="EquatorialAngles") scr -> setAnglekind(0);
  }

  if (tobs_found) scr -> setTime ( tobs_tmp, tunit );

  if (fov_found) scr -> setFieldOfView ( fov, fov_unit );

  if (alpha0_found) scr -> setAlpha0(alpha0);

  if (delta0_found) scr -> setDelta0(delta0);

  int sum_found=fourvel_found+screen1_found
    +screen2_found+screen3_found;

  if (sum_found>0 && sum_found<4)
    throwError("In Screen::subContractor: either all four, or no "
	       "basis vectors should be given");


# if GYOTO_DEBUG_ENABLED
  GYOTO_DEBUG_EXPR(scr->getDmax());
# endif

  return scr;
}
#endif
